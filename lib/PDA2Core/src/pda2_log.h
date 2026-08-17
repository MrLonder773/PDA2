#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — pda2_log.h
//  Логирование через Serial. Serial.printf не используется
//  нигде в проекте — только эти макросы.
// ════════════════════════════════════════════════════════

#include "pda2_config.h"
#include "pda2_platform.h"

#ifndef PDA2_SIM
  #include <Arduino.h>
  #define PDA2_LOG_PRINTF(fmt, ...) \
    Serial.printf(fmt, ##__VA_ARGS__)
#else
  #include <cstdio>
  #define PDA2_LOG_PRINTF(fmt, ...) \
    printf(fmt, ##__VA_ARGS__)
#endif

#if PDA2_LOG_LEVEL >= 1
  #define PDA_LOGE(tag, fmt, ...) \
    PDA2_LOG_PRINTF("[E][%lu][%s] " fmt "\n", (unsigned long)pda2_platform_now_ms(), tag, ##__VA_ARGS__)
#else
  #define PDA_LOGE(tag, fmt, ...) do {} while(0)
#endif

#if PDA2_LOG_LEVEL >= 2
  #define PDA_LOGW(tag, fmt, ...) \
    PDA2_LOG_PRINTF("[W][%lu][%s] " fmt "\n", (unsigned long)pda2_platform_now_ms(), tag, ##__VA_ARGS__)
#else
  #define PDA_LOGW(tag, fmt, ...) do {} while(0)
#endif

#if PDA2_LOG_LEVEL >= 3
  #define PDA_LOGI(tag, fmt, ...) \
    PDA2_LOG_PRINTF("[I][%lu][%s] " fmt "\n", (unsigned long)pda2_platform_now_ms(), tag, ##__VA_ARGS__)
#else
  #define PDA_LOGI(tag, fmt, ...) do {} while(0)
#endif

// Теги — использовать строго эти:
//   "core"     "display"   "touch"   "rtc"
//   "imu"      "fs"        "apps"    "prefs"
//   "clock"    "notes"     "files"   "accel"   "settings"
//   "qpanel"   "espnow"    "audio"   "walkie"