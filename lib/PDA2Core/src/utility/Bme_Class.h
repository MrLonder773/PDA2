#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Bme_Class.h
//  BME280 (температура, влажность, давление) + высота.
//  Raw I2C, без внешних библиотек. BMP280 (без влажности) не поддерживается.
//
//  Использование: PDA.update() вызывает update() каждый тик,
//  сам класс опрашивает шину не чаще 1 Гц. Приложение читает
//  get()/altitudeRel()/altitudeAbs() — это кэш, шину они не трогают.
//
//  Высота — два независимых метода:
//    altitudeRel() — от точки zeroHere() (нуль только в RAM)
//    altitudeAbs() — над уровнем моря по давлению QNH (хранится в Prefs)
// ════════════════════════════════════════════════════════

#include <stdint.h>

struct pda2_env_t {
    float temp_c;       // °C
    float hum_pct;      // %RH
    float press_hpa;    // гПа
    bool  valid;        // false: чтение не удалось или ещё не было.
                        // Поля тогда — последние удачные значения (или 0).
};

class Bme_Class {
public:
    void        begin();                // ошибка — в лог, загрузку не блокирует
    void        update();               // каждый тик; шину читает не чаще 1 Гц

    bool        ok() const;             // BME280 найден и настроен
    pda2_env_t  get() const;            // последний снимок из кэша, проверять .valid

    // Относительная высота (от точки «нуль здесь»)
    bool  zeroHere();                   // текущее давление = 0 м; true: нуль задан,
                                        // false: нет валидного давления, нуль не менялся
    void  zeroClear();
    bool  zeroSet() const;
    float altitudeRel() const;          // м; NAN, если !valid или нуль не задан

    // Абсолютная высота (над уровнем моря по QNH)
    bool  setQnh(float hpa);            // давление, приведённое к уровню моря; true: принято и сохранено
                                        // (допустимо 850..1100 гПа, иначе false и значение не меняется)
    bool  setQnhFromAltitude(float meters); // QNH по известной высоте текущего места: берёт текущее
                                        // давление; true: принято и сохранено, false: нет валидного
                                        // давления / результат вне 850..1100 гПа (значение не меняется)
    float qnh() const;                  // по умолчанию 1013.25 (стандартная атмосфера)
    float altitudeAbs() const;          // м; NAN, если !valid

private:
    static constexpr float    QNH_DEFAULT      = 1013.25f;
    static constexpr float    QNH_MIN          = 850.0f;
    static constexpr float    QNH_MAX          = 1100.0f;
    static constexpr float    PRESS_MIN        = 300.0f;    // правдоподобный диапазон давления, гПа
    static constexpr float    PRESS_MAX        = 1100.0f;
    static constexpr uint32_t UPDATE_PERIOD_MS = 1000;

    bool       _ok = false;
    pda2_env_t _snap = {};
    float      _qnh = QNH_DEFAULT;
    bool       _zero_set = false;
    float      _zero_hpa = 0.0f;

    bool       _polled = false;             // троттлинг (используют _esp32/_sim)
    uint32_t   _last_ms = 0;

    // Общая логика (Bme_Class.cpp) — вызывается из _esp32/_sim
    void _set_env(float temp_c, float hum_pct, float press_hpa);
    void _env_failed();
    void _load_qnh();
    static float _altitude(float p, float p0);
};