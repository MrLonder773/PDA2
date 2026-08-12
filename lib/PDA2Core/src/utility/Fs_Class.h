#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_Class.h
//  LittleFS (internal) + SD. Единая точка доступа к ФС.
// ════════════════════════════════════════════════════════

#include <Arduino.h>
#include <vector>

struct FsEntry {
    String name;
    bool   isDir;
    size_t size;    // 0 для папок, используй dirSize() если нужен размер
};

class FsSource {
public:
    virtual bool   begin()                                        = 0;
    virtual bool   ok() const                                     = 0;

    virtual std::vector<FsEntry> ls(const char* path)            = 0;
    virtual bool   exists(const char* path)                       = 0;
    virtual bool   isDir(const char* path)                        = 0;
    virtual bool   mkdir(const char* path)                        = 0;

    virtual String read(const char* path)                         = 0;
    virtual bool   readBytes(const char* path,
                             uint8_t* buf, size_t len)            = 0;
    virtual bool   write(const char* path, const String& data)   = 0;
    virtual bool   writeBytes(const char* path,
                              const uint8_t* buf, size_t len)     = 0;

    virtual bool   remove(const char* path)                       = 0;
    virtual bool   rename(const char* from, const char* to)       = 0;
    virtual bool   copy(const char* from, const char* to)         = 0;

    virtual size_t size(const char* path)                         = 0;
    virtual size_t dirSize(const char* path)                      = 0;
    virtual size_t freeSpace()                                    = 0;
    virtual size_t totalSpace()                                   = 0;
};

// ── LittleFS ────────────────────────────────────────────

class FsInternal : public FsSource {
public:
    bool   begin()                                        override;
    bool   ok() const                                     override { return _ok; }

    std::vector<FsEntry> ls(const char* path)            override;
    bool   exists(const char* path)                       override;
    bool   isDir(const char* path)                        override;
    bool   mkdir(const char* path)                        override;

    String read(const char* path)                         override;
    bool   readBytes(const char* path,
                     uint8_t* buf, size_t len)            override;
    bool   write(const char* path, const String& data)   override;
    bool   writeBytes(const char* path,
                      const uint8_t* buf, size_t len)     override;

    bool   remove(const char* path)                       override;
    bool   rename(const char* from, const char* to)       override;
    bool   copy(const char* from, const char* to)         override;

    size_t size(const char* path)                         override;
    size_t dirSize(const char* path)                      override;
    size_t freeSpace()                                    override;
    size_t totalSpace()                                   override;

private:
    bool _ok = false;
};

// ── SD карта ────────────────────────────────────────────

class FsSd : public FsSource {
public:
    bool   begin()                                        override;
    bool   ok() const                                     override { return _ok; }

    std::vector<FsEntry> ls(const char* path)            override;
    bool   exists(const char* path)                       override;
    bool   isDir(const char* path)                        override;
    bool   mkdir(const char* path)                        override;

    String read(const char* path)                         override;
    bool   readBytes(const char* path,
                     uint8_t* buf, size_t len)            override;
    bool   write(const char* path, const String& data)   override;
    bool   writeBytes(const char* path,
                      const uint8_t* buf, size_t len)     override;

    bool   remove(const char* path)                       override;
    bool   rename(const char* from, const char* to)       override;
    bool   copy(const char* from, const char* to)         override;

    size_t size(const char* path)                         override;
    size_t dirSize(const char* path)                      override;
    size_t freeSpace()                                    override;
    size_t totalSpace()                                   override;

private:
    bool _ok = false;
};

// ── Fs_Class ────────────────────────────────────────────

class Fs_Class {
public:
    FsInternal internal;
    FsSd       sd;

    void begin();
    bool sdAvailable();
};