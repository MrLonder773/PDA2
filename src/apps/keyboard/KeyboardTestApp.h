#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — KeyboardTestApp.h
//  Временное тестовое приложение для PDA.Keyboard (кирпич 1–2).
//  Три поля: текст, пароль, число. Удалить после кирпича 2.
// ════════════════════════════════════════════════════════

#include <PDA2.h>

class KeyboardTestApp : public PDA2App {
public:
    KeyboardTestApp() { name = "KbTest"; }

    void onInit()  override;
    void onOpen()  override;
    void onClose() override;
    void onTick(uint32_t delta_ms) override;

private:
    lv_obj_t* _ta  = nullptr;   // KBT_TEXT
    lv_obj_t* _pw  = nullptr;   // KBT_PASSWORD
    lv_obj_t* _num = nullptr;   // KBT_NUMBER

    lv_obj_t* _make_field(int y, const char* caption);
};