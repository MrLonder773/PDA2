// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_Volume.cpp
//  Вся логика Fs в одном месте, одинаковая на ESP32 и в sim:
//  нормализация путей, рекурсия, лимиты, copy с откатом, find,
//  removeAll, дозапись, потоки, коды ошибок (FsResult).
//  Платформа даёт только FsBackend.
//
//  Папка перечисляется целиком и закрывается ДО рекурсии и удаления:
//  у SD ограничено число одновременно открытых файлов, а удалять
//  записи во время обхода папки небезопасно.
//  Порядок в ls() и find() детерминирован: по имени, побайтно.
//  Буфер ввода-вывода (kChunk) берётся из PSRAM на время операции.
// ════════════════════════════════════════════════════════

#include "Fs_Class.h"
#include "Fs_Backend.h"
#include "Fs_Path.h"
#include "../pda2_config.h"
#include "../pda2_platform.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr size_t kChunk = 4096;

// 4 КБ из PSRAM на время операции; на ПК — обычный malloc
struct IoBuf {
    uint8_t* p;
    IoBuf() : p((uint8_t*)pda2_platform_psram_malloc(kChunk)) {}
    ~IoBuf() { if (p) free(p); }
    IoBuf(const IoBuf&)            = delete;
    IoBuf& operator=(const IoBuf&) = delete;
};

struct DirItem {
    std::string path;   // полный путь
    std::string name;   // только имя
    bool        isDir;
    uint64_t    size;
};

struct ByName {
    bool operator()(const FsBackendEntry& a, const FsBackendEntry& b) const {
        return a.name < b.name;
    }
};

std::string _join(const std::string& dir, const std::string& name) {
    return dir == "/" ? "/" + name : dir + "/" + name;
}

std::string _lower(std::string s) {   // только ASCII
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
    return s;
}

bool _strAppend(String& s, const char* p, size_t n) {
#ifdef PDA2_SIM
    s.append(p, n);
    return true;
#else
    return s.concat(p, (unsigned int)n);
#endif
}

bool _reserve(String& s, size_t n) {
#ifdef PDA2_SIM
    s.reserve(n);
    return true;
#else
    return s.reserve((unsigned int)n);
#endif
}

// ── причины отказа ──────────────────────────────────────

// файл должен существовать и не быть папкой
FsResult _needFile(FsBackend& b, const std::string& path, uint64_t& sz) {
    bool isDir = false;
    sz = 0;
    if (!b.stat(path, isDir, sz)) return FsResult::NotFound;
    if (isDir) return FsResult::IsDir;
    return FsResult::Ok;
}

// родитель path существует и является папкой
FsResult _parentCheck(FsBackend& b, const std::string& path) {
    size_t slash = path.rfind('/');
    if (slash == std::string::npos || slash == 0) return FsResult::Ok;   // корень
    bool isDir = false;
    uint64_t sz = 0;
    if (!b.stat(path.substr(0, slash), isDir, sz)) return FsResult::NotFound;
    if (!isDir) return FsResult::NotDir;
    return FsResult::Ok;
}

// запись не дошла: эвристика по свободному месту (на плате не проверено)
FsResult _writeFail(FsBackend& b, size_t len) {
    return b.freeSpace() < (uint64_t)len ? FsResult::NoSpace : FsResult::IoError;
}

bool _isDir(FsBackend& b, const std::string& path) {
    bool isDir = false;
    uint64_t sz = 0;
    return b.stat(path, isDir, sz) && isDir;
}

bool _listDir(FsBackend& b, const std::string& dir, std::vector<DirItem>& items) {
    items.clear();
    std::vector<FsBackendEntry> raw;
    if (!b.list(dir, raw)) return false;
    std::sort(raw.begin(), raw.end(), ByName());
    items.reserve(raw.size());
    for (const FsBackendEntry& e : raw) {
        DirItem it;
        it.name  = e.name;
        it.path  = _join(dir, e.name);
        it.isDir = e.isDir;
        it.size  = e.isDir ? 0 : e.size;
        items.push_back(it);
    }
    return true;
}

FsResult _mkdirs(FsBackend& b, const std::string& path, uint8_t depth) {
    if (path == "/") return FsResult::Ok;
    bool isDir = false;
    uint64_t sz = 0;
    if (b.stat(path, isDir, sz))                    // есть: папка — успех, файл — отказ
        return isDir ? FsResult::Ok : FsResult::NotDir;
    if (depth > PDA2_FS_MAX_DEPTH) return FsResult::DepthLimit;
    size_t slash = path.rfind('/');
    if (slash != std::string::npos && slash > 0) {
        FsResult r = _mkdirs(b, path.substr(0, slash), depth + 1);
        if (!r) return r;
    }
    return b.makeDir(path) ? FsResult::Ok : FsResult::IoError;
}

uint64_t _dirSize(FsBackend& b, const std::string& path, uint8_t depth) {
    if (depth > PDA2_FS_MAX_DEPTH) return 0;
    std::vector<DirItem> items;
    if (!_listDir(b, path, items)) return 0;
    uint64_t total = 0;
    for (const DirItem& it : items)
        total += it.isDir ? _dirSize(b, it.path, depth + 1) : it.size;
    return total;
}

FsResult _removeAll(FsBackend& b, const std::string& path, uint8_t depth) {
    bool isDir = false;
    uint64_t sz = 0;
    if (!b.stat(path, isDir, sz)) return FsResult::NotFound;
    if (!isDir) return b.removeFile(path) ? FsResult::Ok : FsResult::IoError;
    if (depth > PDA2_FS_MAX_DEPTH) return FsResult::DepthLimit;
    std::vector<DirItem> items;
    if (!_listDir(b, path, items)) return FsResult::IoError;
    FsResult first = FsResult::Ok;                  // первая ошибка, обход не прерываем
    for (const DirItem& it : items) {
        FsResult r = _removeAll(b, it.path, depth + 1);
        if (first && !r) first = r;
    }
    if (!b.removeDir(path) && first) first = FsResult::IoError;
    return first;
}

void _find(FsBackend& b, const std::string& dir, const std::string& needle,
           uint8_t depth, std::vector<std::string>& out) {
    std::vector<DirItem> items;
    if (!_listDir(b, dir, items)) return;
    for (const DirItem& it : items) {
        if (out.size() >= (size_t)PDA2_FS_FIND_MAX) return;
        if (_lower(it.name).find(needle) != std::string::npos) out.push_back(it.path);
        if (it.isDir && depth < PDA2_FS_MAX_DEPTH) _find(b, it.path, needle, depth + 1, out);
    }
}

FsResult _put(FsBackend& b, const std::string& path, const uint8_t* data, size_t len, FsOpenMode mode) {
    if (_isDir(b, path)) return FsResult::IsDir;
    std::unique_ptr<FsBackendFile> f = b.open(path, mode);
    if (!f) {
        FsResult r = _parentCheck(b, path);
        return r ? FsResult(FsResult::IoError) : r;
    }
    size_t n = len ? f->write(data, len) : 0;
    bool closed = f->close();
    if (n != len) return _writeFail(b, len);
    return closed ? FsResult::Ok : FsResult::IoError;   // пустые данные — успех
}

FsResult _copyFile(FsBackend& b, const std::string& from, const std::string& to) {
    if (from == to) return FsResult::Exists;        // иначе запись обнулит источник
    uint64_t sz = 0;
    FsResult r = _needFile(b, from, sz);
    if (!r) return r;
    r = _parentCheck(b, to);
    if (!r) return r;
    if (_isDir(b, to)) return FsResult::IsDir;
    IoBuf buf;
    if (!buf.p) return FsResult::NoMemory;
    std::unique_ptr<FsBackendFile> src = b.open(from, FsOpenMode::Read);
    if (!src) return FsResult::IoError;
    std::unique_ptr<FsBackendFile> dst = b.open(to, FsOpenMode::Write);
    if (!dst) return FsResult::IoError;
    FsResult res = FsResult::Ok;
    size_t n;
    while ((n = src->read(buf.p, kChunk)) > 0) {
        if (dst->write(buf.p, n) != n) { res = _writeFail(b, n); break; }
    }
    src->close();
    if (!dst->close() && res) res = FsResult::IoError;
    if (!res) b.removeFile(to);                     // не оставлять обрезанный файл
    return res;
}

} // namespace

// ════════════════════════════════════════════════════════
//  FsStream
// ════════════════════════════════════════════════════════

FsStream::FsStream() : _mode(FsOpenMode::Read), _be(nullptr) {}
FsStream::~FsStream() { close(); }

FsStream::FsStream(FsStream&& o) noexcept
    : _f(std::move(o._f)), _mode(o._mode), _be(o._be) { o._be = nullptr; }

FsStream& FsStream::operator=(FsStream&& o) noexcept {
    if (this != &o) {
        close();
        _f    = std::move(o._f);
        _mode = o._mode;
        _be   = o._be;
        o._be = nullptr;
    }
    return *this;
}

FsResult FsStream::read(uint8_t* buf, size_t len, size_t& got) {
    got = 0;
    if (!_f) return FsResult::IoError;
    if (_mode != FsOpenMode::Read) return FsResult::Unsupported;
    if (!buf && len) return FsResult::BadPath;
    while (got < len) {
        size_t n = _f->read(buf + got, len - got);
        if (n == 0) break;
        got += n;
    }
    return FsResult::Ok;
}

FsResult FsStream::write(const uint8_t* buf, size_t len) {
    if (!_f) return FsResult::IoError;
    if (_mode == FsOpenMode::Read) return FsResult::Unsupported;
    if (!len) return FsResult::Ok;
    if (!buf) return FsResult::BadPath;
    if (_f->write(buf, len) != len) return _be ? _writeFail(*_be, len) : FsResult(FsResult::IoError);
    return FsResult::Ok;
}

FsResult FsStream::seek(uint64_t pos) {
    if (!_f) return FsResult::IoError;
    if (_mode == FsOpenMode::Append) return FsResult::Unsupported;
    return _f->seek(pos) ? FsResult::Ok : FsResult::IoError;
}

uint64_t FsStream::tell() { return _f ? _f->tell() : 0; }
uint64_t FsStream::size() { return _f ? _f->size() : 0; }

FsResult FsStream::close() {
    if (!_f) return FsResult::Ok;
    bool ok = _f->close();
    _f.reset();
    return ok ? FsResult::Ok : FsResult::IoError;
}

// ════════════════════════════════════════════════════════
//  FsVolume
// ════════════════════════════════════════════════════════

bool FsVolume::_path(const char* in, std::string& out) const {
    return _ok && _be && fs_normPath(in, out);
}

FsResult FsVolume::_prep(const char* in, std::string& out) const {
    if (!_ok || !_be) return FsResult::NotMounted;
    return fs_normPath(in, out) ? FsResult::Ok : FsResult::BadPath;
}

std::vector<FsEntry> FsVolume::ls(const char* path) {
    std::vector<FsEntry> result;
    std::string p;
    if (!_path(path, p)) return result;
    std::vector<DirItem> items;
    if (!_listDir(*_be, p, items)) return result;
    for (const DirItem& it : items) {
        FsEntry e;
        e.name  = it.name.c_str();
        e.isDir = it.isDir;
        e.size  = (size_t)it.size;
        result.push_back(e);
    }
    return result;
}

bool FsVolume::exists(const char* path) {
    std::string p;
    bool isDir = false;
    uint64_t sz = 0;
    return _path(path, p) && _be->stat(p, isDir, sz);
}

bool FsVolume::isDir(const char* path) {
    std::string p;
    return _path(path, p) && _isDir(*_be, p);
}

FsResult FsVolume::mkdir(const char* path) {
    std::string p;
    FsResult r = _prep(path, p);
    if (!r) return r;
    return _mkdirs(*_be, p, 0);
}

FsResult FsVolume::read(const char* path, String& out) {
    out = String();
    std::string p;
    FsResult r = _prep(path, p);
    if (!r) return r;
    uint64_t sz = 0;
    r = _needFile(*_be, p, sz);
    if (!r) return r;
    const uint64_t limit = _readMax ? (uint64_t)_readMax : (uint64_t)PDA2_FS_READ_MAX;
    if (sz > limit) return FsResult::TooBig;
    if (sz == 0) return FsResult::Ok;
    IoBuf buf;
    if (!buf.p) return FsResult::NoMemory;
    if (!_reserve(out, (size_t)sz)) { out = String(); return FsResult::NoMemory; }
    std::unique_ptr<FsBackendFile> f = _be->open(p, FsOpenMode::Read);
    if (!f) { out = String(); return FsResult::IoError; }
    uint64_t got = 0;
    while (got < sz) {
        size_t want = std::min<size_t>(kChunk, (size_t)(sz - got));
        size_t n = f->read(buf.p, want);
        if (n == 0) break;
        if (!_strAppend(out, reinterpret_cast<const char*>(buf.p), n)) {
            out = String();
            return FsResult::NoMemory;
        }
        got += n;
    }
    if (got != sz) { out = String(); return FsResult::IoError; }
    return FsResult::Ok;
}

String FsVolume::read(const char* path) {
    String s;
    read(path, s);
    return s;
}

FsResult FsVolume::readBytes(const char* path, uint8_t* buf, size_t len) {
    return readBytes(path, buf, len, 0);
}

FsResult FsVolume::readBytes(const char* path, uint8_t* buf, size_t len, uint64_t offset) {
    std::string p;
    FsResult r = _prep(path, p);
    if (!r) return r;
    uint64_t sz = 0;
    r = _needFile(*_be, p, sz);
    if (!r) return r;
    if (!buf && len) return FsResult::BadPath;
    if (offset > sz || (uint64_t)len > sz - offset) return FsResult::ShortRead;
    std::unique_ptr<FsBackendFile> f = _be->open(p, FsOpenMode::Read);
    if (!f) return FsResult::IoError;
    if (offset && !f->seek(offset)) return FsResult::IoError;
    size_t got = 0;
    while (got < len) {
        size_t n = f->read(buf + got, len - got);
        if (n == 0) break;
        got += n;
    }
    return got == len ? FsResult::Ok : FsResult::ShortRead;
}

FsResult FsVolume::write(const char* path, const String& data) {
    std::string p;
    FsResult r = _prep(path, p);
    if (!r) return r;
    return _put(*_be, p, reinterpret_cast<const uint8_t*>(data.c_str()), (size_t)data.length(), FsOpenMode::Write);
}

FsResult FsVolume::writeBytes(const char* path, const uint8_t* buf, size_t len) {
    std::string p;
    FsResult r = _prep(path, p);
    if (!r) return r;
    if (!buf && len) return FsResult::BadPath;
    return _put(*_be, p, buf, len, FsOpenMode::Write);
}

FsResult FsVolume::append(const char* path, const String& data) {
    std::string p;
    FsResult r = _prep(path, p);
    if (!r) return r;
    return _put(*_be, p, reinterpret_cast<const uint8_t*>(data.c_str()), (size_t)data.length(), FsOpenMode::Append);
}

FsResult FsVolume::remove(const char* path) {
    std::string p;
    FsResult r = _prep(path, p);
    if (!r) return r;
    uint64_t sz = 0;
    r = _needFile(*_be, p, sz);                     // папки — только removeAll
    if (!r) return r;
    return _be->removeFile(p) ? FsResult::Ok : FsResult::IoError;
}

FsResult FsVolume::removeAll(const char* path) {
    std::string p;
    FsResult r = _prep(path, p);
    if (!r) return r;
    if (p == "/") return FsResult::BadPath;         // корень не удаляем
    return _removeAll(*_be, p, 0);
}

FsResult FsVolume::rename(const char* from, const char* to) {
    std::string a, b;
    FsResult r = _prep(from, a);
    if (!r) return r;
    r = _prep(to, b);
    if (!r) return r;
    if (a == "/" || b == "/") return FsResult::BadPath;
    bool isDir = false;
    uint64_t sz = 0;
    if (!_be->stat(a, isDir, sz)) return FsResult::NotFound;
    r = _parentCheck(*_be, b);
    if (!r) return r;
    return _be->rename(a, b) ? FsResult::Ok : FsResult::IoError;
}

FsResult FsVolume::copy(const char* from, const char* to) {
    std::string a, b;
    FsResult r = _prep(from, a);
    if (!r) return r;
    r = _prep(to, b);
    if (!r) return r;
    return _copyFile(*_be, a, b);
}

FsResult FsVolume::open(const char* path, FsOpenMode mode, FsStream& out) {
    out.close();
    std::string p;
    FsResult r = _prep(path, p);
    if (!r) return r;
    if (mode == FsOpenMode::Read) {
        uint64_t sz = 0;
        r = _needFile(*_be, p, sz);
        if (!r) return r;
    } else {
        if (_isDir(*_be, p)) return FsResult::IsDir;
        r = _parentCheck(*_be, p);
        if (!r) return r;
    }
    std::unique_ptr<FsBackendFile> f = _be->open(p, mode);
    if (!f) return FsResult::IoError;
    out._f    = std::move(f);
    out._mode = mode;
    out._be   = _be;
    return FsResult::Ok;
}

FsResult FsVolume::find(const char* root, const char* name,
                        std::vector<String>& out, bool* truncated) {
    out.clear();
    if (truncated) *truncated = false;
    std::string r;
    FsResult res = _prep(root, r);
    if (!res) return res;
    std::string needle = _lower(name ? name : "");
    if (needle.empty()) return FsResult::BadPath;
    bool isDir = false;
    uint64_t sz = 0;
    if (!_be->stat(r, isDir, sz)) return FsResult::NotFound;
    if (!isDir) return FsResult::NotDir;
    std::vector<std::string> found;
    _find(*_be, r, needle, 0, found);
    for (const std::string& s : found) out.push_back(String(s.c_str()));
    if (truncated) *truncated = found.size() >= (size_t)PDA2_FS_FIND_MAX;
    return FsResult::Ok;
}

std::vector<String> FsVolume::find(const char* root, const char* name) {
    std::vector<String> result;
    find(root, name, result, nullptr);
    return result;
}

size_t FsVolume::size(const char* path) {
    std::string p;
    if (!_path(path, p)) return 0;
    bool isDir = false;
    uint64_t sz = 0;
    if (!_be->stat(p, isDir, sz) || isDir) return 0;
    return (size_t)sz;
}

uint64_t FsVolume::dirSize(const char* path) {
    std::string p;
    return _path(path, p) ? _dirSize(*_be, p, 0) : 0;
}

uint64_t FsVolume::freeSpace()  { return (_ok && _be) ? _be->freeSpace()  : 0; }
uint64_t FsVolume::totalSpace() { return (_ok && _be) ? _be->totalSpace() : 0; }