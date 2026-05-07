#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — pda2_config.h
//  Все пины, адреса и константы ТОЛЬКО здесь.
//  При переносе на другое железо — меняешь один этот файл.
// ════════════════════════════════════════════════════════

// ── Дисплей (SPI) ───────────────────────────────────────
#define PDA2_PIN_CS      10
#define PDA2_PIN_RST      8
#define PDA2_PIN_DC       9
#define PDA2_PIN_MOSI    11
#define PDA2_PIN_SCK     12
#define PDA2_PIN_BL      46
#define PDA2_SPI_FREQ    79000000   // 79 MHz
#define PDA2_SPI_FREQ_BL 50000

// ── I2C (общая шина) ────────────────────────────────────
#define PDA2_PIN_SDA      4
#define PDA2_PIN_SCL      5

// ── Touch (FT6236) ──────────────────────────────────────
#define PDA2_PIN_TOUCH_INT  3
#define PDA2_PIN_TOUCH_RST  2
#define PDA2_I2C_TOUCH   0x38

// ── RTC (DS3231) ────────────────────────────────────────
#define PDA2_I2C_RTC     0x68

// ── IMU (BMI160) ────────────────────────────────────────
#define PDA2_I2C_IMU     0x69

// ── Экран ───────────────────────────────────────────────
#define PDA2_SCREEN_W     320
#define PDA2_SCREEN_H     480
#define PDA2_ROTATION       2       // 0-3, default landscape

// ── UI / Apps ───────────────────────────────────────────
#define PDA2_MAX_APPS      16
#define PDA2_ANIM_MS      240       // длительность анимации перехода
#define PDA2_GRID_COLS      4
#define PDA2_ICON_SIZE     60

// ── Настройки из приложений ─────────────────────────────

// NotesApp
#define PDA2_NOTES_MAX_SIZE   8192   // макс. размер одной заметки в байтах
#define PDA2_NOTES_MAX_FILES  20     // макс. файлов в роллере

// AccelApp
#define PDA2_ACCEL_CUBE_SIZE   70        // полуразмер куба, px (unit → px)
#define PDA2_ACCEL_CUBE_CX    160        // центр X canvas, px
#define PDA2_ACCEL_CUBE_CY    155        // центр Y canvas, px
#define PDA2_ACCEL_FOCAL      320        // перспектива, px
#define PDA2_ACCEL_AXIS_LEN    90        // длина линий осей, px
#define PDA2_ACCEL_TICK_MS     50        // интервал перерисовки, мс
#define PDA2_ACCEL_BG_COLOR  0x0d1117   // фон canvas

#define PDA2_ACCEL_GYRO_DEADZONE  1.5f  // °/s — ниже этого считаем покоем
#define PDA2_ACCEL_GYRO_SIGN_X   (-1)   // +1 или -1
#define PDA2_ACCEL_GYRO_SIGN_Y   (-1)
#define PDA2_ACCEL_GYRO_SIGN_Z   (-1)
#define PDA2_ACCEL_GYRO_MAP_X     2     // 0=gx, 1=gy, 2=gz
#define PDA2_ACCEL_GYRO_MAP_Y     0
#define PDA2_ACCEL_GYRO_MAP_Z     1

// ── Touch жесты ─────────────────────────────────────────
// Свайп-home: касание ниже PDA2_HOME_SWIPE_PCT% высоты экрана.
// 75 = нижние 25%. Работает корректно в portrait и landscape.
#define PDA2_HOME_SWIPE_PCT  75

// Launcher режим: PDA2_LAUNCHER_GRID или PDA2_LAUNCHER_CAROUSEL
#define PDA2_LAUNCHER_GRID      0
#define PDA2_LAUNCHER_CAROUSEL  1
#define PDA2_LAUNCHER_MODE      PDA2_LAUNCHER_GRID

// ── Тема ────────────────────────────────────────────────
#define PDA2_THEME_DARK         0   // 0=светлая 1=тёмная

// ── Система ─────────────────────────────────────────────
#define PDA2_SLEEP_TIMEOUT_MS   300000   // 5 минут
#define PDA2_DEBUG_OVERLAY      0        // 0=выкл 1=вкл (FPS и память)
#define PDA2_MAX_APPS 16

// ── Логирование ─────────────────────────────────────────
// 0=выкл  1=error  2=warn  3=info
#define PDA2_LOG_LEVEL          3

// ── NVS ─────────────────────────────────────────────────
#define PDA2_NVS_NS    "pda2"      // namespace в NVS