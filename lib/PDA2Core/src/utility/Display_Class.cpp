// ════════════════════════════════════════════════════════
//  PDA 2 — Display_Class.cpp
//  LovyanGFX LGFX_PDA2 + LVGL 9.x RENDER_MODE_FULL
//
//  Почему FULL:
//    ILI9488 — 3 байта/пиксель (18-bit SPI).
//    PARTIAL → несколько транзакций → мерцание.
//    FULL → один кадр за транзакцию → нет мерцания.
//    Один буфер в PSRAM (~300KB). Два буфера — PSRAM bus contention (BL-1).
//
//  Fade-анимации запрещены — вызывают затемнение на ILI9488.
// ════════════════════════════════════════════════════════

#include "Display_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"

#include <LovyanGFX.hpp>
#include <lvgl.h>
#include <esp_heap_caps.h>

// ── LovyanGFX конфигурация для ILI9488 ──────────────────
class LGFX_PDA2 : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9488 _panel;
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
            cfg.pin_miso   = -1;
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
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
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
    return (uint32_t)millis();
}

// ── Реализация Display_Class ─────────────────────────────
bool Display_Class::begin(uint8_t brightness, uint8_t rotation) {
    _brightness = brightness;
    _brightness = 200;
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
    _lcd.setColorDepth(16);   // RGB565 на стороне LVGL

    PDA_LOGI("display", "LovyanGFX ok. %dx%d rot=%d",
             _lcd.width(), _lcd.height(), _rotation);

    // LVGL init
    lv_init();
    lv_tick_set_cb(_tick_cb);

    // Буфер в PSRAM (RENDER_MODE_FULL)
    const size_t buf_size = (size_t)_lcd.width() * _lcd.height() * 2;
    _buf1 = (uint8_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);

    if (!_buf1) {
        PDA_LOGE("display", "PSRAM alloc failed! buf_size=%u", buf_size);
        return false;  // [A-06]
    }

    PDA_LOGI("display", "LVGL buf: 1 x %u bytes in PSRAM", buf_size);

    // Создать LVGL display
    _lv_disp = lv_display_create(_lcd.width(), _lcd.height());
    lv_display_set_flush_cb(_lv_disp, _flush_cb);
    lv_display_set_buffers(_lv_disp, _buf1, nullptr, buf_size,
                       LV_DISPLAY_RENDER_MODE_FULL);

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
