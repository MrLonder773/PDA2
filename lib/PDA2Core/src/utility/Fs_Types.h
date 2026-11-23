#pragma once
#include <cstdint>

enum class FsOpenMode : uint8_t { Read, Write, Append };   // Write обнуляет файл

struct FsResult {
    enum Code : uint8_t {
        Ok, NotMounted, BadPath, NotFound, IsDir, NotDir, Exists, TooBig,
        NoMemory, NoSpace, ShortRead, DepthLimit, Unsupported, Busy,
        Cancelled, IoError
    };
    Code code;
    FsResult(Code c = Ok) : code(c) {}
    operator bool() const { return code == Ok; }
    bool operator==(Code c) const { return code == c; }
    bool operator!=(Code c) const { return code != c; }
    const char* name() const {
        switch (code) {
            case Ok: return "Ok";               case NotMounted: return "NotMounted";
            case BadPath: return "BadPath";     case NotFound: return "NotFound";
            case IsDir: return "IsDir";         case NotDir: return "NotDir";
            case Exists: return "Exists";       case TooBig: return "TooBig";
            case NoMemory: return "NoMemory";   case NoSpace: return "NoSpace";
            case ShortRead: return "ShortRead"; case DepthLimit: return "DepthLimit";
            case Unsupported: return "Unsupported"; case Busy: return "Busy";
            case Cancelled: return "Cancelled"; default: return "IoError";
        }
    }
};