#pragma once

#include <stdint.h>
#include <string.h>

// ════════════════════════════════════════════════════════
//  PDA 2 — PdaStr32
//  Маленькая платформонезависимая строка фиксированного
//  размера — замена Arduino String там, где String попадал
//  в публичный интерфейс только ради коротких строк
//  (BT device name ≤31 байт, MAC-адрес "aa:bb:cc:dd:ee:ff").
//
//  Аудит реального использования (см. session doc) показал:
//  ни роста, ни конкатенации, ни heap нигде не требуется —
//  везде только конструктор из const char*, .c_str(), .length().
//  Поэтому здесь их нет. Один файл, ни ESP32-, ни sim-специфики —
//  компилируется одинаково на обеих платформах.
//
//  Обрезание длинных строк — молчаливое (без лога), см. Rule 12 —
//  логирование не было частью согласованного объёма.
// ════════════════════════════════════════════════════════

class PdaStr32 {
public:
    PdaStr32() { _buf[0] = '\0'; }
    PdaStr32(const char* s) { _set(s); }

    PdaStr32& operator=(const char* s) { _set(s); return *this; }

    const char* c_str() const { return _buf; }
    size_t      length() const { return strlen(_buf); }
    bool        empty() const { return _buf[0] == '\0'; }

private:
    static constexpr size_t CAP = 32;   // включая terminator — максимум 31 полезный байт
    char _buf[CAP];

    void _set(const char* s) {
        if (!s) { _buf[0] = '\0'; return; }
        strncpy(_buf, s, CAP - 1);
        _buf[CAP - 1] = '\0';
    }
};