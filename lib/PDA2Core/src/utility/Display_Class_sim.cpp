// ════════════════════════════════════════════════════════
//  PDA 2 — Display_Class_sim.cpp
//  SDL2-окно + LVGL 9.x RENDER_MODE_FULL (та же схема, что
//  и на железе — один буфер, один flush за кадр).
//
//  Формат текстуры — RGB565, тот же, что LVGL уже кладёт
//  в буфер на ESP32-стороне (see _lcd.setColorDepth(16) в
//  Display_Class_esp32.cpp) — конвертация не нужна.
//
//  raw() — PdaRawDisplay_Sim::pushImage() пишет прямо в SDL2-
//  текстуру (SDL_UpdateTexture на конкретный SDL_Rect) и сразу
//  показывает кадр — мимо LVGL-буфера (_buf1), тем же способом,
//  каким LGFX_Device::pushImage() на железе пишет прямо в LCD
//  через SPI мимо LVGL. GLTestApp/render.cpp достаёт raw() через
//  auto&, поэтому компилируется на обеих платформах без правок.
//
//  Компилируется, только если PDA2_SIM определён —
//  в прошивке ESP32 тело пустое (см. Display_Class_esp32.cpp).
// ════════════════════════════════════════════════════════

#include "Display_Class.h"
#include "../pda2_config.h"
#include "../pda2_log.h"
#include "../pda2_platform.h"

#ifdef PDA2_SIM

#include <SDL2/SDL.h>
#include <lvgl.h>
#include <cstdlib>

// ── Глобальные объекты ───────────────────────────────────
static SDL_Window*   _window  = nullptr;
static SDL_Renderer* _renderer= nullptr;
static SDL_Texture*  _texture = nullptr;
static lv_display_t* _lv_disp = nullptr;
static uint8_t*      _buf1    = nullptr;

// ── Логическая ширина/высота с учётом rotation ───────────
// (rotation 1/3 — поворот на 90°, оси меняются местами;
//  0/2 — оси как есть, PDA2_ROTATION=2 по умолчанию)
static int _logical_w(uint8_t rotation) {
    return (rotation == 1 || rotation == 3) ? PDA2_SCREEN_H : PDA2_SCREEN_W;
}
static int _logical_h(uint8_t rotation) {
    return (rotation == 1 || rotation == 3) ? PDA2_SCREEN_W : PDA2_SCREEN_H;
}

// ── LVGL flush callback ──────────────────────────────────
static void _flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    // RENDER_MODE_FULL — area всегда весь экран, поэтому просто
    // обновляем текстуру целиком и показываем кадр.
    SDL_UpdateTexture(_texture, nullptr, px_map,
                       lv_display_get_horizontal_resolution(disp) * 2 /*RGB565*/);
    SDL_RenderClear(_renderer);
    SDL_RenderCopy(_renderer, _texture, nullptr, nullptr);
    SDL_RenderPresent(_renderer);
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

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        PDA_LOGE("display", "SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    int w = _logical_w(_rotation);
    int h = _logical_h(_rotation);

    _window = SDL_CreateWindow("PDA 2 SIM",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                w, h, SDL_WINDOW_SHOWN);
    if (!_window) {
        PDA_LOGE("display", "SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
    if (!_renderer) {
        PDA_LOGE("display", "SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGB565,
                                  SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!_texture) {
        PDA_LOGE("display", "SDL_CreateTexture failed: %s", SDL_GetError());
        return false;
    }

    PDA_LOGI("display", "SDL2 window ok. %dx%d rot=%d", w, h, _rotation);

    // LVGL init
    lv_init();
    lv_tick_set_cb(_tick_cb);

    // Буфер — обычный malloc, PSRAM на PC нет
    const size_t buf_size = (size_t)w * h * 2;
    _buf1 = (uint8_t*)malloc(buf_size);

    if (!_buf1) {
        PDA_LOGE("display", "malloc failed! buf_size=%u", (unsigned)buf_size);
        return false;
    }

    PDA_LOGI("display", "LVGL buf: 1 x %u bytes (RAM)", (unsigned)buf_size);

    _lv_disp = lv_display_create(w, h);
    lv_display_set_flush_cb(_lv_disp, _flush_cb);
    lv_display_set_buffers(_lv_disp, _buf1, nullptr, buf_size,
                       LV_DISPLAY_RENDER_MODE_FULL);

    PDA_LOGI("display", "LVGL ready (SIM)");
    return true;
}

void Display_Class::setBrightness(uint8_t v) {
    _brightness = v;
    // Физической подсветки на PC нет — просто храним значение.
}

uint8_t Display_Class::getBrightness() { return _brightness; }

void Display_Class::setRotation(uint8_t r) {
    _rotation = r;
    // ⚠️ Окно/текстура НЕ пересоздаются при смене rotation —
    // это редкий рантайм-кейс (в проекте нет апы, меняющей его
    // на лету), пересоздание окна оставлено на потом при
    // необходимости. Логическое разрешение для LVGL обновляем
    // всё равно, чтобы не расходиться с getRotation()/width()/height().
    if (_lv_disp) {
        lv_display_set_resolution(_lv_disp, _logical_w(r), _logical_h(r));
    }
    PDA_LOGI("display", "SIM: rotation set to %d (%dx%d, окно НЕ пересоздано)",
             r, _logical_w(r), _logical_h(r));
}

uint8_t Display_Class::getRotation() { return _rotation; }

int Display_Class::width()  { return _logical_w(_rotation); }
int Display_Class::height() { return _logical_h(_rotation); }

// ── raw() / PdaRawDisplay_Sim ────────────────────────────
static PdaRawDisplay_Sim _raw_display;

void PdaRawDisplay_Sim::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) {
    SDL_Rect r{ (int)x, (int)y, (int)w, (int)h };
    SDL_UpdateTexture(_texture, &r, data, w * 2 /*RGB565*/);
    SDL_RenderClear(_renderer);
    SDL_RenderCopy(_renderer, _texture, nullptr, nullptr);
    SDL_RenderPresent(_renderer);
}

PdaRawDisplay_Sim& Display_Class::raw() {
    return _raw_display;
}

#endif // PDA2_SIM