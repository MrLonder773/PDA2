// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_Class_sim.cpp
//  Backend поверх std::filesystem — папки на диске
//  рядом с CMakeLists.txt симулятора:
//    sim/fs_data/internal/   (аналог LittleFS)
//    sim/fs_data/sd/         (аналог SD, см. PDA2_SIM_SD_AVAILABLE)
//  Логика (пути, рекурсия, лимиты) — в Fs_Volume.cpp, здесь только
//  монтирование и тонкий backend.
//  Компилируется только в сборке симулятора (-DPDA2_SIM).
//
//  Имитация битого раздела LittleFS: если на месте fs_data/internal
//  лежит ФАЙЛ, а не папка, mount не удаётся (см. begin/format).
// ════════════════════════════════════════════════════════

#include "Fs_Class.h"

#ifdef PDA2_SIM

#include "Fs_Backend.h"
#include "../pda2_config.h"
#include "../pda2_log.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <system_error>

namespace fs = std::filesystem;

static const char* SIM_FS_INTERNAL_ROOT = "fs_data/internal";
static const char* SIM_FS_SD_ROOT       = "fs_data/sd";

namespace {

// ── файл ────────────────────────────────────────────────

class SimFile : public FsBackendFile {
public:
    SimFile(const fs::path& p, FsOpenMode m)
        : _path(p), _writing(m != FsOpenMode::Read), _mode(m) {
        std::ios::openmode om = std::ios::binary;
        if (m == FsOpenMode::Read)       om |= std::ios::in;
        else if (m == FsOpenMode::Write) om |= std::ios::out | std::ios::trunc;
        else                             om |= std::ios::out | std::ios::app;
        _f.open(p, om);
    }
    bool isOpen() const { return _f.is_open(); }

    size_t read(uint8_t* buf, size_t len) override {
        if (!_f.is_open()) return 0;
        _f.read(reinterpret_cast<char*>(buf), (std::streamsize)len);
        return (size_t)_f.gcount();
    }
    size_t write(const uint8_t* buf, size_t len) override {
        if (!_f.is_open()) return 0;
        _f.write(reinterpret_cast<const char*>(buf), (std::streamsize)len);
        return _f.good() ? len : 0;
    }
    uint64_t size() override {
        if (_f.is_open()) _f.flush();
        std::error_code ec;
        uint64_t s = (uint64_t)fs::file_size(_path, ec);
        return ec ? 0 : s;
    }
    bool seek(uint64_t pos) override {
        if (!_f.is_open() || _mode == FsOpenMode::Append) return false;
        _f.clear();                                  // после EOF иначе чтение не вернётся
        if (_mode == FsOpenMode::Read) _f.seekg((std::streamoff)pos);
        else                           _f.seekp((std::streamoff)pos);
        return !_f.fail();
    }
    uint64_t tell() override {
        if (!_f.is_open()) return 0;
        std::streamoff p = (_mode == FsOpenMode::Read) ? (std::streamoff)_f.tellg()
                                                       : (std::streamoff)_f.tellp();
        return p < 0 ? 0 : (uint64_t)p;
    }
    bool close() override {
        if (!_f.is_open()) return true;
        _f.flush();
        bool ok = !_writing || !_f.fail();
        _f.close();
        return ok;
    }

private:
    fs::path     _path;
    bool         _writing;
    FsOpenMode   _mode;
    std::fstream _f;
};

// ── backend ─────────────────────────────────────────────

class SimBackend : public FsBackend {
public:
    explicit SimBackend(const char* root) : _root(root) {}

    bool stat(const std::string& path, bool& isDir, uint64_t& size) override {
        std::error_code ec;
        fs::path f = _full(path);
        fs::file_status st = fs::status(f, ec);
        if (ec || !fs::exists(st)) return false;
        isDir = fs::is_directory(st);
        size = 0;
        if (!isDir) {
            uint64_t s = (uint64_t)fs::file_size(f, ec);
            size = ec ? 0 : s;
        }
        return true;
    }

    bool list(const std::string& dir, std::vector<FsBackendEntry>& out) override {
        out.clear();
        std::error_code ec;
        fs::path d = _full(dir);
        if (!fs::is_directory(d, ec)) return false;
        for (auto& entry : fs::directory_iterator(d, ec)) {
            std::error_code ec2;
            FsBackendEntry e;
            e.name  = entry.path().filename().string();
            e.isDir = entry.is_directory(ec2);
            e.size  = e.isDir ? 0 : (uint64_t)entry.file_size(ec2);
            out.push_back(e);
        }
        return true;
    }

    bool makeDir(const std::string& path) override {
        std::error_code ec;
        bool created = fs::create_directory(_full(path), ec);
        return created && !ec;
    }

    bool removeFile(const std::string& path) override {
        std::error_code ec;
        fs::path f = _full(path);
        if (fs::is_directory(f, ec)) return false;
        bool removed = fs::remove(f, ec);
        return removed && !ec;
    }

    bool removeDir(const std::string& path) override {
        std::error_code ec;
        fs::path f = _full(path);
        if (!fs::is_directory(f, ec)) return false;
        bool removed = fs::remove(f, ec);               // непустая папка — ошибка
        return removed && !ec;
    }

    bool rename(const std::string& from, const std::string& to) override {
        std::error_code ec;
        fs::rename(_full(from), _full(to), ec);
        return !ec;
    }

    std::unique_ptr<FsBackendFile> open(const std::string& path, FsOpenMode mode) override {
        std::error_code ec;
        fs::path f = _full(path);
        if (fs::is_directory(f, ec)) return nullptr;
        if (mode == FsOpenMode::Read && !fs::exists(f, ec)) return nullptr;
        std::unique_ptr<SimFile> file(new SimFile(f, mode));
        if (!file->isOpen()) return nullptr;
        return file;
    }

    uint64_t freeSpace() override {
        std::error_code ec;
        fs::space_info si = fs::space(_root, ec);
        return ec ? 0 : (uint64_t)si.available;
    }
    uint64_t totalSpace() override {
        std::error_code ec;
        fs::space_info si = fs::space(_root, ec);
        return ec ? 0 : (uint64_t)si.capacity;
    }

private:
    fs::path _full(const std::string& path) const {
        return _root / fs::path(path).relative_path();
    }
    fs::path _root;
};

SimBackend s_internalBe(SIM_FS_INTERNAL_ROOT);
SimBackend s_sdBe(SIM_FS_SD_ROOT);

// ── корень ──────────────────────────────────────────────

bool _ensure_root(const char* root) {
    std::error_code ec;
    fs::create_directories(root, ec);
    return !ec;
}

// mount: корень — папка (или её удалось создать). Файл на месте корня = «битый раздел».
bool _mountRoot(const char* root) {
    std::error_code ec;
    if (fs::is_directory(root, ec)) return true;
    if (fs::exists(root, ec)) return false;
    return _ensure_root(root);
}

// format: убрать всё по пути корня и создать пустую папку
bool _formatRoot(const char* root) {
    std::error_code ec;
    fs::remove_all(root, ec);
    if (ec) return false;
    return _ensure_root(root);
}

} // namespace

// ════════════════════════════════════════════════════════
//  FsInternal (sim)
// ════════════════════════════════════════════════════════

bool FsInternal::begin() {
    return begin(true);
}

bool FsInternal::begin(bool allowFormat) {
    _be = &s_internalBe;
    _formatted = false;
    _ok = _mountRoot(SIM_FS_INTERNAL_ROOT);
    if (!_ok && allowFormat) {
        PDA_LOGW("fs.int", "sim FS mount failed, formatting");
        _ok = _formatRoot(SIM_FS_INTERNAL_ROOT);
        _formatted = _ok;
    }
    if (_ok)
        PDA_LOGI("fs.int", "sim FS ok (%s). Free: %u / %u MB",
                 SIM_FS_INTERNAL_ROOT,
                 (unsigned)(freeSpace() / (1024ULL * 1024ULL)),
                 (unsigned)(totalSpace() / (1024ULL * 1024ULL)));
    else if (allowFormat)
        PDA_LOGE("fs.int", "sim FS mount failed (%s)", SIM_FS_INTERNAL_ROOT);
    else
        PDA_LOGE("fs.int", "sim FS mount failed, data kept (not formatted)");
    return _ok;
}

bool FsInternal::format() {
    if (_ok) return false;
    _be = &s_internalBe;
    _ok = _formatRoot(SIM_FS_INTERNAL_ROOT);
    _formatted = _ok;
    if (_ok) PDA_LOGW("fs.int", "sim FS formatted by request");
    else     PDA_LOGE("fs.int", "sim FS format failed");
    return _ok;
}

// ════════════════════════════════════════════════════════
//  FsSd (sim) — доступность управляется PDA2_SIM_SD_AVAILABLE
// ════════════════════════════════════════════════════════

bool FsSd::begin() {
    _be = &s_sdBe;
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
        PDA_LOGI("fs.sd", "sim SD ok (%s). Free: %u / %u MB",
                 SIM_FS_SD_ROOT,
                 (unsigned)(freeSpace() / (1024ULL * 1024ULL)),
                 (unsigned)(totalSpace() / (1024ULL * 1024ULL)));
    else
        PDA_LOGI("fs.sd", "sim SD not available (PDA2_SIM_SD_AVAILABLE)");
    return _ok;
}

#endif // PDA2_SIM