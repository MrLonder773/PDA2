// ════════════════════════════════════════════════════════
//  PDA 2 — Keyboard_Layout.cpp (блок класса Keyboard_Class)
//  lib/PDA2Core/src/utility/Keyboard_Layout.cpp
//
//  Карты клавиатуры: ширина клавиши — число в ctrl-карте (1..15).
//  Цифровая страница (тип NUMBER): 4 колонки, нижний ряд «0» и «>» по 2.
//  RU: 11 колонок, все ряды 11 единиц.
//  EN и 123: 10 колонок, ряды по 20 единиц (клавиша = 2),
//  скрытые «прокладки» центрируют ряд из 9 клавиш.
//  Подписи-заглушки (до глифов): ^ ⇧, ^^ ⇧ (верхний регистр),
//  < ⌫, > ↵, RU/EN — глобус.
//  _apply_layout(): карты страниц и внешний вид клавиш.
// ════════════════════════════════════════════════════════

#include "Keyboard_Class.h"
#include "../PDA2.h"

#define KW(n) ((lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_NO_REPEAT | LV_BUTTONMATRIX_CTRL_CLICK_TRIG | (n)))
#define KR(n) ((lv_buttonmatrix_ctrl_t)(n))   // нажатие + автоповтор (только ⌫)
#define KH(n) ((lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_HIDDEN | (n)))

// ── RU (11 колонок) ──────────────────────────────────────
static const char* const MAP_RU_LO[] = {
    "й","ц","у","к","е","н","г","ш","щ","з","х","\n",
    "ф","ы","в","а","п","р","о","л","д","ж","э","\n",
    "^","я","ч","с","м","и","т","ь","б","ю","<","\n",
    "123",",","EN","Русский",".",">","" };
static const char* const MAP_RU_UP[] = {
    "Й","Ц","У","К","Е","Н","Г","Ш","Щ","З","Х","\n",
    "Ф","Ы","В","А","П","Р","О","Л","Д","Ж","Э","\n",
    "^^","Я","Ч","С","М","И","Т","Ь","Б","Ю","<","\n",
    "123",",","EN","Русский",".",">","" };
static const lv_buttonmatrix_ctrl_t CTRL_RU[] = {
    KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),
    KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),
    KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KW(1),KR(1),
    KW(1),KW(1),KW(1),KW(4),KW(1),KW(2) };

// ── EN (10 колонок) ──────────────────────────────────────
static const char* const MAP_EN_LO[] = {
    "q","w","e","r","t","y","u","i","o","p","\n",
    " ","a","s","d","f","g","h","j","k","l"," ","\n",
    "^","z","x","c","v","b","n","m","<","\n",
    "123",",","RU","English",".",">","" };
static const char* const MAP_EN_UP[] = {
    "Q","W","E","R","T","Y","U","I","O","P","\n",
    " ","A","S","D","F","G","H","J","K","L"," ","\n",
    "^^","Z","X","C","V","B","N","M","<","\n",
    "123",",","RU","English",".",">","" };
static const lv_buttonmatrix_ctrl_t CTRL_EN[] = {
    KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),
    KH(1),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KH(1),
    KW(3),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KR(3),
    KW(1),KW(1),KW(1),KW(4),KW(1),KW(2) };

// ── 123 (две карты: отличаются левой клавишей и подписью пробела) ──
static const char* const MAP_NUM_RU[] = {
    "1","2","3","4","5","6","7","8","9","0","\n",
    "-","/",":",";","(",")","@","#","&","\"","\n",
    "+","%","_","=","?","!","'","*","<","\n",
    "РУС",",","EN","Русский",".",">","" };
static const char* const MAP_NUM_EN[] = {
    "1","2","3","4","5","6","7","8","9","0","\n",
    "-","/",":",";","(",")","@","#","&","\"","\n",
    "+","%","_","=","?","!","'","*","<","\n",
    "ENG",",","RU","English",".",">","" };
static const lv_buttonmatrix_ctrl_t CTRL_NUM[] = {
    KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),
    KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),
    KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KW(2),KR(4),
    KW(3),KW(2),KW(2),KW(8),KW(2),KW(3) };

// ── Цифровая страница (тип NUMBER): 4 колонки, ⌫ с автоповтором ──
static const char* const MAP_PAD[] = {
    "1","2","3","<","\n",
    "4","5","6","-","\n",
    "7","8","9",".","\n",
    "0",">","" };
static const lv_buttonmatrix_ctrl_t CTRL_PAD[] = {
    KW(1),KW(1),KW(1),KR(1),
    KW(1),KW(1),KW(1),KW(1),
    KW(1),KW(1),KW(1),KW(1),
    KW(2),KW(2) };

// ── карты страниц и внешний вид клавиш ────────────────────
// Вызывается из attach() сразу после создания _kb.
void Keyboard_Class::_apply_layout() {
    lv_keyboard_set_map(_kb, LV_KEYBOARD_MODE_USER_1,  MAP_RU_LO,  CTRL_RU);
    lv_keyboard_set_map(_kb, LV_KEYBOARD_MODE_USER_2,  MAP_RU_UP,  CTRL_RU);
    lv_keyboard_set_map(_kb, LV_KEYBOARD_MODE_USER_3,  MAP_EN_LO,  CTRL_EN);
    lv_keyboard_set_map(_kb, LV_KEYBOARD_MODE_USER_4,  MAP_EN_UP,  CTRL_EN);
    lv_keyboard_set_map(_kb, LV_KEYBOARD_MODE_SPECIAL, MAP_NUM_RU, CTRL_NUM);
    lv_keyboard_set_map(_kb, LV_KEYBOARD_MODE_NUMBER,  MAP_NUM_EN, CTRL_NUM);
    lv_keyboard_set_map(_kb, LV_KEYBOARD_MODE_TEXT_LOWER, MAP_PAD, CTRL_PAD);

    // Внешний вид — временно литералами (до Ui_Class / токенов)
    lv_obj_set_style_bg_color(_kb, lv_color_hex(0x1a2438), 0);
    lv_obj_set_style_bg_opa(_kb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_kb, 0, 0);
    lv_obj_set_style_pad_all(_kb, 0, 0);
    lv_obj_set_style_pad_row(_kb, PDA2_KB_GAP, 0);
    lv_obj_set_style_pad_column(_kb, PDA2_KB_GAP, 0);

    lv_obj_set_style_bg_color(_kb, lv_color_hex(0x2C3A57), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(_kb, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_radius(_kb, 6, LV_PART_ITEMS);
    lv_obj_set_style_border_width(_kb, 0, LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(_kb, 0, LV_PART_ITEMS);
    lv_obj_set_style_text_color(_kb, lv_color_hex(0xffffff), LV_PART_ITEMS);
    lv_obj_set_style_text_font(_kb, PDA.Fonts.get(16), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(_kb, lv_color_hex(0x4A5A7C), (lv_style_selector_t)LV_PART_ITEMS | (lv_style_selector_t)LV_STATE_PRESSED);
}

// Прямоугольник клавиши по id (по карте текущей страницы). Ширина — число в ctrl-карте
// (младшие 4 бита), в ряду: avail = W - (n-1)*gap, клавиша = avail*units/total.
bool Keyboard_Class::_key_rect(uint32_t id, int& x, int& y, int& w, int& h) {
    const char* const* map; const lv_buttonmatrix_ctrl_t* ctl;
    switch (_page) {
        case P_RU_LO:  map = MAP_RU_LO;  ctl = CTRL_RU;  break;
        case P_RU_UP:  map = MAP_RU_UP;  ctl = CTRL_RU;  break;
        case P_EN_LO:  map = MAP_EN_LO;  ctl = CTRL_EN;  break;
        case P_EN_UP:  map = MAP_EN_UP;  ctl = CTRL_EN;  break;
        case P_NUM_RU: map = MAP_NUM_RU; ctl = CTRL_NUM; break;
        case P_NUM_EN: map = MAP_NUM_EN; ctl = CTRL_NUM; break;
        default:       map = MAP_PAD;    ctl = CTRL_PAD; break;
    }
    const int W = PDA2_SCREEN_W, H = PDA2_KB_H, G = PDA2_KB_GAP;
    int rows = 1;
    for (int i = 0; map[i][0] != '\0'; i++) if (map[i][0] == '\n') rows++;
    const int key_h = (H - (rows - 1) * G) / rows;

    uint32_t b = 0;
    int r = 0, i = 0;
    while (map[i][0] != '\0') {
        int j = i, cnt = 0, units = 0;
        const uint32_t b0 = b;
        while (map[j][0] != '\0' && map[j][0] != '\n') { units += (int)ctl[b0 + cnt] & 0x0F; cnt++; j++; }
        if (id >= b0 && id < b0 + (uint32_t)cnt) {
            const int avail = W - (cnt - 1) * G;
            int pre = 0;
            for (uint32_t k = b0; k < id; k++) pre += (int)ctl[k] & 0x0F;
            const int u = (int)ctl[id] & 0x0F;
            x = (int)(id - b0) * G + avail * pre / units;
            w = avail * (pre + u) / units - avail * pre / units;
            y = PDA2_KB_Y + r * (key_h + G);
            h = key_h;
            return true;
        }
        b = b0 + cnt;
        i = j;
        if (map[i][0] == '\n') i++;
        r++;
    }
    return false;
}