#include "TinyGLTestApp.h"
#include <cmath>

#ifndef PDA2_SIM
#include <LovyanGFX.hpp>   // нужен полный тип LGFX_Device для raw().startWrite()/setAddrWindow()/writePixelsDMA()/endWrite()
#endif

namespace {
    // Ручная замена gluPerspective: GLU в jserv/tinygl отсутствует, glFrustum реализован.
    void gl_frustum_perspective(float fovy_deg, float aspect, float znear, float zfar) {
        float ymax = znear * tanf(fovy_deg * (float)M_PI / 360.0f);
        float ymin = -ymax;
        float xmin = ymin * aspect;
        float xmax = ymax * aspect;
        glFrustum(xmin, xmax, ymin, ymax, znear, zfar);
    }
}

void TinyGLTestApp::onInit() {
    // пусто — LVGL и железо не трогаем при инициализации
}

void TinyGLTestApp::onOpen() {
    if (!screen) {
        screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    }

    _zb = ZB_open(320, 480, ZB_MODE_5R6G5B, nullptr);
    if (!_zb) {
        PDA_LOGE("tinygl", "ZB_open failed");
        return;
    }
    glInit(_zb);

    glViewport(0, 0, 320, 480);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gl_frustum_perspective(45.0f, 320.0f / 480.0f, 1.0f, 100.0f);
    glMatrixMode(GL_MODELVIEW);

    _angle = 0.0f;
    
}

void TinyGLTestApp::onTick(uint32_t delta_ms) {
    if (!_zb) return;

    _angle += delta_ms * 0.05f;

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f);
    glRotatef(_angle, 0.0f, 1.0f, 0.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f( 0.0f,  1.0f, 0.0f);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, 0.0f);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f( 1.0f, -1.0f, 0.0f);
    glEnd();

    // тот же путь вывода, что в LVGL flush_cb
    PDA.Display.raw().startWrite();
    PDA.Display.raw().setAddrWindow(0, 0, 320, 480);
    PDA.Display.raw().writePixelsDMA((pda2_pixel_ptr_t)_zb->pbuf, 320 * 480);
    PDA.Display.raw().endWrite();
}

void TinyGLTestApp::onClose() {
    if (_zb) {
        glClose();
        ZB_close(_zb);
        _zb = nullptr;
    }
}