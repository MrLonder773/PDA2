#include "GLTestApp.h"
#include "render.h"
//#include "logic.h"
#include <tgx.h>

// ── onOpen ───────────────────────────────────────────────────────────────
// Экран ленивый: строится один раз при первом открытии (screen == nullptr).
// onInit() LVGL не трогает (см. PDA2App.h / правило "ленивые экраны").

void GLTestApp::onOpen() {
    if (!screen) {
        screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    }

    //Старый кусок
    /*
    _zb = ZB_open(W, H, ZB_MODE_5R6G5B, nullptr);
    //Настройка памяти(буффера)
    memset(_zb->pbuf, 0, W * H * sizeof(PIXEL));
    memset(_zb->zbuf, 0, W * H * sizeof(GLushort));
    */
    //Инициализирую библиотеки
    load.init();
}

void GLTestApp::onTick(uint32_t delta_ms) {
    //Не реализовано!
    //GLfloat vertices[] = {
    //    0.0f, 0.0f,
    //    0.5f, 0.0f,
    //    0.0f, 0.5f
    //};
    //render.changeColor(1,0,0);
    render.vertic(im, SCREEN_X, SCREEN_Y, W, H);
    //Не проверено, скорее всего не работает.
    //render.changeColor(0.25, 0.25, 0.25);
}

void GLTestApp::onClose() {
    /*
    if (_zb) {
        glClose();
        ZB_close(_zb);
        _zb = nullptr;
    }
    */
}