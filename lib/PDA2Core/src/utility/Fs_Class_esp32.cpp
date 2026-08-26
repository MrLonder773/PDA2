// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_Class_esp32.cpp
//  LittleFS (internal) — обе платы.
//  SD карта — отдельная SPI-шина, только pda2 (не разведена
//  на PDA2_LITE, см. pda2_config.h).
//  Компилируется только на ESP32 (PDA2_SIM не определён).
// ════════════════════════════════════════════════════════

#include "Fs_Class.h"

#ifndef PDA2_SIM

#include "../pda2_config.h"
#include "../pda2_log.h"
#include <LittleFS.h>

// ════════════════════════════════════════════════════════
//  FsInternal (LittleFS) — общая для обеих плат
// ════════════════════════════════════════════════════════

bool FsInternal::begin() {
    _ok = LittleFS.begin(true);
    if (_ok)
        PDA_LOGI("fs.int", "LittleFS ok. Free: %u / %u",
                 LittleFS.totalBytes() - LittleFS.usedBytes(),
                 LittleFS.totalBytes());
    else
        PDA_LOGE("fs.int", "LittleFS mount failed");
    return _ok;
}

std::vector<FsEntry> FsInternal::ls(const char* path) {
    std::vector<FsEntry> result;
    if (!_ok) return result;
    File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) return result;
    File f = dir.openNextFile();
    while (f) {
        FsEntry e;
        e.name  = f.name();
        e.isDir = f.isDirectory();
        e.size  = e.isDir ? 0 : f.size();
        result.push_back(e);
        f = dir.openNextFile();
    }
    return result;
}

bool FsInternal::exists(const char* path) {
    return _ok && LittleFS.exists(path);
}

bool FsInternal::isDir(const char* path) {
    if (!_ok) return false;
    File f = LittleFS.open(path);
    return f && f.isDirectory();
}

bool FsInternal::mkdir(const char* path) {
    return _ok && LittleFS.mkdir(path);
}

String FsInternal::read(const char* path) {
    if (!_ok) return "";
    File f = LittleFS.open(path, "r");
    if (!f) return "";
    return f.readString();
}

bool FsInternal::readBytes(const char* path, uint8_t* buf, size_t len) {
    if (!_ok) return false;
    File f = LittleFS.open(path, "r");
    if (!f) return false;
    return f.read(buf, len) == len;
}

bool FsInternal::write(const char* path, const String& data) {
    if (!_ok) return false;
    File f = LittleFS.open(path, "w");
    if (!f) return false;
    return f.print(data) > 0;
}

bool FsInternal::writeBytes(const char* path, const uint8_t* buf, size_t len) {
    if (!_ok) return false;
    File f = LittleFS.open(path, "w");
    if (!f) return false;
    return f.write(buf, len) == len;
}

bool FsInternal::remove(const char* path) {
    return _ok && LittleFS.remove(path);
}

bool FsInternal::rename(const char* from, const char* to) {
    return _ok && LittleFS.rename(from, to);
}

bool FsInternal::copy(const char* from, const char* to) {
    if (!_ok) return false;
    File src = LittleFS.open(from, "r");
    if (!src) return false;
    File dst = LittleFS.open(to, "w");
    if (!dst) return false;
    uint8_t buf[256];
    size_t n;
    while ((n = src.read(buf, sizeof(buf))) > 0)
        dst.write(buf, n);
    return true;
}

size_t FsInternal::size(const char* path) {
    if (!_ok) return 0;
    File f = LittleFS.open(path, "r");
    return f ? f.size() : 0;
}

size_t FsInternal::dirSize(const char* path) {
    if (!_ok) return 0;
    size_t total = 0;
    File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) return 0;
    File f = dir.openNextFile();
    while (f) {
        if (f.isDirectory())
            total += dirSize(f.name());
        else
            total += f.size();
        f = dir.openNextFile();
    }
    return total;
}

size_t FsInternal::freeSpace() {
    return _ok ? (LittleFS.totalBytes() - LittleFS.usedBytes()) : 0;
}

size_t FsInternal::totalSpace() {
    return _ok ? LittleFS.totalBytes() : 0;
}

// ════════════════════════════════════════════════════════
//  FsSd (SD карта, отдельная SPI-шина — GPIO 6/7/21/42)
//  Только pda2 — на PDA2_LITE не разведена (см. pda2_config.h)
// ════════════════════════════════════════════════════════

#include <SD.h>
#include <SPI.h>

static SPIClass _spiSd(HSPI);

bool FsSd::begin() {
    _spiSd.begin(PDA2_PIN_SD_SCK, PDA2_PIN_SD_MISO,
                 PDA2_PIN_SD_MOSI, PDA2_PIN_SD_CS);
    _ok = SD.begin(PDA2_PIN_SD_CS, _spiSd);
    if (_ok)
        PDA_LOGI("fs.sd", "SD ok. Free: %u / %u",
                 SD.totalBytes() - SD.usedBytes(), SD.totalBytes());
    else
        PDA_LOGE("fs.sd", "SD mount failed");
    return _ok;
}

std::vector<FsEntry> FsSd::ls(const char* path) {
    std::vector<FsEntry> result;
    if (!_ok) return result;
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) return result;
    File f = dir.openNextFile();
    while (f) {
        FsEntry e;
        e.name  = f.name();
        e.isDir = f.isDirectory();
        e.size  = e.isDir ? 0 : f.size();
        result.push_back(e);
        f = dir.openNextFile();
    }
    return result;
}

bool FsSd::exists(const char* path) {
    return _ok && SD.exists(path);
}

bool FsSd::isDir(const char* path) {
    if (!_ok) return false;
    File f = SD.open(path);
    return f && f.isDirectory();
}

bool FsSd::mkdir(const char* path) {
    return _ok && SD.mkdir(path);
}

String FsSd::read(const char* path) {
    if (!_ok) return "";
    File f = SD.open(path, "r");
    if (!f) return "";
    return f.readString();
}

bool FsSd::readBytes(const char* path, uint8_t* buf, size_t len) {
    if (!_ok) return false;
    File f = SD.open(path, "r");
    if (!f) return false;
    return f.read(buf, len) == len;
}

bool FsSd::write(const char* path, const String& data) {
    if (!_ok) return false;
    File f = SD.open(path, "w");
    if (!f) return false;
    return f.print(data) > 0;
}

bool FsSd::writeBytes(const char* path, const uint8_t* buf, size_t len) {
    if (!_ok) return false;
    File f = SD.open(path, "w");
    if (!f) return false;
    return f.write(buf, len) == len;
}

bool FsSd::remove(const char* path) {
    return _ok && SD.remove(path);
}

bool FsSd::rename(const char* from, const char* to) {
    return _ok && SD.rename(from, to);
}

bool FsSd::copy(const char* from, const char* to) {
    if (!_ok) return false;
    File src = SD.open(from, "r");
    if (!src) return false;
    File dst = SD.open(to, "w");
    if (!dst) return false;
    uint8_t buf[256];
    size_t n;
    while ((n = src.read(buf, sizeof(buf))) > 0)
        dst.write(buf, n);
    return true;
}

size_t FsSd::size(const char* path) {
    if (!_ok) return 0;
    File f = SD.open(path, "r");
    return f ? f.size() : 0;
}

size_t FsSd::dirSize(const char* path) {
    if (!_ok) return 0;
    size_t total = 0;
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) return 0;
    File f = dir.openNextFile();
    while (f) {
        if (f.isDirectory())
            total += dirSize(f.name());
        else
            total += f.size();
        f = dir.openNextFile();
    }
    return total;
}

size_t FsSd::freeSpace() {
    return _ok ? (SD.totalBytes() - SD.usedBytes()) : 0;
}

size_t FsSd::totalSpace() {
    return _ok ? SD.totalBytes() : 0;
}

#endif // !PDA2_SIM