// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_Class_esp32.cpp
//  Backend поверх fs::FS: LittleFS (internal) — обе платы,
//  SD карта — отдельная SPI-шина, только pda2 (не разведена
//  на PDA2_LITE, см. pda2_config.h).
//  Логика (пути, рекурсия, лимиты) — в Fs_Volume.cpp, здесь только
//  монтирование и тонкий backend.
//  Компилируется только на ESP32 (PDA2_SIM не определён).
// ════════════════════════════════════════════════════════

#include "Fs_Class.h"

#ifndef PDA2_SIM

#include "Fs_Backend.h"
#include "../pda2_config.h"
#include "../pda2_log.h"
#include <LittleFS.h>
#include <SD.h>
#include <SPI.h>
#include <memory>
#include <string>
#include <vector>

namespace {

// ── файл ────────────────────────────────────────────────

class EspFile : public FsBackendFile {
public:
    EspFile(File f, bool append) : _f(f), _open(true), _append(append), _failed(false) {}
    ~EspFile() override { close(); }

    size_t read(uint8_t* buf, size_t len) override {
        return _open ? _f.read(buf, len) : 0;
    }
    size_t write(const uint8_t* buf, size_t len) override {
        if (!_open) return 0;
        size_t n = _f.write(buf, len);
        if (n != len) _failed = true;
        return n;
    }
    uint64_t size() override {
        return _open ? (uint64_t)_f.size() : 0;
    }
    bool seek(uint64_t pos) override {
        if (!_open || _append || pos > 0xFFFFFFFFull) return false;
        return _f.seek((uint32_t)pos, SeekSet);
    }
    uint64_t tell() override {
        return _open ? (uint64_t)_f.position() : 0;
    }
    bool close() override {
        if (_open) {
            _f.close();
            _open = false;
        }
        return !_failed;     // false — запись не дошла (раньше всегда true)
    }

private:
    File _f;
    bool _open;
    bool _append;
    bool _failed;
};

// ── backend (общая часть для LittleFS и SD) ─────────────

class EspBackend : public FsBackend {
public:
    explicit EspBackend(fs::FS& f) : _fs(f) {}

    bool stat(const std::string& path, bool& isDir, uint64_t& size) override {
        File f = _fs.open(path.c_str());
        if (!f) return false;
        isDir = f.isDirectory();
        size  = isDir ? 0 : (uint64_t)f.size();
        f.close();
        return true;
    }

    bool list(const std::string& dir, std::vector<FsBackendEntry>& out) override {
        out.clear();
        File d = _fs.open(dir.c_str());
        if (!d || !d.isDirectory()) {
            d.close();
            return false;
        }
        File e = d.openNextFile();
        while (e) {
            FsBackendEntry it;
            std::string nm = e.name();
            size_t slash = nm.rfind('/');
            it.name  = (slash == std::string::npos) ? nm : nm.substr(slash + 1);
            it.isDir = e.isDirectory();
            it.size  = it.isDir ? 0 : (uint64_t)e.size();
            out.push_back(it);
            e.close();
            e = d.openNextFile();
        }
        d.close();
        return true;
    }

    bool makeDir(const std::string& path) override    { return _fs.mkdir(path.c_str()); }
    bool removeFile(const std::string& path) override { return _fs.remove(path.c_str()); }
    bool removeDir(const std::string& path) override  { return _fs.rmdir(path.c_str()); }
    bool rename(const std::string& from, const std::string& to) override {
        return _fs.rename(from.c_str(), to.c_str());
    }

    std::unique_ptr<FsBackendFile> open(const std::string& path, FsOpenMode mode) override {
        const char* m = (mode == FsOpenMode::Read)  ? FILE_READ
                      : (mode == FsOpenMode::Write) ? FILE_WRITE
                                                    : FILE_APPEND;
        File f = _fs.open(path.c_str(), m);
        if (!f || f.isDirectory()) {
            f.close();
            return nullptr;
        }
        return std::unique_ptr<FsBackendFile>(new EspFile(f, mode == FsOpenMode::Append));
    }

private:
    fs::FS& _fs;
};

class LfsBackend : public EspBackend {
public:
    LfsBackend() : EspBackend(LittleFS) {}
    uint64_t freeSpace() override  { return (uint64_t)LittleFS.totalBytes() - (uint64_t)LittleFS.usedBytes(); }
    uint64_t totalSpace() override { return (uint64_t)LittleFS.totalBytes(); }
};

class SdBackend : public EspBackend {
public:
    SdBackend() : EspBackend(SD) {}
    uint64_t freeSpace() override  { return SD.totalBytes() - SD.usedBytes(); }
    uint64_t totalSpace() override { return SD.totalBytes(); }
};

LfsBackend s_lfsBe;
SdBackend  s_sdBe;

} // namespace

// ════════════════════════════════════════════════════════
//  FsInternal (LittleFS) — общая для обеих плат
// ════════════════════════════════════════════════════════

bool FsInternal::begin() {
    return begin(true);
}

bool FsInternal::begin(bool allowFormat) {
    _be = &s_lfsBe;
    _formatted = false;
    _ok = LittleFS.begin(false);
    if (!_ok && allowFormat) {
        PDA_LOGW("fs.int", "LittleFS mount failed, formatting");
        _ok = LittleFS.begin(true);
        _formatted = _ok;
    }
    if (_ok)
        PDA_LOGI("fs.int", "LittleFS ok. Free: %u / %u",
                 (unsigned)freeSpace(), (unsigned)totalSpace());
    else if (allowFormat)
        PDA_LOGE("fs.int", "LittleFS mount failed");
    else
        PDA_LOGE("fs.int", "LittleFS mount failed, data kept (not formatted)");
    return _ok;
}

bool FsInternal::format() {
    if (_ok) return false;
    _be = &s_lfsBe;
    _ok = LittleFS.begin(true);
    _formatted = _ok;
    if (_ok) PDA_LOGW("fs.int", "LittleFS formatted by request");
    else     PDA_LOGE("fs.int", "LittleFS format failed");
    return _ok;
}

// ════════════════════════════════════════════════════════
//  FsSd (SD карта, отдельная SPI-шина — GPIO 6/7/21/42)
//  Только pda2 — на PDA2_LITE не разведена (см. pda2_config.h)
// ════════════════════════════════════════════════════════

static SPIClass _spiSd(HSPI);

bool FsSd::begin() {
    _be = &s_sdBe;
    _spiSd.begin(PDA2_PIN_SD_SCK, PDA2_PIN_SD_MISO,
                 PDA2_PIN_SD_MOSI, PDA2_PIN_SD_CS);
    _ok = SD.begin(PDA2_PIN_SD_CS, _spiSd);
    if (_ok)
        PDA_LOGI("fs.sd", "SD ok. Free: %u / %u MB",
                 (unsigned)(freeSpace() / (1024ULL * 1024ULL)),
                 (unsigned)(totalSpace() / (1024ULL * 1024ULL)));
    else
        PDA_LOGE("fs.sd", "SD mount failed");
    return _ok;
}

#endif // !PDA2_SIM