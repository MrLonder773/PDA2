#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — PDA2App.h
//  Базовый класс всех приложений.
//  Каждое приложение = отдельная папка в apps/ + наследник.
// ════════════════════════════════════════════════════════

#include <lvgl.h>

class PDA2App {
public:
    const char*  name   = "App";
    lv_obj_t*    screen = nullptr;   // создаётся в onInit() через lv_obj_create(NULL)

    virtual ~PDA2App() = default;

    // Вызывается один раз при PDA.Apps.start()
    // Строим UI: screen = lv_obj_create(NULL); ...
    virtual void onInit() {}

    // Вызывается каждый раз при открытии приложения
    virtual void onOpen() {}

    // Вызывается при закрытии (возврат в launcher)
    virtual void onClose() {}

    // Вызывается каждый фрейм пока приложение активно
    // delta_ms — время с предыдущего вызова в мс
    virtual void onTick(uint32_t delta_ms) {}

    // Вызывается каждый фрейм пока приложение свёрнуто
    virtual void onBackground() {}

    virtual void onKey(uint8_t keycode, uint8_t modifier) {}  // ← добавлено
};

// ════════════════════════════════════════════════════════
//  Шаблон нового приложения:
//
//  apps/myapp/MyApp.h:
//  ───────────────────
//  #pragma once
//  #include <PDA2.h>
//
//  class MyApp : public PDA2App {
//  public:
//      MyApp() { name = "MyApp"; }
//      void onInit()  override;
//      void onOpen()  override;
//      void onClose() override;
//      void onTick(uint32_t delta_ms) override;
//  private:
//      lv_obj_t* _lbl_title = nullptr;
//  };
//
//  apps/myapp/MyApp.cpp:
//  ─────────────────────
//  #include "MyApp.h"
//
//  void MyApp::onInit() {
//      screen = lv_obj_create(NULL);          // NULL — обязательно
//      _lbl_title = lv_label_create(screen);
//      lv_obj_set_style_bg_opa(_lbl_title, LV_OPA_COVER, 0);   // обязательно!
//      lv_label_set_text(_lbl_title, "Hello");
//  }
//
//  void MyApp::onTick(uint32_t dt) {
//      pda2_time_t t = PDA.Rtc.get();
//      // ...
//  }
// ════════════════════════════════════════════════════════