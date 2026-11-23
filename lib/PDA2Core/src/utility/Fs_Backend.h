#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_Backend.h
//  Крошечный платформенный слой под Fs_Volume.cpp.
//  Backend не принимает решений: ни рекурсии, ни лимитов, ни
//  нормализации. Все пути приходят уже нормализованными
//  (Fs_Path.h): ведущий '/', без '//', без хвостового '/'.
//  Реализации: Fs_Class_esp32.cpp (fs::FS), Fs_Class_sim.cpp (std::filesystem).
// ════════════════════════════════════════════════════════

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "Fs_Types.h"

struct FsBackendEntry {
    std::string name;      // только имя, без пути
    bool        isDir;
    uint64_t    size;      // 0 для папок
};



class FsBackendFile {
public:
    virtual ~FsBackendFile() {}
    virtual size_t   read(uint8_t* buf, size_t len)        = 0;   // 0 = конец или ошибка
    virtual size_t   write(const uint8_t* buf, size_t len) = 0;   // сколько записано
    virtual uint64_t size()                                = 0;
    virtual bool     close()                               = 0;   // повторный вызов безвреден; false — запись не дошла
    virtual bool     seek(uint64_t pos) = 0;   // false: не поддерживается (Append) или ошибка
    virtual uint64_t tell()             = 0;
};

class FsBackend {
public:
    virtual ~FsBackend() {}

    // false, если пути нет
    virtual bool stat(const std::string& path, bool& isDir, uint64_t& size) = 0;
    // дети одной папки; false, если это не папка
    virtual bool list(const std::string& dir, std::vector<FsBackendEntry>& out) = 0;
    // одна папка; родитель уже существует
    virtual bool makeDir(const std::string& path)    = 0;
    virtual bool removeFile(const std::string& path) = 0;   // только файл
    virtual bool removeDir(const std::string& path)  = 0;   // только пустую папку
    virtual bool rename(const std::string& from, const std::string& to) = 0;
    // nullptr, если не открылось или это папка
    virtual std::unique_ptr<FsBackendFile> open(const std::string& path, FsOpenMode mode) = 0;

    virtual uint64_t freeSpace()  = 0;
    virtual uint64_t totalSpace() = 0;
};