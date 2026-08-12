#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Prefs_Class.h
//  NVS-обёртка. Хранит настройки между перезагрузками.
// ════════════════════════════════════════════════════════
#include <Arduino.h>
#include <stdint.h>

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

     // Bluetooth 
    bool    getBtEnabled();
    void    setBtEnabled(bool val);
    String  getBtLastDevice();          // "aa:bb:cc:dd:ee:ff" (lowercase)
    void    setBtLastDevice(const String& mac);
    uint8_t getBtLastDeviceType();      // 0=public, 1=random (default 1)
    void    setBtLastDeviceType(uint8_t t);
    String getBtLastDeviceName();
    void   setBtLastDeviceName(const String& name);
};