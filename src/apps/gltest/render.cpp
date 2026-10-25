// #include "render.h"
// #ifndef PDA2_SIM
// #include <LovyanGFX.hpp>   // нужен полный тип LGFX_Device для pushImage()
// #endif

// void load::init() {
//     PDA_LOGI("core", "Ok TGX loaded");
// }

// //Классы рендера
// /* Возможно ломает цвета
// void render::changeColor(float r, float g, float b){
//     tgx::RGB565 color3(r, g, b);
// }
// */

// void render::vertic(tgx::Image<tgx::RGB565>& im, int screen_x, int screen_y, int w, int h /*int x, int y*/) {
//     im.clear(tgx::RGB565_Black);
//     im.fillTriangle({75, 2}, {40, 90}, {150, 10} , tgx::RGB565_Purple, tgx::RGB565_Magenta);
//     auto& lcd = PDA.Display.raw();
//     lcd.startWrite();
//     lcd.setAddrWindow(screen_x, screen_y, w, h);
//     lcd.writePixelsDMA((pda2_pixel_ptr_t)im.data(), w * h);
//     lcd.endWrite();
// }

// // ─ Для WapWapG: как теперь рисуется этот кадр ─────────────
//     // Раньше тут был lcd.pushImage(...) — высокоуровневый метод
//     // LovyanGFX, сам конвертирующий/свопающий пиксели под формат
//     // панели. Из-за этого на физическом PDA2 цвета уезжали (фиолетовый
//     // становился зелёным/циановым), хотя в симуляторе (где pushImage
//     // был простой SDL-заглушкой) всё было верно — баг был виден
//     // только на живом железе.
//     //
//     // Теперь вызывается тот же низкоуровневый путь, что и обычный
//     // LVGL UI (Launcher/ClockApp и т.д.) — он цвета не портил никогда:
//     //   1. startWrite()     — открыть SPI-транзакцию с панелью
//     //                         (на симе — no-op, см. PdaRawDisplay_Sim)
//     //   2. setAddrWindow()  — задать прямоугольник под кадр
//     //   3. writePixelsDMA() — залить пиксели как есть, без доп.
//     //                         конвертации (ESP32 — DMA в SPI,
//     //                         сим — SDL_UpdateTexture + Present)
//     //   4. endWrite()       — закрыть транзакцию (на симе — no-op)
//     //
//     // Один и тот же вызов работает на обеих платформах без #ifdef —
//     // разный тип указателя на пиксель (lgfx::rgb565_t* на ESP32,
//     // uint16_t* на симе) спрятан за pda2_pixel_ptr_t (Display_Class.h).

// /*Не работает массив
//  * UPD: замена на glVertexPointer(2, GL_FLOAT, 0, &vertices);
//  *    GLfloat quadss[] =
//  *    {
//  *       glColor3f(1.0, 0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
//  *       glColor3f(0.0, 1.0f, 0.0f); glVertex2f(0.5f, 0.0f);
//  *       glColor3f(0.0, 0.0f, 1.0f); glVertex2f(0.0f, 0.5f);
//  *       glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(0.0f, 0.5f);
//  *      glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(0.5f, 0.0f);
//  *      glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(0.5f, 0.5f);
//       };*/

// //Coded by WapWapG
// //My steam: https://steamcommunity.com/profiles/76561199850375269/