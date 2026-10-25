#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — pda2_config.h
//  Все пины, адреса и константы ТОЛЬКО здесь.
//
//  Архитектура (актуально): ESP32-S3-WROOM-1 N16R8 — единственный чип.
//  Со-процессор ESP32 WROOM-32E полностью удалён (UART-мост более не используется).
//
//  PDA2_LITE — отдельный репозиторий, отдельный pda2_config.h. Не смешивать.
// ════════════════════════════════════════════════════════

// ── Класс устройства ────────────────────────────────────
#define PDA2_CLASS_DEVELOPER    0
#define PDA2_DEVICE_CLASS       PDA2_CLASS_DEVELOPER

#define PDA2_VERSION "3.0.0"   // версия либы PDA2Core / релиза устройства, отдельно от версии доки

// ── Дисплей (SPI2/FSPI) ──────────────────────────────────
  #define PDA2_PIN_CS      10
  #define PDA2_PIN_RST      8
  #define PDA2_PIN_DC       9
  #define PDA2_PIN_MOSI    11
  #define PDA2_PIN_SCK     12
  #define PDA2_PIN_BL      46
  #define PDA2_SPI_FREQ    80000000
  #define PDA2_SPI_FREQ_BL 50000

// ── SD карта (отдельная SPI-шина, не протестирована на железе) ──
  #define PDA2_PIN_SD_MOSI  6
  #define PDA2_PIN_SD_SCK   7
  #define PDA2_PIN_SD_MISO 21
  #define PDA2_PIN_SD_CS   42

// ── I2C ───────────────────────────────────────────────────
  #define PDA2_PIN_SDA      4
  #define PDA2_PIN_SCL      5

  // ── Touch (FT6236) ──────────────────────────────────────
  #define PDA2_PIN_TOUCH_INT  3
  #define PDA2_PIN_TOUCH_RST  2
  #define PDA2_I2C_TOUCH   0x38

  // ── RTC / IMU / Компас / BME280 / INA219 ────────────────
  #define PDA2_I2C_RTC     0x68
  #define PDA2_I2C_IMU     0x69
  #define PDA2_I2C_COMPASS 0x0D   // это магнитометр, компас отдельно от акселеометра с гироскопом >w<
  #define PDA2_I2C_BME280  0x76
  #define PDA2_I2C_INA219  0x40

// ── I2S Микрофон / Динамик ──────────────────────────────
  #define PDA2_PIN_I2S_MIC_WS      13
  #define PDA2_PIN_I2S_MIC_SCK     14
  #define PDA2_PIN_I2S_MIC_SD      15
  #define PDA2_PIN_I2S_SPK_WS      16
  #define PDA2_PIN_I2S_SPK_BCK     17
  #define PDA2_PIN_I2S_SPK_DATA    18
  // текущая плата: MAX98357A. Новая плата: PCM5102A → PAM8403 (моно, один канал).

// ── Громкость (глобальные кнопки, закреплено) ────────────
#define PDA2_PIN_VOL_UP     1   // бывший PTT (GPIO1), переиспользован
#define PDA2_PIN_VOL_DOWN   0   // GPIO0 / BOOT, переиспользован как runtime-кнопка после загрузки

// ── ESP32-CAM (UART-мост к OV2640) — зарезервировано, не распаяно ──
#define PDA2_PIN_CAM_UART_TX   38
#define PDA2_PIN_CAM_UART_RX   39

// ── Кнопка питания (Pololu Mini Pushbutton Power Switch LV) ─────
// Схема: узел "A" свитча -> кнопка -> GND (on-only), тот же узел "A"
// параллельно заведён на PWR_SENSE. Прошивка при нажатии сама
// готовит выключение и импульсом на PWR_OFF гасит VOUT.
#define PDA2_PIN_PWR_SENSE   40   // вход, чтение нажатий (узел "A" свитча)
#define PDA2_PIN_PWR_OFF     41   // выход, импульс = мягкое выключение

// ── GPIO45/47 свободны ────────────────────────────────────
// Предназначены для опционального съёмного nRF24 (CE/CSN, делит SPI
// с SD). Владение динамическое — выдаются через PDA.Gpio.claim(),
// в конфиге зарезервированы как общего назначения.
// SIM800L/GPS/IR исключены из устройства навсегда.

// ── LED ───────────────────────────────────────────────────
#define PDA2_PIN_LED       48

// ── Экран ───────────────────────────────────────────────
// Физический поворот панели относительно «нативной» ориентации (0..3).
// Один источник правды: его читают и Display_Class_esp32 (offset_rotation),
// и Touch_Class (_transform) — тач всегда следует за картинкой.
#define PDA2_DISPLAY_ROT_OFFSET 2  
#define PDA2_SCREEN_W     320
#define PDA2_SCREEN_H     480
#define PDA2_ROTATION       2

// ── UI / Apps ───────────────────────────────────────────
#define PDA2_MAX_APPS      16
#define PDA2_ANIM_MS      240
#define PDA2_GRID_COLS      4
#define PDA2_ICON_SIZE     60

// ── NotesApp ────────────────────────────────────────────
#define PDA2_NOTES_MAX_SIZE   8192
#define PDA2_NOTES_MAX_FILES  20

// ── AccelApp canvas ───────────────────────────────────────
  // #define PDA2_ACCEL_CANVAS_W   280
  // #define PDA2_ACCEL_CANVAS_H   370
  // #define PDA2_ACCEL_CANVAS_X   20
  // #define PDA2_ACCEL_CANVAS_Y   10
  // #define PDA2_ACCEL_STRIP_H    100
  // #define PDA2_ACCEL_CUBE_CX    140
  // #define PDA2_ACCEL_CUBE_CY    185
  // #define PDA2_ACCEL_CUBE_SIZE   80
  // #define PDA2_ACCEL_BG_COLOR    0x0f172a
  // #define PDA2_ACCEL_FOCAL       300
  // #define PDA2_ACCEL_AXIS_LEN    96
  // #define PDA2_ACCEL_TICK_MS     33
  // #define PDA2_ACCEL_GYRO_DEADZONE  1.5f
  // #define PDA2_ACCEL_GYRO_SIGN_X   (-1)
  // #define PDA2_ACCEL_GYRO_SIGN_Y   (-1)
  // #define PDA2_ACCEL_GYRO_SIGN_Z   (-1)
  // #define PDA2_ACCEL_GYRO_MAP_X     0
  // #define PDA2_ACCEL_GYRO_MAP_Y     2
  // #define PDA2_ACCEL_GYRO_MAP_Z     1

// ── Launcher ──────────────────────────────────────────────
#define PDA2_LAUNCHER_GRID        0
#define PDA2_LAUNCHER_CAROUSEL    1
#define PDA2_LAUNCHER_MODE   PDA2_LAUNCHER_GRID

// ── Touch жесты ───────────────────────────────────────────
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
#define PDA2_QP_SWIPE_TOP_PCT    15
#define PDA2_QP_NOTIF_H         360
#define PDA2_QP_SETTINGS_H      200
#define PDA2_QP_ANIM_MS         220

// ── Логирование / NVS ─────────────────────────────────────
#define PDA2_LOG_LEVEL            3
#define PDA2_NVS_NS    "pda2"