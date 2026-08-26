#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — pda2_config.h
//  Все пины, адреса и константы ТОЛЬКО здесь.
//
//  Архитектура v2.0.14:
//    ESP32-S3  — главный чип (UI, логика, приложения)
//    ESP32 WROOM — со-процессор (nRF24, SIM800L, IR RX)
//    Связь: UART (GPIO 38/39 на S3 / 16-17 на WROOM),
//           пакетный протокол 0xAA...CRC8
//
//  PDA2_LITE (v1.0) — вариант на ESP32-S3 без WROOM-моста,
//    ST7735 128x160 вместо ILI9488, EC11-энкодер вместо тача,
//    nRF24 напрямую на S3. См. #ifdef PDA2_LITE ниже.
// ════════════════════════════════════════════════════════

// ── Класс устройства ────────────────────────────────────
#define PDA2_CLASS_DEVELOPER    0
#define PDA2_CLASS_COMMUNICATOR 1
#define PDA2_CLASS_FIELD        2
#define PDA2_CLASS_MINIMAL      3
#define PDA2_DEVICE_CLASS       PDA2_CLASS_DEVELOPER

#define PDA2_VERSION "3.0.0"

// ── Дисплей (SPI) ─────────────────────────────────────────
  #define PDA2_PIN_CS      10    // дисплей CS
  #define PDA2_PIN_RST      8
  #define PDA2_PIN_DC       9
  #define PDA2_PIN_MOSI    11
  #define PDA2_PIN_SCK     12
  #define PDA2_PIN_BL      46
  #define PDA2_SPI_FREQ    79000000
  #define PDA2_SPI_FREQ_BL 50000

// ── SD карта (только pda2, отдельная SPI-шина) ───────────
// Раньше shared с дисплеем (MOSI=11/SCK=12) — конфликтовало с
// LovyanGFX (bus_shared=false, отдельный SPI2_HOST). Теперь своя шина.
  #define PDA2_PIN_SD_MOSI  6
  #define PDA2_PIN_SD_SCK   7
  #define PDA2_PIN_SD_MISO 21
  #define PDA2_PIN_SD_CS   42
// на Lite SD не разведена — GPIO6/7/21/42 заняты под I2S/энкодер/дисплей

// ── I2C — на Lite отсутствует (RTC/IMU/Touch пока не решены) ──
  #define PDA2_PIN_SDA      4
  #define PDA2_PIN_SCL      5

  // ── Touch (FT6236) ──────────────────────────────────────
  #define PDA2_PIN_TOUCH_INT  3
  #define PDA2_PIN_TOUCH_RST  2
  #define PDA2_I2C_TOUCH   0x38

  // ── RTC / IMU / BME280 / INA219 ─────────────────────────
  #define PDA2_I2C_RTC     0x68
  #define PDA2_I2C_IMU     0x69
  #define PDA2_I2C_BME280  0x76
  #define PDA2_I2C_INA219  0x40

// ── I2S Микрофон / Динамик ──────────────────────────────
  #define PDA2_PIN_I2S_MIC_WS      13
  #define PDA2_PIN_I2S_MIC_SCK     14
  #define PDA2_PIN_I2S_MIC_SD      15
  #define PDA2_PIN_I2S_SPK_WS      16
  #define PDA2_PIN_I2S_SPK_BCK     17
  #define PDA2_PIN_I2S_SPK_DATA    18

// ── PTT ─────────────────────────────────────────────────
#define PDA2_PIN_PTT      1   // совпадает в обеих версиях

// ── GPS NEO-6M (UART2) — только pda2 ────────────────────
  #define PDA2_PIN_GPS_RX  40
  #define PDA2_PIN_GPS_TX  41
// на Lite GPIO40/41 заняты под Display RST/DC — GPS не разведён

// ── IR TX — только pda2 ─────────────────────────────────
  #define PDA2_PIN_IR_TX   47
// на Lite GPIO47 занят под Encoder DT — IR не разведён
// IR RX → на WROOM (только pda2)

// ── nRF24 ────────────────────────────────────────────────
  // на Lite — напрямую на S3, WROOM-моста нет
  #define PDA2_PIN_NRF_CE    46
  #define PDA2_PIN_NRF_CSN    7
  #define PDA2_PIN_NRF_SCK   18
  #define PDA2_PIN_NRF_MISO   3
  #define PDA2_PIN_NRF_MOSI   8
// на pda2 (S3) nRF24 живёт на WROOM — своих пинов здесь нет

// ── SIM800L → на WROOM (Фаза 3, далёкий ящик, пины TBD) ──

// ── WROOM со-процессор (UART1) — только pda2 ─────────────
  #define PDA2_PIN_WROOM_TX   38    // S3 → WROOM
  #define PDA2_PIN_WROOM_RX   39    // WROOM → S3
  #define PDA2_WROOM_BAUD     460800

// ── Экран ───────────────────────────────────────────────
  #define PDA2_SCREEN_W     320
  #define PDA2_SCREEN_H     480
  #define PDA2_ROTATION       2

// ── UI / Apps ───────────────────────────────────────────
#define PDA2_MAX_APPS      16
#define PDA2_ANIM_MS      240
#define PDA2_GRID_COLS      4    // используется только в GRID-режиме (pda2)
#define PDA2_ICON_SIZE     60    // используется только в GRID-режиме (pda2)
// Lite (carousel) размеры иконок пока не заведены — понадобятся
// когда дойдём до Apps_Class carousel-реализации.

// ── NotesApp ────────────────────────────────────────────
#define PDA2_NOTES_MAX_SIZE   8192
#define PDA2_NOTES_MAX_FILES  20

// ── AccelApp canvas (только pda2 — AccelApp требует IMU, на Lite не заведён) ──
  #define PDA2_ACCEL_CANVAS_W   280
  #define PDA2_ACCEL_CANVAS_H   370
  #define PDA2_ACCEL_CANVAS_X   20
  #define PDA2_ACCEL_CANVAS_Y   10
  #define PDA2_ACCEL_STRIP_H    100
  #define PDA2_ACCEL_CUBE_CX    140
  #define PDA2_ACCEL_CUBE_CY    185
  #define PDA2_ACCEL_CUBE_SIZE   80
  #define PDA2_ACCEL_BG_COLOR    0x0f172a
  #define PDA2_ACCEL_FOCAL       300
  #define PDA2_ACCEL_AXIS_LEN    96
  #define PDA2_ACCEL_TICK_MS     33
  #define PDA2_ACCEL_GYRO_DEADZONE  1.5f
  #define PDA2_ACCEL_GYRO_SIGN_X   (-1)
  #define PDA2_ACCEL_GYRO_SIGN_Y   (-1)
  #define PDA2_ACCEL_GYRO_SIGN_Z   (-1)
  #define PDA2_ACCEL_GYRO_MAP_X     0
  #define PDA2_ACCEL_GYRO_MAP_Y     2
  #define PDA2_ACCEL_GYRO_MAP_Z     1

// ── Launcher ──────────────────────────────────────────────
#define PDA2_LAUNCHER_GRID        0
#define PDA2_LAUNCHER_CAROUSEL    1
#define PDA2_LAUNCHER_MODE   PDA2_LAUNCHER_GRID

// ── Touch жесты — только pda2 ───────────────────────────
#define PDA2_HOME_SWIPE_PCT      75

// ── Тема / Система ───────────────────────────────────────
#define PDA2_THEME_DARK           0
#define PDA2_THEME_LIGHT          1
#define PDA2_SLEEP_TIMEOUT_MS   300000
#define PDA2_DEBUG_OVERLAY            0

// ── Симулятор ─────────────────────────────────────────────
#define PDA2_SIM_SD_AVAILABLE 1


// ── Quick Panel ───────────────────────────────────────────
#define PDA2_NOTIF_MAX            8
#define PDA2_QP_SWIPE_TOP_PCT    15   // тач-жест, на Lite не используется
#define PDA2_QP_NOTIF_H         360
#define PDA2_QP_SETTINGS_H      200

// Lite QuickPanel — отдельный экран через Launcher (не оверлей-шторка),
// свои размеры под 128x160 заведём когда дойдём до реализации.
#define PDA2_QP_ANIM_MS         220

// ── Логирование / NVS ─────────────────────────────────────
#define PDA2_LOG_LEVEL            3 
#define PDA2_NVS_NS    "pda2"

