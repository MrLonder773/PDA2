#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — pda2_log.h
//  Логирование через Serial. Serial.printf не используется
//  нигде в проекте — только эти макросы.
// ════════════════════════════════════════════════════════

#include "pda2_config.h"
#include <Arduino.h>

#if PDA2_LOG_LEVEL >= 1
  #define PDA_LOGE(tag, fmt, ...) \
    Serial.printf("[E][%lu][%s] " fmt "\n", millis(), tag, ##__VA_ARGS__)
#else
  #define PDA_LOGE(tag, fmt, ...) do {} while(0)
#endif

#if PDA2_LOG_LEVEL >= 2
  #define PDA_LOGW(tag, fmt, ...) \
    Serial.printf("[W][%lu][%s] " fmt "\n", millis(), tag, ##__VA_ARGS__)
#else
  #define PDA_LOGW(tag, fmt, ...) do {} while(0)
#endif

#if PDA2_LOG_LEVEL >= 3
  #define PDA_LOGI(tag, fmt, ...) \
    Serial.printf("[I][%lu][%s] " fmt "\n", millis(), tag, ##__VA_ARGS__)
#else
  #define PDA_LOGI(tag, fmt, ...) do {} while(0)
#endif

// Теги — использовать строго эти:
//   "core"     "display"   "touch"   "rtc"
//   "imu"      "fs"        "apps"    "prefs"
//   "clock"    "notes"     "files"   "accel"   "settings"