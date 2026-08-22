#include "render.h"
#ifndef PDA2_SIM
#include <LovyanGFX.hpp>   // нужен полный тип LGFX_Device для pushImage()
#endif

void load::init() {
    PDA_LOGI("core", "Ok TGX loaded");
}

//Классы рендера
/* Возможно ломает цвета
void render::changeColor(float r, float g, float b){
    tgx::RGB565 color3(r, g, b);
}
*/

void render::vertic(tgx::Image<tgx::RGB565>& im, int screen_x, int screen_y, int w, int h /*int x, int y*/) {
    im.clear(tgx::RGB565_Black);
    im.fillTriangle({75, 2}, {40, 90}, {150, 10} , tgx::RGB565_Purple, tgx::RGB565_Magenta);
    auto& lcd = PDA.Display.raw();
    lcd.pushImage(screen_x, screen_y, w, h, (uint16_t*)im.data());
}

/*Не работает массив
 * UPD: замена на glVertexPointer(2, GL_FLOAT, 0, &vertices);
 *    GLfloat quadss[] =
 *    {
 *       glColor3f(1.0, 0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
 *       glColor3f(0.0, 1.0f, 0.0f); glVertex2f(0.5f, 0.0f);
 *       glColor3f(0.0, 0.0f, 1.0f); glVertex2f(0.0f, 0.5f);
 *       glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(0.0f, 0.5f);
 *      glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(0.5f, 0.0f);
 *      glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(0.5f, 0.5f);
      };*/

//Coded by WapWapG
//My steam: https://steamcommunity.com/profiles/76561199850375269/