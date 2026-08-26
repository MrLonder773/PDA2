// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_Class_sim.cpp
//  Реальная ФС поверх std::filesystem — папки на диске
//  рядом с CMakeLists.txt симулятора:
//    sim/fs_data/internal/   (аналог LittleFS)
//    sim/fs_data/sd/         (аналог SD, см. PDA2_SIM_SD_AVAILABLE)
//  Компилируется только в сборке симулятора (-DPDA2_SIM).
// ════════════════════════════════════════════════════════

#include "Fs_Class.h"

#ifdef PDA2_SIM

#include "../pda2_config.h"
#include "../pda2_log.h"
#include <filesystem>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

static const char* SIM_FS_INTERNAL_ROOT = "fs_data/internal";
static const char* SIM_FS_SD_ROOT       = "fs_data/sd";

// ── общие хелперы (переиспользуются FsInternal и FsSd) ───

static bool _ensure_root(const char* root) {
    std::error_code ec;
    fs::create_directories(root, ec);
    return !ec;
}

// root + path, с санитайзингом ".." — приложение не может выйти за пределы root
static fs::path _full(const char* root, const char* path) {
    fs::path p = fs::path(root) / fs::path(path).relative_path();
    fs::path clean;
    for (auto& part : p) {
        if (part == "..") continue;   // выкинуть попытки выйти наружу
        clean /= part;
    }
    return clean;
}

static std::vector<FsEntry> _ls(const char* root, const char* path) {
    std::vector<FsEntry> result;
    std::error_code ec;
    fs::path dir = _full(root, path);
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return result;

    for (auto& entry : fs::directory_iterator(dir, ec)) {
        FsEntry e;
        e.name  = entry.path().filename().string();
        e.isDir = entry.is_directory();
        e.size  = e.isDir ? 0 : entry.file_size();
        result.push_back(e);
    }
    return result;
}

static size_t _dirSize(const fs::path& p) {
    size_t total = 0;
    std::error_code ec;
    if (!fs::exists(p, ec) || !fs::is_directory(p, ec)) return 0;
    for (auto& entry : fs::recursive_directory_iterator(p, ec)) {
        if (!entry.is_directory()) {
            std::error_code sz_ec;
            total += entry.file_size(sz_ec);
        }
    }
    return total;
}

static bool _copy(const fs::path& from, const fs::path& to) {
    std::error_code ec;
    fs::copy_file(from, to, fs::copy_options::overwrite_existing, ec);
    return !ec;
}

// ════════════════════════════════════════════════════════
//  FsInternal (sim)
// ════════════════════════════════════════════════════════

bool FsInternal::begin() {
    _ok = _ensure_root(SIM_FS_INTERNAL_ROOT);
    if (_ok)
        PDA_LOGI("fs.int", "sim FS ok (%s). Free: %u / %u",
                 SIM_FS_INTERNAL_ROOT,
                 (unsigned)freeSpace(), (unsigned)totalSpace());
    else
        PDA_LOGE("fs.int", "sim FS mkdir failed (%s)", SIM_FS_INTERNAL_ROOT);
    return _ok;
}

std::vector<FsEntry> FsInternal::ls(const char* path) {
    if (!_ok) return {};
    return _ls(SIM_FS_INTERNAL_ROOT, path);
}

bool FsInternal::exists(const char* path) {
    if (!_ok) return false;
    std::error_code ec;
    return fs::exists(_full(SIM_FS_INTERNAL_ROOT, path), ec);
}

bool FsInternal::isDir(const char* path) {
    if (!_ok) return false;
    std::error_code ec;
    return fs::is_directory(_full(SIM_FS_INTERNAL_ROOT, path), ec);
}

bool FsInternal::mkdir(const char* path) {
    if (!_ok) return false;
    std::error_code ec;
    fs::create_directories(_full(SIM_FS_INTERNAL_ROOT, path), ec);
    return !ec;
}

String FsInternal::read(const char* path) {
    if (!_ok) return "";
    std::ifstream f(_full(SIM_FS_INTERNAL_ROOT, path), std::ios::binary);
    if (!f) return "";
    return String((std::istreambuf_iterator<char>(f)),
                   std::istreambuf_iterator<char>());
}

bool FsInternal::readBytes(const char* path, uint8_t* buf, size_t len) {
    if (!_ok) return false;
    std::ifstream f(_full(SIM_FS_INTERNAL_ROOT, path), std::ios::binary);
    if (!f) return false;
    f.read(reinterpret_cast<char*>(buf), len);
    return (size_t)f.gcount() == len;
}

bool FsInternal::write(const char* path, const String& data) {
    if (!_ok) return false;
    std::ofstream f(_full(SIM_FS_INTERNAL_ROOT, path),
                     std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(data.data(), data.size());
    return f.good();
}

bool FsInternal::writeBytes(const char* path, const uint8_t* buf, size_t len) {
    if (!_ok) return false;
    std::ofstream f(_full(SIM_FS_INTERNAL_ROOT, path),
                     std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(buf), len);
    return f.good();
}

bool FsInternal::remove(const char* path) {
    if (!_ok) return false;
    std::error_code ec;
    return fs::remove(_full(SIM_FS_INTERNAL_ROOT, path), ec);
}

bool FsInternal::rename(const char* from, const char* to) {
    if (!_ok) return false;
    std::error_code ec;
    fs::rename(_full(SIM_FS_INTERNAL_ROOT, from),
               _full(SIM_FS_INTERNAL_ROOT, to), ec);
    return !ec;
}

bool FsInternal::copy(const char* from, const char* to) {
    if (!_ok) return false;
    return _copy(_full(SIM_FS_INTERNAL_ROOT, from),
                 _full(SIM_FS_INTERNAL_ROOT, to));
}

size_t FsInternal::size(const char* path) {
    if (!_ok) return 0;
    std::error_code ec;
    size_t s = fs::file_size(_full(SIM_FS_INTERNAL_ROOT, path), ec);
    return ec ? 0 : s;
}

size_t FsInternal::dirSize(const char* path) {
    if (!_ok) return 0;
    return _dirSize(_full(SIM_FS_INTERNAL_ROOT, path));
}

size_t FsInternal::freeSpace() {
    if (!_ok) return 0;
    std::error_code ec;
    fs::space_info si = fs::space(SIM_FS_INTERNAL_ROOT, ec);
    return ec ? 0 : (size_t)si.available;
}

size_t FsInternal::totalSpace() {
    if (!_ok) return 0;
    std::error_code ec;
    fs::space_info si = fs::space(SIM_FS_INTERNAL_ROOT, ec);
    return ec ? 0 : (size_t)si.capacity;
}

// ════════════════════════════════════════════════════════
//  FsSd (sim) — доступность управляется PDA2_SIM_SD_AVAILABLE
// ════════════════════════════════════════════════════════

bool FsSd::begin() {
#ifdef PDA2_SIM_SD_AVAILABLE
    #if PDA2_SIM_SD_AVAILABLE
        _ok = _ensure_root(SIM_FS_SD_ROOT);
    #else
        _ok = false;
    #endif
#else
    _ok = false;
#endif
    if (_ok)
        PDA_LOGI("fs.sd", "sim SD ok (%s). Free: %u / %u",
                 SIM_FS_SD_ROOT, (unsigned)freeSpace(), (unsigned)totalSpace());
    else
        PDA_LOGI("fs.sd", "sim SD not available (PDA2_SIM_SD_AVAILABLE)");
    return _ok;
}

std::vector<FsEntry> FsSd::ls(const char* path) {
    if (!_ok) return {};
    return _ls(SIM_FS_SD_ROOT, path);
}

bool FsSd::exists(const char* path) {
    if (!_ok) return false;
    std::error_code ec;
    return fs::exists(_full(SIM_FS_SD_ROOT, path), ec);
}

bool FsSd::isDir(const char* path) {
    if (!_ok) return false;
    std::error_code ec;
    return fs::is_directory(_full(SIM_FS_SD_ROOT, path), ec);
}

bool FsSd::mkdir(const char* path) {
    if (!_ok) return false;
    std::error_code ec;
    fs::create_directories(_full(SIM_FS_SD_ROOT, path), ec);
    return !ec;
}

String FsSd::read(const char* path) {
    if (!_ok) return "";
    std::ifstream f(_full(SIM_FS_SD_ROOT, path), std::ios::binary);
    if (!f) return "";
    return String((std::istreambuf_iterator<char>(f)),
                   std::istreambuf_iterator<char>());
}

bool FsSd::readBytes(const char* path, uint8_t* buf, size_t len) {
    if (!_ok) return false;
    std::ifstream f(_full(SIM_FS_SD_ROOT, path), std::ios::binary);
    if (!f) return false;
    f.read(reinterpret_cast<char*>(buf), len);
    return (size_t)f.gcount() == len;
}

bool FsSd::write(const char* path, const String& data) {
    if (!_ok) return false;
    std::ofstream f(_full(SIM_FS_SD_ROOT, path),
                     std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(data.data(), data.size());
    return f.good();
}

bool FsSd::writeBytes(const char* path, const uint8_t* buf, size_t len) {
    if (!_ok) return false;
    std::ofstream f(_full(SIM_FS_SD_ROOT, path),
                     std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(buf), len);
    return f.good();
}

bool FsSd::remove(const char* path) {
    if (!_ok) return false;
    std::error_code ec;
    return fs::remove(_full(SIM_FS_SD_ROOT, path), ec);
}

bool FsSd::rename(const char* from, const char* to) {
    if (!_ok) return false;
    std::error_code ec;
    fs::rename(_full(SIM_FS_SD_ROOT, from),
               _full(SIM_FS_SD_ROOT, to), ec);
    return !ec;
}

bool FsSd::copy(const char* from, const char* to) {
    if (!_ok) return false;
    return _copy(_full(SIM_FS_SD_ROOT, from),
                 _full(SIM_FS_SD_ROOT, to));
}

size_t FsSd::size(const char* path) {
    if (!_ok) return 0;
    std::error_code ec;
    size_t s = fs::file_size(_full(SIM_FS_SD_ROOT, path), ec);
    return ec ? 0 : s;
}

size_t FsSd::dirSize(const char* path) {
    if (!_ok) return 0;
    return _dirSize(_full(SIM_FS_SD_ROOT, path));
}

size_t FsSd::freeSpace() {
    if (!_ok) return 0;
    std::error_code ec;
    fs::space_info si = fs::space(SIM_FS_SD_ROOT, ec);
    return ec ? 0 : (size_t)si.available;
}

size_t FsSd::totalSpace() {
    if (!_ok) return 0;
    std::error_code ec;
    fs::space_info si = fs::space(SIM_FS_SD_ROOT, ec);
    return ec ? 0 : (size_t)si.capacity;
}

#endif // PDA2_SIM