#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Fs_Class.h
//  SPIFFS-обёртка.
// ════════════════════════════════════════════════════════

#include <stddef.h>

class Fs_Class {
public:
    void begin();
    bool ok();

    size_t totalBytes();
    size_t usedBytes();
    size_t freeBytes();

private:
    bool _ok = false;
};