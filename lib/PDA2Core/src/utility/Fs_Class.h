#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_Class.h
//  LittleFS (internal) + SD. Единая точка доступа к ФС.
//  Логика одна на обе платформы (Fs_Volume.cpp), платформа даёт
//  только FsBackend (см. Fs_Backend.h).
//  Результат операций — FsResult (Fs_Types.h): приводится к bool
//  (true = Ok), причина — через == FsResult::X или name().
// ════════════════════════════════════════════════════════

#include <memory>
#include <string>
#include <vector>
#include "Fs_Types.h"

#ifdef PDA2_SIM
    #include <cstdint>
    using String = std::string;
#else
    #include <Arduino.h>
#endif

class Prefs_Class;
class FsBackend;
class FsBackendFile;
class FsVolume;

struct FsEntry {
    String name;
    bool   isDir;
    size_t size;    // 0 для папок, используй dirSize() если нужен размер
};

// Потоковый файл (RAII). Открывается через FsSource::open(); закрывается
// в деструкторе. Каждый вызов короткий: большой файл читают кусками из onTick().
class FsStream {
public:
    FsStream();
    ~FsStream();
    FsStream(FsStream&&) noexcept;
    FsStream& operator=(FsStream&&) noexcept;
    FsStream(const FsStream&)            = delete;
    FsStream& operator=(const FsStream&) = delete;

    bool       isOpen() const { return (bool)_f; }
    // got < len — конец файла, не ошибка
    FsResult   read(uint8_t* buf, size_t len, size_t& got);
    FsResult   write(const uint8_t* buf, size_t len);
    FsResult   seek(uint64_t pos);          // Unsupported в режиме Append
    uint64_t   tell();
    uint64_t   size();
    FsResult   close();                     // повторный вызов безвреден

private:
    friend class FsVolume;
    std::unique_ptr<FsBackendFile> _f;
    FsOpenMode _mode;
    FsBackend* _be;
};

class FsSource {
public:
    virtual bool   begin()                                        = 0;
    virtual bool   ok() const                                     = 0;

    virtual std::vector<FsEntry> ls(const char* path)            = 0;   // по имени, побайтно
    virtual bool   exists(const char* path)                       = 0;
    virtual bool   isDir(const char* path)                        = 0;
    virtual FsResult mkdir(const char* path)                      = 0;   // создаёт и недостающие родительские папки

    // Целиком в память: лимит PDA2_FS_READ_MAX (TooBig), резерв размера заранее.
    // Пустой файл — Ok и пустая строка; ошибка отличима от данных.
    virtual FsResult read(const char* path, String& out)          = 0;
    // Старый вид: при любой ошибке "". Причину не даёт — новый код пользуется read(path, out).
    virtual String read(const char* path)                         = 0;
    // len байт с начала файла; файл короче — ShortRead
    virtual FsResult readBytes(const char* path,
                               uint8_t* buf, size_t len)          = 0;
    virtual FsResult readBytes(const char* path,
                               uint8_t* buf, size_t len,
                               uint64_t offset)                   = 0;
    virtual FsResult write(const char* path, const String& data)  = 0;
    virtual FsResult writeBytes(const char* path,
                                const uint8_t* buf, size_t len)   = 0;
    virtual FsResult append(const char* path, const String& data) = 0;   // файл создаётся, если его нет

    virtual FsResult remove(const char* path)                     = 0;   // только файл; папки удаляет removeAll
    virtual FsResult removeAll(const char* path)                  = 0;   // папка с содержимым; "/" отклоняется
    virtual FsResult rename(const char* from, const char* to)     = 0;
    virtual FsResult copy(const char* from, const char* to)       = 0;

    // Поток. Write обнуляет файл, Append дописывает (seek там не поддерживается).
    virtual FsResult open(const char* path, FsOpenMode mode,
                          FsStream& out)                          = 0;

    // Рекурсивный поиск по имени: подстрока, регистр сворачивается только для ASCII.
    // Не больше PDA2_FS_FIND_MAX результатов, глубина до PDA2_FS_MAX_DEPTH.
    // truncated (необязателен) = true, если упёрлись в лимит (при ровно MAX возможен ложный true).
    virtual FsResult find(const char* root, const char* name,
                          std::vector<String>& out,
                          bool* truncated = nullptr)              = 0;
    virtual std::vector<String> find(const char* root, const char* name) = 0;   // старый вид

    virtual size_t size(const char* path)                         = 0;
    virtual uint64_t dirSize(const char* path)                    = 0;
    virtual uint64_t freeSpace()                                  = 0;
    virtual uint64_t totalSpace()                                 = 0;
};

// ── Том: вся логика поверх FsBackend (Fs_Volume.cpp) ────

class FsVolume : public FsSource {
public:
    bool   ok() const                                     override { return _ok; }

    std::vector<FsEntry> ls(const char* path)            override;
    bool   exists(const char* path)                       override;
    bool   isDir(const char* path)                        override;
    FsResult mkdir(const char* path)                      override;

    FsResult read(const char* path, String& out)          override;
    String   read(const char* path)                       override;
    FsResult readBytes(const char* path,
                       uint8_t* buf, size_t len)          override;
    FsResult readBytes(const char* path,
                       uint8_t* buf, size_t len,
                       uint64_t offset)                   override;
    FsResult write(const char* path, const String& data)  override;
    FsResult writeBytes(const char* path,
                        const uint8_t* buf, size_t len)   override;
    FsResult append(const char* path, const String& data) override;

    FsResult remove(const char* path)                     override;
    FsResult removeAll(const char* path)                  override;
    FsResult rename(const char* from, const char* to)     override;
    FsResult copy(const char* from, const char* to)       override;

    FsResult open(const char* path, FsOpenMode mode,
                  FsStream& out)                          override;

    FsResult find(const char* root, const char* name,
                  std::vector<String>& out,
                  bool* truncated = nullptr)              override;
    std::vector<String> find(const char* root, const char* name) override;

    size_t size(const char* path)                         override;
    uint64_t dirSize(const char* path)                    override;
    uint64_t freeSpace()                                  override;
    uint64_t totalSpace()                                 override;

    // Лимит read(path, out); 0 = PDA2_FS_READ_MAX. Нужен самотесту.
    void   setReadLimit(size_t n) { _readMax = n; }

protected:
    // том смонтирован и путь допустим; out — нормализованный путь
    bool     _path(const char* in, std::string& out) const;
    FsResult _prep(const char* in, std::string& out) const;   // NotMounted / BadPath / Ok

    bool       _ok = false;
    FsBackend* _be = nullptr;   // задаёт begin() в _esp32/_sim
    size_t     _readMax = 0;
};

// ── LittleFS ────────────────────────────────────────────

class FsInternal : public FsVolume {
public:
    bool   begin()                                        override;   // = begin(true)
    // allowFormat=false: при неудачном mount флеш не трогаем, ok()=false
    bool   begin(bool allowFormat);
    // Только при ok()==false: форматирует и монтирует заново. Иначе false.
    bool   format();

    // true, если при старте раздел не смонтировался и был отформатирован (данные потеряны)
    bool   wasFormatted() const { return _formatted; }

private:
    bool _formatted = false;
};

// ── SD карта ────────────────────────────────────────────

class FsSd : public FsVolume {
public:
    bool   begin()                                        override;
};

// ── Fs_Class ────────────────────────────────────────────

class Fs_Class {
public:
    FsInternal internal;
    FsSd       sd;

    // Форматирование LittleFS при сбое mount разрешено только до первого
    // успешного монтирования (флаг в Prefs).
    void begin(Prefs_Class& prefs);
    bool sdAvailable();
};