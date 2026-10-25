#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Prefs_Class.h
//  NVS-обёртка. Хранит настройки между перезагрузками.
// ════════════════════════════════════════════════════════
#include <stdint.h>
#include "PdaStr32.h"

class Prefs_Class {
public:
    void    begin();

    // Яркость дисплея (0–255, default 200)
    uint8_t getBrightness();
    void    setBrightness(uint8_t v);

    // Ротация дисплея (0–3, default PDA2_ROTATION)
    uint8_t getRotation();
    void    setRotation(uint8_t r);

    // Тема (0=светлая 1=тёмная)
    uint8_t getTheme();
    void    setTheme(uint8_t t);

    // Таймаут сна в мс
    uint32_t getSleepTimeout();
    void     setSleepTimeout(uint32_t ms);

    // Последнее открытое приложение
    int8_t  getLastApp();
    void    setLastApp(int8_t id);

    // WiFi (default: true)
    bool    getWifi();
    void    setWifi(bool val);

    // WiFi STA / AP учётки
    PdaStr32 getWifiSSID();
    void     setWifiSSID(const PdaStr32& ssid);
    PdaStr32 getWifiPass();
    void     setWifiPass(const PdaStr32& pass);
    PdaStr32 getApSSID();
    void     setApSSID(const PdaStr32& ssid);
    PdaStr32 getApPass();
    void     setApPass(const PdaStr32& pass);

    // Bluetooth
    bool    getBtEnabled();
    void    setBtEnabled(bool val);
    PdaStr32 getBtLastDevice();          // "aa:bb:cc:dd:ee:ff" (lowercase)
    void    setBtLastDevice(const PdaStr32& mac);
    uint8_t getBtLastDeviceType();      // 0=public, 1=random (default 1)
    void    setBtLastDeviceType(uint8_t t);
    PdaStr32 getBtLastDeviceName();
    void     setBtLastDeviceName(const PdaStr32& name);

    // Магнитометр: hard-iron offset (сырые коды QMC5883L)
    bool    getMagOffset(int16_t out[3]);        // false — не сохранён, out не тронут
    void    setMagOffset(const int16_t in[3]);
    void    clearMagOffset();

    // Давление на уровне моря для абсолютной высоты, гПа (default 1013.25)
    float   getQnh();
    void    setQnh(float hpa);

    // Часовой пояс (используется Rtc_Class): стандартное смещение от UTC, минуты
    // (default 120 = Киев) и правило перехода на летнее время (0=OFF, 1=EU; default 1)
    int16_t getTzOffset();
    void    setTzOffset(int16_t minutes);
    uint8_t getDstRule();
    void    setDstRule(uint8_t rule);
};