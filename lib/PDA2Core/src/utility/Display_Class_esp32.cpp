// ════════════════════════════════════════════════════════
//  PDA 2 — Display_Class_esp32.cpp
//  LovyanGFX LGFX_PDA2 (Panel_ST7796) + LVGL 9.x RENDER_MODE_PARTIAL
//
//  Панель: MSP3526 — ST7796U, 320×480, SPI, RGB565 (2 байта/пиксель).
//
//  Боевой режим — PARTIAL: два буфера в PSRAM (~1/10 экрана каждый).
//  Тиринг аппаратный (нет TE-пина на модуле), одинаков на FULL и PARTIAL,
//  программно не устраняется.
//
//  Fade-анимации запрещены (правило 8).
//
//  Подбор на железе: cfg.rgb_order (красный↔синий), PDA2_SPI_FREQ (артефакты на 79 МГц → 40 МГц → ~27 МГц).
//
//  Компилируется, только если PDA2_SIM НЕ определён —
//  в сборке симулятора тело пустое (см. Display_Class_sim.cpp).
//  raw() здесь возвращает настоящий LGFX_Device — на симуляторе
//  та же сигнатура вызова (auto&, pushImage) работает через
//  PdaRawDisplay_Sim (см. Display_Class.h и Display_Class_sim.cpp).
// ════════════════════════════════════════════════════════

#include "Display_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"
#include "../pda2_platform.h"

#ifndef PDA2_SIM

#include <LovyanGFX.hpp>
#include <lvgl.h>
#include <esp_heap_caps.h>

// ── LovyanGFX конфигурация для ST7796U ──────────────────
class LGFX_PDA2 : public lgfx::LGFX_Device {
    lgfx::Panel_ST7796  _panel;
    lgfx::Bus_SPI       _bus;
    lgfx::Light_PWM     _light;
public:
    LGFX_PDA2() {
        {
            auto cfg = _bus.config();
            cfg.spi_host   = SPI2_HOST;
            cfg.spi_mode   = 0;
            cfg.freq_write = PDA2_SPI_FREQ;
            cfg.freq_read  = 16000000;
            cfg.pin_mosi   = PDA2_PIN_MOSI;
            cfg.pin_miso   = -1;            // MISO на модуле не подключён
            cfg.pin_sclk   = PDA2_PIN_SCK;
            cfg.pin_dc     = PDA2_PIN_DC;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs   = PDA2_PIN_CS;
            cfg.pin_rst  = PDA2_PIN_RST;
            cfg.pin_busy = -1;
            cfg.panel_width  = PDA2_SCREEN_W;
            cfg.panel_height = PDA2_SCREEN_H;
            cfg.offset_x     = 0;
            cfg.offset_y     = 0;
            cfg.offset_rotation = PDA2_DISPLAY_ROT_OFFSET;   // физический поворот панели, тач берёт то же значение
            cfg.readable    = false;
            cfg.invert      = true;
            cfg.rgb_order   = false;
            cfg.dlen_16bit  = false;
            cfg.bus_shared  = false;
            _panel.config(cfg);
        }
        {
            auto cfg = _light.config();
            cfg.pin_bl    = PDA2_PIN_BL;
            cfg.invert    = false;
            cfg.freq = PDA2_SPI_FREQ_BL;
            cfg.pwm_channel = 7;
            _light.config(cfg);
            _panel.setLight(&_light);
        }
        setPanel(&_panel);
    }
};

// ── Глобальные объекты ───────────────────────────────────
static LGFX_PDA2      _lcd;
static lv_display_t*  _lv_disp = nullptr;
static uint8_t*       _buf1 = nullptr;
static uint8_t*       _buf2 = nullptr;

// ── LVGL flush callback ──────────────────────────────────
static void _flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    _lcd.startWrite();
    _lcd.setAddrWindow(area->x1, area->y1, w, h);
    _lcd.writePixelsDMA((lgfx::rgb565_t*)px_map, w * h);
    _lcd.endWrite();
    lv_display_flush_ready(disp);
}

// ── LVGL tick callback ───────────────────────────────────
static uint32_t _tick_cb() {
    return pda2_platform_now_ms();
}

// ── Реализация Display_Class ─────────────────────────────
bool Display_Class::begin(uint8_t brightness, uint8_t rotation) {
    _brightness = brightness;
    _rotation   = rotation;

    pinMode(PDA2_PIN_RST, OUTPUT);
    digitalWrite(PDA2_PIN_RST, LOW);
    delay(100);
    digitalWrite(PDA2_PIN_RST, HIGH);
    delay(200);

    // LovyanGFX init
    _lcd.init();
    _lcd.setRotation(_rotation);
    _lcd.setBrightness(_brightness);
    _lcd.setColorDepth(16);   // RGB565

    PDA_LOGI("display", "LovyanGFX ok (ST7796). %dx%d rot=%d",
             _lcd.width(), _lcd.height(), _rotation);

    // LVGL init
    lv_init();
    lv_tick_set_cb(_tick_cb);

    // Два буфера в PSRAM, ~1/10 экрана каждый (RENDER_MODE_PARTIAL)
    const size_t partial_lines = _lcd.height() / 10;
    const size_t buf_size = (size_t)_lcd.width() * partial_lines * 2;

    _buf1 = (uint8_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    _buf2 = (uint8_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);

    if (!_buf1 || !_buf2) {
        PDA_LOGE("display", "PSRAM alloc failed! buf_size=%u", buf_size);
        return false;  // [A-06]
    }

    PDA_LOGI("display", "LVGL buf: 2 x %u lines, %u bytes each in PSRAM",
             partial_lines, buf_size);

    // Создать LVGL display
    _lv_disp = lv_display_create(_lcd.width(), _lcd.height());
    lv_display_set_flush_cb(_lv_disp, _flush_cb);
    lv_display_set_buffers(_lv_disp, _buf1, _buf2, buf_size,
                       LV_DISPLAY_RENDER_MODE_PARTIAL);

    PDA_LOGI("display", "LVGL 9.2.1 ready");
    return true;  // [A-06]
}

void Display_Class::setBrightness(uint8_t v) {
    _brightness = v;
    _lcd.setBrightness(v);
}

uint8_t Display_Class::getBrightness() { return _brightness; }

void Display_Class::setRotation(uint8_t r) {
    _rotation = r;
    _lcd.setRotation(r);
    if (_lv_disp) {
        lv_display_set_resolution(_lv_disp, _lcd.width(), _lcd.height());
    }
    PDA_LOGI("display", "Rotation set to %d (%dx%d)", r, _lcd.width(), _lcd.height());
}

uint8_t Display_Class::getRotation() { return _rotation; }

int Display_Class::width()  { return _lcd.width(); }
int Display_Class::height() { return _lcd.height(); }

lgfx::v1::LGFX_Device& Display_Class::raw() {
    return _lcd;
}

#endif // !PDA2_SIM