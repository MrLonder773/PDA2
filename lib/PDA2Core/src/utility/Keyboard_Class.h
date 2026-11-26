#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Keyboard_Class
//  lib/PDA2Core/src/utility/Keyboard_Class.h
//
//  Экранная клавиатура (модальная утилита). Приложение создаёт
//  её на СВОЁМ экране (attach), в onTick() читает события через
//  get() и само пишет в lv_textarea.
//
//  Шаг 1а: RU/EN, ⇧ (одноразовый), 123, ⌫, ↵. Тип поля TEXT.
//  Шаг 1б: тап по свободному месту экрана скрывает клавиатуру,
//  тап по полю показывает (show/hide — и вручную).
//  Кирпич 2: типы полей TEXT / PASSWORD / NUMBER (bind, field()).
//  Шаг 3а: подсказки в углу клавиш (RU, нижний регистр) и долгое
//  нажатие вводит подсказку вместо буквы.
//  Шаг 3б: попап над клавишей на время долгого нажатия (показывает вводимую
//  подсказку). Выбор варианта пальцем — 3в.
//  Без lv_timer, без анимаций.
//
//  Реализация — блок-файлы одного класса: Keyboard_Class.cpp (ядро),
//  Keyboard_Layout.cpp (карты и внешний вид), Keyboard_Hints.cpp (подсказки, попап).
//
//  Основа — lv_keyboard БЕЗ привязки к textarea
//  (lv_keyboard_set_textarea не вызывается): сам он ничего не пишет.
//  Подписи "abc", "ABC", "1#" и LV_SYMBOL_* в картах не использовать:
//  их перехватывает штатный обработчик lv_keyboard.
// ════════════════════════════════════════════════════════

#include <lvgl.h>
#include "../pda2_config.h"

// Подписи служебных клавиш (карты и разбор используют одни макросы)
#define KB_KEY_SHIFT     "↑"
#define KB_KEY_SHIFT_UP  "↑↑"
#define KB_KEY_BS        LV_SYMBOL_BACKSPACE
#define KB_KEY_ENTER     LV_SYMBOL_NEW_LINE
#define KB_KEY_NUM       "?123"
#define KB_KEY_SYM       "=\\<"
#define KB_KEY_ABC_RU    "АБВ"
#define KB_KEY_ABC_EN    "ABC"

enum KbEventType : uint8_t {
    KB_CHAR = 1,      // utf8 — введённый символ (пробел тоже)
    KB_BACKSPACE,
    KB_ENTER
};

struct KbEvent {
    KbEventType type;
    char        utf8[5];   // UTF-8 + '\0'; пусто у BACKSPACE/ENTER
};

// Тип поля: определяет страницу клавиатуры и режим поля.
enum KbType : uint8_t {
    KBT_TEXT = 0,    // RU/EN/123, старт на RU
    KBT_PASSWORD,    // как TEXT, старт на EN, поле в режиме пароля (точки)
    KBT_NUMBER       // цифровая страница: 0-9, «-», «.», ⌫, ↵
};

class Keyboard_Class {
public:
    // Создать клавиатуру на экране приложения (parent). field — первое поле
    // ввода (становится активным): тап по нему показывает клавиатуру; тап по
    // свободному месту parent скрывает. Повторный вызов, пока клавиатура
    // жива, игнорируется. При удалении экрана клавиатура умирает вместе
    // с ним и сама обнуляет указатель.
    void attach(lv_obj_t* parent, lv_obj_t* field = nullptr, KbType type = KBT_TEXT);

    // Ещё одно поле того же экрана (не больше PDA2_KB_FIELDS на экран).
    // Тап по нему делает его активным, выбирает страницу по типу и показывает
    // клавиатуру. Вызывать после attach().
    void bind(lv_obj_t* field, KbType type);

    // Активное поле (то, по которому тапнули последним); nullptr, если нет.
    // Приложение пишет события get() именно в него.
    lv_obj_t* field() const { return _field; }

    // Показать / скрыть (клавиатура остаётся на экране, флаг HIDDEN).
    // hide() очищает очередь событий и запомненную подсказку.
    void show();
    void hide();

    // Взять очередное событие. false — очередь пуста.
    bool get(KbEvent& ev);

    bool attached() const { return _kb != nullptr; }

private:
    enum Page : uint8_t { P_RU_LO, P_RU_UP, P_EN_LO, P_EN_UP, P_NUM_RU, P_NUM_EN, P_PAD, P_SYM_RU, P_SYM_EN };

    struct Bind {                    // user_data колбэка поля
        Keyboard_Class* self = nullptr;
        lv_obj_t*       obj  = nullptr;
        KbType          type = KBT_TEXT;
    };

    lv_obj_t* _kb      = nullptr;
    lv_obj_t* _field   = nullptr;    // активное поле
    Page      _page    = P_RU_LO;
    KbType    _type    = KBT_TEXT;
    bool      _lang_en = false;

    Bind      _binds[PDA2_KB_FIELDS];
    uint8_t   _nbinds = 0;

    KbEvent   _q[PDA2_KB_QUEUE];
    uint8_t   _head  = 0;
    uint8_t   _count = 0;

    lv_obj_t*   _hints     = nullptr;   // контейнер подсказок (дочерний _kb), виден только на P_RU_LO
    const char* _long_hint = nullptr;   // подсказка, запомненная на LONG; nullptr — нет
    uint32_t    _long_id   = 0;         // клавиша, на которой запомнена

    // 3в: панель вариантов над рядом + «ножка» над клавишей (дочерние родителя клавиатуры)
    lv_obj_t*   _cpanel      = nullptr;
    lv_obj_t*   _cleg        = nullptr;
    lv_obj_t*   _cell[2]     = {nullptr, nullptr};   // дочерние _cpanel
    lv_obj_t*   _cell_lbl[2] = {nullptr, nullptr};
    const char* _var[2]      = {nullptr, nullptr};
    uint8_t     _var_n       = 0;
    uint8_t     _var_idx     = 0;
    int16_t     _var_cx      = 0;       // центр клавиши по x
    bool        _var_on      = false;   // VALUE глушится, ввод на RELEASED
    bool        _bs_hold = false;       // ⌫ удерживается: повтор делает get(), штатный глушится
    uint32_t    _bs_next = 0;           // время следующего стирания, мс
    uint32_t _press_ms = 0;   // время касания (PRESSED), мс

    bool     _hide_pending = false;   // попап ждёт скрытия после отпускания
    uint32_t _hide_at      = 0;       // время скрытия, мс

    uint32_t _pop_id = LV_BUTTONMATRIX_BUTTON_NONE;   // клавиша, для которой показан попап
    void _panel_hide();                                // спрятать панель, не трогая _pop_id
    void _press_popup(lv_obj_t* obj);                  // попап для нажатой клавиши
    bool _key_rect(uint32_t id, int& x, int& y, int& w, int& h);   // Keyboard_Layout.cpp

    void _cells_show(uint8_t row, uint8_t col);
    void _cells_update(int x);
    void _push(KbEventType t, const char* utf8);
    void _show(Page p);
    void _start_page(KbType type);
    void _activate(Bind& b);
    void _handle(const char* txt);
    void _apply_layout();   // Keyboard_Layout.cpp
    void _make_hints();
    void _sync_hints();
    void _make_popup();
    void _popup_show(uint8_t row, uint8_t col, const char* txt);
    void _popup_hide();
    void _popup_release();            // отпускание: сброс состояния, панель прячется с задержкой

    void _popup_show(uint32_t id, const char* txt);
    void _cells_show(uint32_t id);
    void _set_sym_map(Page p);   // Keyboard_Layout.cpp

    static void _event_cb(lv_event_t* e);
    static void _touch_cb(lv_event_t* e);   // PRESSED/RELEASED/PRESS_LOST прячут попап; LONG_PRESSED запоминает подсказку и показывает попап
    static void _outside_cb(lv_event_t* e);
    static void _field_cb(lv_event_t* e);
    static void _delete_cb(lv_event_t* e);
};