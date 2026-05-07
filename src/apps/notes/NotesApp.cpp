// ─────────────────────────────────────────────────────────────
// apps/notes/NotesApp.cpp  —  NotesApp v1.0.2  (PDA 2)
//
// Изменения v1.0.2:
//   - NOTES_MAX / MAX_FILES / W / H → из pda2_config.h
//   - _drop_cb[8] → _drop_cb[16]  (переполнение при File: 5 пунктов)
//   - Select All: lv_textarea_set_cursor_pos(LV_TEXTAREA_CURSOR_LAST)
//   - _lbl_chars, _lbl_fname: bg_color явно через BG_MENU (TEAR-1)
//   - Find panel H_FIND: убран лишний отступ снизу
//
// Важно:
//   — lv_timer запрещены (ПРАВИЛО 6). Загрузка через state machine в onTick.
//   — SPIFFS используется напрямую: Fs_Class предоставляет только .ok().
//     При расширении Fs_Class — перенести _fsLoad/_fsSave туда.
//   — Динамические лейблы: bg_opa=COVER + bg_color (ПРАВИЛО 10).
// ─────────────────────────────────────────────────────────────
#include "NotesApp.h"
#include "SPIFFS.h"
#include <string.h>
#include <stdlib.h>

// ─────────────────────────────────────────────────────────────
//  FS
// ─────────────────────────────────────────────────────────────

char* NotesApp::_fsLoad(const char* path) {
    if (!PDA.Fs.ok()) return nullptr;
    File f = SPIFFS.open(path, "r");
    if (!f) return nullptr;
    size_t sz = f.size();
    if (sz > (size_t)NOTES_MAX) sz = NOTES_MAX;
    char* buf = (char*)malloc(sz + 1);
    if (buf) { f.read((uint8_t*)buf, sz); buf[sz] = '\0'; }
    f.close();
    return buf;
}

bool NotesApp::_fsSave(const char* path, const char* text) {
    if (!PDA.Fs.ok()) return false;
    File f = SPIFFS.open(path, "w");
    if (!f) return false;
    if (text) f.print(text);
    f.close();
    return true;
}

bool NotesApp::_isTextExt(const char* fn) {
    static const char* exts[] = {
        ".txt", ".md",  ".log", ".cfg",  ".ini",
        ".json",".csv", ".xml",
        ".h",   ".hpp", ".c",   ".cpp",  ".ino",
        ".py",  ".js",  ".ts",
        ".sh",  ".bat", ".yaml",".yml",
        ".env", ".toml",".htm", ".html",
        nullptr
    };
    const char* dot = strrchr(fn, '.');
    if (!dot) return false;
    for (int i = 0; exts[i]; i++)
        if (strcasecmp(dot, exts[i]) == 0) return true;
    return false;
}

// ─────────────────────────────────────────────────────────────
//  Lifecycle
// ─────────────────────────────────────────────────────────────

void NotesApp::onInit() {
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(BG), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    _buildUI();
    PDA_LOGI("notes", "Init OK");
}

void NotesApp::onOpen() {
    _closeDropdown();
    _hideFindPanel();
    _hideKb();
    _hideFilePanel();
    _updateFilenameLabel();
    _updateStatusbar();
    _startLoad();
    PDA_LOGI("notes", "Opened: %s", _cur_file);
}

void NotesApp::onClose() {
    _doSave();
    _closeDropdown();
    _hideFindPanel();
    _hideKb();
    _hideFilePanel();
    _load_st  = LoadSt::IDLE;
    _load_acc = 0;
    if (_pending_buf) { free(_pending_buf); _pending_buf = nullptr; }
    PDA_LOGI("notes", "Closed");
}

void NotesApp::onTick(uint32_t delta_ms) {
    _tickLoad(delta_ms);
}

// ─────────────────────────────────────────────────────────────
//  Build UI
// ─────────────────────────────────────────────────────────────

void NotesApp::_buildUI() {
    _buildMenubar();
    _buildTextarea();
    _buildStatusbar();
    _buildFindPanel();
    _buildFilePanel();
    _resizeTextarea();
}

void NotesApp::_styleBtn(lv_obj_t* btn, uint32_t color) {
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 4, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
}

// ── Menubar ───────────────────────────────────────────────────
void NotesApp::_buildMenubar() {
    _menubar = lv_obj_create(screen);
    lv_obj_set_size(_menubar, W, H_MENU);
    lv_obj_set_pos(_menubar, 0, 0);
    lv_obj_set_style_bg_color(_menubar, lv_color_hex(BG_MENU), 0);
    lv_obj_set_style_bg_opa(_menubar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(_menubar, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_border_side(_menubar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(_menubar, 1, 0);
    lv_obj_set_style_pad_all(_menubar, 0, 0);
    lv_obj_set_style_radius(_menubar, 0, 0);
    lv_obj_remove_flag(_menubar, LV_OBJ_FLAG_SCROLLABLE);

    const char*  labels[] = { "File", "Edit", "View" };
    lv_obj_t**   ptrs[]   = { &_btn_file, &_btn_edit, &_btn_view };
    const int    xpos[]   = { 0, 107, 214 };
    const int    widths[] = { 107, 107, 106 };

    for (int i = 0; i < 3; i++) {
        lv_obj_t* btn = lv_button_create(_menubar);
        *ptrs[i] = btn;
        lv_obj_set_size(btn, widths[i], H_MENU);
        lv_obj_set_pos(btn, xpos[i], 0);
        _styleBtn(btn, BG_MENU);
        lv_obj_set_style_radius(btn, 0, 0);

        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xcccccc), 0);
        lv_obj_center(lbl);

        _drop_cb[i] = { this, i + 1 };
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            auto* d = static_cast<DropCbData*>(lv_event_get_user_data(e));
            lv_obj_t* anchor = nullptr;
            switch (d->item) {
                case 1: anchor = d->app->_btn_file; break;
                case 2: anchor = d->app->_btn_edit; break;
                case 3: anchor = d->app->_btn_view; break;
            }
            d->app->_openDropdown(d->item, anchor);
        }, LV_EVENT_CLICKED, &_drop_cb[i]);
    }
}

// ── Textarea + keyboard + loading label ───────────────────────
void NotesApp::_buildTextarea() {
    _textarea = lv_textarea_create(screen);
    lv_obj_set_pos(_textarea, 0, H_MENU);
    lv_textarea_set_max_length(_textarea, NOTES_MAX);
    lv_textarea_set_placeholder_text(_textarea, "Type here...");
    lv_obj_set_style_bg_color(_textarea, lv_color_hex(BG_TA), 0);
    lv_obj_set_style_bg_opa(_textarea, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(_textarea, lv_color_hex(C_TEXT), 0);
    lv_obj_set_style_border_width(_textarea, 0, 0);
    lv_obj_set_style_radius(_textarea, 0, 0);
    lv_obj_set_style_pad_all(_textarea, 6, 0);
    lv_textarea_set_one_line(_textarea, false);

    lv_obj_add_event_cb(_textarea, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_showKb(
            static_cast<NotesApp*>(lv_event_get_user_data(e))->_textarea);
    }, LV_EVENT_FOCUSED, this);

    lv_obj_add_event_cb(_textarea, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_hideKb();
    }, LV_EVENT_DEFOCUSED, this);

    lv_obj_add_event_cb(_textarea, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_updateStatusbar();
    }, LV_EVENT_VALUE_CHANGED, this);

    // Loading overlay
    _lbl_loading = lv_label_create(screen);
    lv_label_set_text(_lbl_loading, LV_SYMBOL_REFRESH "  Loading...");
    lv_obj_set_style_text_color(_lbl_loading, lv_color_hex(0xffdd00), 0);
    lv_obj_set_style_bg_color(_lbl_loading, lv_color_hex(BG_TA), 0);
    lv_obj_set_style_bg_opa(_lbl_loading, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(_lbl_loading, 12, 0);
    lv_obj_set_style_radius(_lbl_loading, 8, 0);
    lv_obj_align(_lbl_loading, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(_lbl_loading, LV_OBJ_FLAG_HIDDEN);

    // Keyboard
    // ⚠️ LVGL 9.2.1: lv_keyboard = buttonmatrix. Без явных стилей на
    //    LV_PART_ITEMS рендерится как пустой прямоугольник (дефолт темы).
    _keyboard = lv_keyboard_create(screen);
    lv_obj_set_size(_keyboard, W, H_KB);
    // Фон контейнера
    lv_obj_set_style_bg_color(_keyboard, lv_color_hex(0x1e1e3a), 0);
    lv_obj_set_style_bg_opa(_keyboard, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_keyboard, 0, 0);
    lv_obj_set_style_radius(_keyboard, 0, 0);
    lv_obj_set_style_pad_all(_keyboard, 4, 0);
    lv_obj_set_style_pad_gap(_keyboard, 4, 0);
    // Кнопки (LV_PART_ITEMS)
    lv_obj_set_style_bg_color(_keyboard, lv_color_hex(0x2a2a4a), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(_keyboard, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_text_color(_keyboard, lv_color_hex(0xffffff), LV_PART_ITEMS);
    lv_obj_set_style_border_color(_keyboard, lv_color_hex(0x1a1a2e), LV_PART_ITEMS);
    lv_obj_set_style_border_width(_keyboard, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(_keyboard, 4, LV_PART_ITEMS);
    // Нажатое состояние
    lv_obj_set_style_bg_color(_keyboard, lv_color_hex(0x4a4a8a),
                              LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(_keyboard, LV_OPA_COVER,
                            LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(_keyboard, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_hideKb();
    }, LV_EVENT_READY, this);
    lv_obj_add_event_cb(_keyboard, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_hideKb();
    }, LV_EVENT_CANCEL, this);
}

// ── Statusbar ─────────────────────────────────────────────────
void NotesApp::_buildStatusbar() {
    _statusbar = lv_obj_create(screen);
    lv_obj_set_size(_statusbar, W, H_STAT);
    lv_obj_set_pos(_statusbar, 0, H - H_STAT);
    lv_obj_set_style_bg_color(_statusbar, lv_color_hex(BG_MENU), 0);
    lv_obj_set_style_bg_opa(_statusbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(_statusbar, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_border_side(_statusbar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(_statusbar, 1, 0);
    lv_obj_set_style_pad_all(_statusbar, 0, 0);
    lv_obj_set_style_radius(_statusbar, 0, 0);
    lv_obj_remove_flag(_statusbar, LV_OBJ_FLAG_SCROLLABLE);

    // [FIX v1.0.2] bg_color задан явно на обоих лейблах (TEAR-1)
    _lbl_chars = lv_label_create(_statusbar);
    lv_label_set_text(_lbl_chars, "0 chars");
    lv_obj_set_style_text_color(_lbl_chars, lv_color_hex(C_DIM), 0);
    lv_obj_set_style_bg_color(_lbl_chars, lv_color_hex(BG_MENU), 0);
    lv_obj_set_style_bg_opa(_lbl_chars, LV_OPA_COVER, 0);
    lv_obj_align(_lbl_chars, LV_ALIGN_LEFT_MID, 6, 0);

    _lbl_fname = lv_label_create(_statusbar);
    lv_label_set_text(_lbl_fname, "note.txt");
    lv_obj_set_style_text_color(_lbl_fname, lv_color_hex(C_DIM), 0);
    lv_obj_set_style_bg_color(_lbl_fname, lv_color_hex(BG_MENU), 0);
    lv_obj_set_style_bg_opa(_lbl_fname, LV_OPA_COVER, 0);
    lv_obj_align(_lbl_fname, LV_ALIGN_RIGHT_MID, -6, 0);
}

// ── Find / Replace панель ─────────────────────────────────────
void NotesApp::_buildFindPanel() {
    _find_panel = lv_obj_create(screen);
    lv_obj_set_size(_find_panel, W, H_FIND);
    lv_obj_set_style_bg_color(_find_panel, lv_color_hex(BG_MENU), 0);
    lv_obj_set_style_bg_opa(_find_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(_find_panel, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_border_side(_find_panel, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(_find_panel, 1, 0);
    lv_obj_set_style_pad_hor(_find_panel, 6, 0);
    lv_obj_set_style_pad_ver(_find_panel, 4, 0);
    lv_obj_set_style_radius(_find_panel, 0, 0);
    lv_obj_remove_flag(_find_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_find_panel, LV_OBJ_FLAG_HIDDEN);

    // Row 1: "Find:" + ta_find + < > X
    lv_obj_t* lbl_f = lv_label_create(_find_panel);
    lv_label_set_text(lbl_f, "Find:");
    lv_obj_set_style_text_color(lbl_f, lv_color_hex(C_DIM), 0);
    lv_obj_set_pos(lbl_f, 0, 4);

    _ta_find = lv_textarea_create(_find_panel);
    lv_obj_set_size(_ta_find, 162, 26);
    lv_obj_set_pos(_ta_find, 42, 0);
    lv_textarea_set_one_line(_ta_find, true);
    lv_obj_set_style_bg_color(_ta_find, lv_color_hex(BG_TA), 0);
    lv_obj_set_style_bg_opa(_ta_find, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(_ta_find, lv_color_hex(C_TEXT), 0);
    lv_obj_set_style_border_color(_ta_find, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_radius(_ta_find, 4, 0);
    lv_obj_set_style_pad_ver(_ta_find, 3, 0);
    lv_obj_set_style_pad_hor(_ta_find, 4, 0);
    lv_obj_add_event_cb(_ta_find, [](lv_event_t* e) {
        auto* app = static_cast<NotesApp*>(lv_event_get_user_data(e));
        app->_showKb(app->_ta_find);
    }, LV_EVENT_FOCUSED, this);
    lv_obj_add_event_cb(_ta_find, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_hideKb();
    }, LV_EVENT_DEFOCUSED, this);

    const int    bx[] = { 208, 240, 272 };
    const char*  bl[] = { "<", ">", "X" };
    const uint32_t bc[] = { C_BLUE, C_BLUE, C_RED };
    lv_obj_t* find_btns[3] = {};
    for (int i = 0; i < 3; i++) {
        lv_obj_t* b = lv_button_create(_find_panel);
        find_btns[i] = b;
        lv_obj_set_size(b, 28, 26);
        lv_obj_set_pos(b, bx[i], 0);
        _styleBtn(b, bc[i]);
        lv_obj_t* l = lv_label_create(b);
        lv_label_set_text(l, bl[i]);
        lv_obj_set_style_text_color(l, lv_color_hex(C_TEXT), 0);
        lv_obj_center(l);
    }
    lv_obj_add_event_cb(find_btns[0], [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_doFind(false);
    }, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(find_btns[1], [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_doFind(true);
    }, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(find_btns[2], [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_hideFindPanel();
    }, LV_EVENT_CLICKED, this);

    // Row 2: Replace (скрыты по умолчанию)
    _lbl_repl = lv_label_create(_find_panel);
    lv_label_set_text(_lbl_repl, "Repl:");
    lv_obj_set_style_text_color(_lbl_repl, lv_color_hex(C_DIM), 0);
    lv_obj_set_pos(_lbl_repl, 0, 36);
    lv_obj_add_flag(_lbl_repl, LV_OBJ_FLAG_HIDDEN);

    _ta_replace = lv_textarea_create(_find_panel);
    lv_obj_set_size(_ta_replace, 260, 26);
    lv_obj_set_pos(_ta_replace, 42, 32);
    lv_textarea_set_one_line(_ta_replace, true);
    lv_textarea_set_placeholder_text(_ta_replace, "Replace with...");
    lv_obj_set_style_bg_color(_ta_replace, lv_color_hex(BG_TA), 0);
    lv_obj_set_style_bg_opa(_ta_replace, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(_ta_replace, lv_color_hex(C_TEXT), 0);
    lv_obj_set_style_border_color(_ta_replace, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_radius(_ta_replace, 4, 0);
    lv_obj_set_style_pad_ver(_ta_replace, 3, 0);
    lv_obj_set_style_pad_hor(_ta_replace, 4, 0);
    lv_obj_add_flag(_ta_replace, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(_ta_replace, [](lv_event_t* e) {
        auto* app = static_cast<NotesApp*>(lv_event_get_user_data(e));
        app->_showKb(app->_ta_replace);
    }, LV_EVENT_FOCUSED, this);
    lv_obj_add_event_cb(_ta_replace, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_hideKb();
    }, LV_EVENT_DEFOCUSED, this);

    _btn_replace = lv_button_create(_find_panel);
    lv_obj_set_size(_btn_replace, 94, 28);
    lv_obj_set_pos(_btn_replace, 0, 64);
    _styleBtn(_btn_replace, C_GREEN);
    lv_obj_center(lv_label_create(_btn_replace));
    lv_label_set_text(lv_obj_get_child(_btn_replace, 0), "Replace");
    lv_obj_set_style_text_color(lv_obj_get_child(_btn_replace, 0), lv_color_hex(C_TEXT), 0);
    lv_obj_add_flag(_btn_replace, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(_btn_replace, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_doReplace();
    }, LV_EVENT_CLICKED, this);

    _btn_replace_all = lv_button_create(_find_panel);
    lv_obj_set_size(_btn_replace_all, 120, 28);
    lv_obj_set_pos(_btn_replace_all, 100, 64);
    _styleBtn(_btn_replace_all, C_GREEN);
    lv_obj_center(lv_label_create(_btn_replace_all));
    lv_label_set_text(lv_obj_get_child(_btn_replace_all, 0), "Replace All");
    lv_obj_set_style_text_color(lv_obj_get_child(_btn_replace_all, 0), lv_color_hex(C_TEXT), 0);
    lv_obj_add_flag(_btn_replace_all, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(_btn_replace_all, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_doReplaceAll();
    }, LV_EVENT_CLICKED, this);
}

// ── File panel ────────────────────────────────────────────────
void NotesApp::_buildFilePanel() {
    _file_panel = lv_obj_create(screen);
    lv_obj_set_size(_file_panel, W, H);
    lv_obj_set_pos(_file_panel, 0, 0);
    lv_obj_set_style_bg_color(_file_panel, lv_color_hex(0x0f0f1a), 0);
    lv_obj_set_style_bg_opa(_file_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_file_panel, 0, 0);
    lv_obj_set_style_pad_all(_file_panel, 12, 0);
    lv_obj_set_style_radius(_file_panel, 0, 0);
    lv_obj_remove_flag(_file_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_file_panel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* title = lv_label_create(_file_panel);
    lv_label_set_text(title, "Open / Save As");
    lv_obj_set_style_text_color(title, lv_color_hex(C_TEXT), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* lbl_name = lv_label_create(_file_panel);
    lv_label_set_text(lbl_name, "File name:");
    lv_obj_set_style_text_color(lbl_name, lv_color_hex(C_DIM), 0);
    lv_obj_set_pos(lbl_name, 0, 30);

    _ta_newfile = lv_textarea_create(_file_panel);
    lv_obj_set_size(_ta_newfile, W - 24, 34);
    lv_obj_set_pos(_ta_newfile, 0, 52);
    lv_textarea_set_one_line(_ta_newfile, true);
    lv_textarea_set_placeholder_text(_ta_newfile, "e.g. mynote.txt");
    lv_obj_set_style_bg_color(_ta_newfile, lv_color_hex(BG_TA), 0);
    lv_obj_set_style_bg_opa(_ta_newfile, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(_ta_newfile, lv_color_hex(C_TEXT), 0);
    lv_obj_set_style_border_color(_ta_newfile, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_radius(_ta_newfile, 4, 0);
    lv_obj_set_style_pad_all(_ta_newfile, 6, 0);
    lv_obj_add_event_cb(_ta_newfile, [](lv_event_t* e) {
        auto* app = static_cast<NotesApp*>(lv_event_get_user_data(e));
        app->_showKb(app->_ta_newfile);
    }, LV_EVENT_FOCUSED, this);
    lv_obj_add_event_cb(_ta_newfile, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_hideKb();
    }, LV_EVENT_DEFOCUSED, this);

    lv_obj_t* lbl_pick = lv_label_create(_file_panel);
    lv_label_set_text(lbl_pick, "Or select existing:");
    lv_obj_set_style_text_color(lbl_pick, lv_color_hex(C_DIM), 0);
    lv_obj_set_pos(lbl_pick, 0, 96);

    _roller_files = lv_roller_create(_file_panel);
    lv_obj_set_size(_roller_files, W - 24, 80);
    lv_obj_set_pos(_roller_files, 0, 116);
    lv_roller_set_options(_roller_files, "note.txt", LV_ROLLER_MODE_NORMAL);
    lv_obj_set_style_bg_color(_roller_files, lv_color_hex(BG_TA), 0);
    lv_obj_set_style_bg_opa(_roller_files, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(_roller_files, lv_color_hex(C_TEXT), 0);
    lv_obj_set_style_border_color(_roller_files, lv_color_hex(C_BORDER), 0);

    lv_obj_t* btn_ok = lv_button_create(_file_panel);
    lv_obj_set_size(btn_ok, 130, 40);
    lv_obj_set_pos(btn_ok, 0, 212);
    _styleBtn(btn_ok, C_GREEN);
    lv_obj_t* lok = lv_label_create(btn_ok);
    lv_label_set_text(lok, "OK");
    lv_obj_set_style_text_color(lok, lv_color_hex(C_TEXT), 0);
    lv_obj_center(lok);
    lv_obj_add_event_cb(btn_ok, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_applyFilePanel();
    }, LV_EVENT_CLICKED, this);

    lv_obj_t* btn_cancel = lv_button_create(_file_panel);
    lv_obj_set_size(btn_cancel, 130, 40);
    lv_obj_set_pos(btn_cancel, 142, 212);
    _styleBtn(btn_cancel, C_RED);
    lv_obj_t* lcan = lv_label_create(btn_cancel);
    lv_label_set_text(lcan, "Cancel");
    lv_obj_set_style_text_color(lcan, lv_color_hex(C_TEXT), 0);
    lv_obj_center(lcan);
    lv_obj_add_event_cb(btn_cancel, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_hideFilePanel();
    }, LV_EVENT_CLICKED, this);
}

// ─────────────────────────────────────────────────────────────
//  Layout
// ─────────────────────────────────────────────────────────────

void NotesApp::_resizeTextarea() {
    if (!_textarea) return;

    int panel_h = _find_vis ? (_replace_mode ? H_REPLACE : H_FIND) : 0;
    int kb_h    = _kb_vis   ? H_KB : 0;
    int ta_h    = H - H_MENU - H_STAT - panel_h - kb_h;

    lv_obj_set_size(_textarea, W, ta_h);

    if (_find_panel) {
        lv_obj_set_pos(_find_panel, 0, H_MENU + ta_h);
        if (_find_vis) lv_obj_remove_flag(_find_panel, LV_OBJ_FLAG_HIDDEN);
        else           lv_obj_add_flag(_find_panel,    LV_OBJ_FLAG_HIDDEN);
    }

    if (_keyboard) {
        lv_obj_set_pos(_keyboard, 0, H - H_STAT - H_KB);
        if (_kb_vis) lv_obj_remove_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
        else         lv_obj_add_flag(_keyboard,    LV_OBJ_FLAG_HIDDEN);
    }
}

// ─────────────────────────────────────────────────────────────
//  Dropdown
// ─────────────────────────────────────────────────────────────

void NotesApp::_openDropdown(int which, lv_obj_t* anchor) {
    _closeDropdown();
    _drop_which = which;

    lv_area_t area;
    lv_obj_get_coords(anchor, &area);
    int dx = area.x1;
    int dy = area.y2;
    if (dx + DROP_W > W) dx = W - DROP_W;

    // Прозрачный blocker перехватывает клик вне dropdown
    _drop_blocker = lv_obj_create(screen);
    lv_obj_set_size(_drop_blocker, W, H);
    lv_obj_set_pos(_drop_blocker, 0, 0);
    lv_obj_set_style_bg_opa(_drop_blocker, LV_OPA_0, 0);
    lv_obj_set_style_border_width(_drop_blocker, 0, 0);
    lv_obj_remove_flag(_drop_blocker, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(_drop_blocker, [](lv_event_t* e) {
        static_cast<NotesApp*>(lv_event_get_user_data(e))->_closeDropdown();
    }, LV_EVENT_CLICKED, this);

    _drop_panel = lv_obj_create(screen);
    lv_obj_set_style_bg_color(_drop_panel, lv_color_hex(BG_DROP), 0);
    lv_obj_set_style_bg_opa(_drop_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(_drop_panel, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_border_width(_drop_panel, 1, 0);
    lv_obj_set_style_pad_all(_drop_panel, 0, 0);
    lv_obj_set_style_radius(_drop_panel, 4, 0);
    lv_obj_remove_flag(_drop_panel, LV_OBJ_FLAG_SCROLLABLE);

    struct Item { const char* lbl; bool sep; bool dim; };

    Item file_items[] = {
        {"New",        false, false},
        {"Open...",    false, false},
        {"",           true,  false},
        {"Save",       false, false},
        {"Save As...", false, false},
        {"",           true,  false},
        {"Close",      false, false},
        {nullptr}
    };
    // Undo dim=true — недоступен (LVGL textarea не имеет Undo API)
    Item edit_items[] = {
        {"Undo",       false, true },
        {"",           true,  false},
        {"Select All", false, false},
        {"",           true,  false},
        {"Find...",    false, false},
        {"Replace...", false, false},
        {nullptr}
    };
    char vw0[24], vw1[24];
    snprintf(vw0, sizeof(vw0), "Word Wrap: %s",  _word_wrap    ? "ON" : "OFF");
    snprintf(vw1, sizeof(vw1), "Status Bar: %s", _statusbar_on ? "ON" : "OFF");
    Item view_items[] = {
        {vw0, false, false},
        {vw1, false, false},
        {nullptr}
    };

    Item* items = nullptr;
    if (which == 1) items = file_items;
    if (which == 2) items = edit_items;
    if (which == 3) items = view_items;
    if (!items) return;

    int y = 0, item_idx = 0;
    for (int i = 0; items[i].lbl != nullptr; i++) {
        if (items[i].sep) {
            lv_obj_t* sep = lv_obj_create(_drop_panel);
            lv_obj_set_size(sep, DROP_W - 2, 1);
            lv_obj_set_pos(sep, 1, y);
            lv_obj_set_style_bg_color(sep, lv_color_hex(C_BORDER), 0);
            lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(sep, 0, 0);
            y += 5;
        } else {
            lv_obj_t* btn = lv_button_create(_drop_panel);
            lv_obj_set_size(btn, DROP_W, DROP_ITEM_H);
            lv_obj_set_pos(btn, 0, y);
            _styleBtn(btn, BG_DROP);
            lv_obj_set_style_radius(btn, 0, 0);
            lv_obj_set_style_pad_left(btn, 10, 0);

            lv_obj_t* lbl = lv_label_create(btn);
            lv_label_set_text(lbl, items[i].lbl);
            lv_obj_set_style_text_color(lbl, lv_color_hex(items[i].dim ? 0x555555 : 0xcccccc), 0);
            lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

            if (!items[i].dim) {
                // [FIX v1.0.2] _drop_cb[16] — безопасный размер
                _drop_cb[3 + item_idx] = { this, item_idx };
                lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                    auto* d = static_cast<DropCbData*>(lv_event_get_user_data(e));
                    d->app->_onDropItem(d->app->_drop_which, d->item);
                }, LV_EVENT_CLICKED, &_drop_cb[3 + item_idx]);
                item_idx++;
            }
            y += DROP_ITEM_H;
        }
    }

    lv_obj_set_size(_drop_panel, DROP_W, y);
    lv_obj_set_pos(_drop_panel, dx, dy);
    lv_obj_move_foreground(_drop_panel);
}

void NotesApp::_closeDropdown() {
    if (_drop_panel)   { lv_obj_del(_drop_panel);   _drop_panel   = nullptr; }
    if (_drop_blocker) { lv_obj_del(_drop_blocker); _drop_blocker = nullptr; }
    _drop_which = 0;
}

void NotesApp::_onDropItem(int which, int item) {
    _closeDropdown();

    if (which == 1) {  // File: 0=New 1=Open 2=Save 3=SaveAs 4=Close
        switch (item) {
            case 0: _doNew(); break;
            case 1: _showFilePanel(FilePanelMode::OPEN); break;
            case 2: _doSave(); PDA.toast("Saved"); break;
            case 3: _showFilePanel(FilePanelMode::SAVE_AS); break;
            case 4: PDA.Apps.close(); break;
        }
    } else if (which == 2) {  // Edit: 0=SelectAll 1=Find 2=Replace
        switch (item) {
            case 0:
                // [FIX v1.0.2] LVGL textarea не имеет highlight selection.
                // Перемещаем курсор в конец — лучшее что можно сделать.
                if (_textarea)
                    lv_textarea_set_cursor_pos(_textarea, LV_TEXTAREA_CURSOR_LAST);
                break;
            case 1: _showFindPanel(false); break;
            case 2: _showFindPanel(true);  break;
        }
    } else if (which == 3) {  // View: 0=WordWrap 1=StatusBar
        switch (item) {
            case 0:
                _word_wrap = !_word_wrap;
                if (_textarea) lv_textarea_set_one_line(_textarea, !_word_wrap);
                break;
            case 1:
                _statusbar_on = !_statusbar_on;
                if (_statusbar) {
                    if (_statusbar_on) lv_obj_remove_flag(_statusbar, LV_OBJ_FLAG_HIDDEN);
                    else               lv_obj_add_flag(_statusbar,    LV_OBJ_FLAG_HIDDEN);
                }
                break;
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  File actions
// ─────────────────────────────────────────────────────────────

void NotesApp::_doNew() {
    _doSave();
    strncpy(_cur_file, "/untitled.txt", sizeof(_cur_file));
    if (_textarea) lv_textarea_set_text(_textarea, "");
    _updateFilenameLabel();
    _updateStatusbar();
    PDA_LOGI("notes", "New file");
}

void NotesApp::_doSave() {
    if (!_textarea) return;
    const char* text = lv_textarea_get_text(_textarea);
    if (_fsSave(_cur_file, text ? text : ""))
        PDA_LOGI("notes", "Saved: %s", _cur_file);
    else
        PDA_LOGW("notes", "Save failed: %s", _cur_file);
}

void NotesApp::_startLoad() {
    if (!_textarea || _load_st != LoadSt::IDLE) return;
    if (_pending_buf) { free(_pending_buf); _pending_buf = nullptr; }

    if (!SPIFFS.exists(_cur_file)) _fsSave(_cur_file, "");

    _pending_buf = _fsLoad(_cur_file);
    if (_lbl_loading) lv_obj_remove_flag(_lbl_loading, LV_OBJ_FLAG_HIDDEN);
    lv_textarea_set_text(_textarea, "");
    _load_st  = LoadSt::SET_TEXT;
    _load_acc = 0;
}

// State machine — lv_timer запрещены (ПРАВИЛО 6)
void NotesApp::_tickLoad(uint32_t delta_ms) {
    if (_load_st == LoadSt::IDLE) return;
    _load_acc += delta_ms;

    if (_load_st == LoadSt::SET_TEXT && _load_acc >= 100) {
        if (_pending_buf && _textarea) {
            lv_textarea_set_text(_textarea, _pending_buf);
            free(_pending_buf);
            _pending_buf = nullptr;
        }
        if (_lbl_loading) lv_obj_add_flag(_lbl_loading, LV_OBJ_FLAG_HIDDEN);
        _updateStatusbar();
        _load_st  = LoadSt::FIX_CURSOR;
        _load_acc = 0;
    }

    if (_load_st == LoadSt::FIX_CURSOR && _load_acc >= 50) {
        if (_textarea) {
            lv_textarea_set_cursor_pos(_textarea, 0);
            lv_obj_scroll_to_y(_textarea, 0, LV_ANIM_OFF);
        }
        _load_st = LoadSt::IDLE;
    }
}

// ─────────────────────────────────────────────────────────────
//  Find / Replace
// ─────────────────────────────────────────────────────────────

void NotesApp::_showFindPanel(bool replace_mode) {
    _replace_mode = replace_mode;
    _find_vis     = true;
    _find_result  = -1;

    if (replace_mode) {
        lv_obj_set_size(_find_panel, W, H_REPLACE);
        lv_obj_remove_flag(_ta_replace,      LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(_btn_replace,     LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(_btn_replace_all, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(_lbl_repl,        LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_set_size(_find_panel, W, H_FIND);
        lv_obj_add_flag(_ta_replace,         LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(_btn_replace,        LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(_btn_replace_all,    LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(_lbl_repl,           LV_OBJ_FLAG_HIDDEN);
    }

    if (_ta_find) lv_textarea_set_text(_ta_find, "");
    _resizeTextarea();
}

void NotesApp::_hideFindPanel() {
    _find_vis    = false;
    _find_result = -1;
    _hideKb();
    _resizeTextarea();
    _updateStatusbar();
}

void NotesApp::_doFind(bool forward) {
    if (!_textarea || !_ta_find) return;
    const char* needle   = lv_textarea_get_text(_ta_find);
    if (!needle || strlen(needle) == 0) return;
    const char* haystack = lv_textarea_get_text(_textarea);
    if (!haystack) return;

    int hay_len    = (int)strlen(haystack);
    int needle_len = (int)strlen(needle);
    if (needle_len == 0 || needle_len > hay_len) { PDA.toast("Not found"); return; }

    int start = _find_result;

    if (forward) {
        start = (start < 0) ? 0 : start + 1;
        for (int i = start; i <= hay_len - needle_len; i++) {
            if (strncasecmp(haystack + i, needle, needle_len) == 0) {
                _find_result = i;
                lv_textarea_set_cursor_pos(_textarea, i);
                lv_obj_scroll_to_y(_textarea, 0, LV_ANIM_OFF);
                _updateStatusbar();
                return;
            }
        }
    } else {
        start = (start < 0) ? hay_len : start - 1;
        for (int i = start; i >= 0; i--) {
            if (strncasecmp(haystack + i, needle, needle_len) == 0) {
                _find_result = i;
                lv_textarea_set_cursor_pos(_textarea, i);
                lv_obj_scroll_to_y(_textarea, 0, LV_ANIM_OFF);
                _updateStatusbar();
                return;
            }
        }
    }

    _find_result = -1;
    PDA.toast("Not found");
    _updateStatusbar();
}

void NotesApp::_doReplace() {
    if (!_textarea || _find_result < 0 || !_ta_find || !_ta_replace) return;
    const char* needle  = lv_textarea_get_text(_ta_find);
    const char* replace = lv_textarea_get_text(_ta_replace);
    if (!needle || !replace) return;

    const char* text = lv_textarea_get_text(_textarea);
    int tlen   = (int)strlen(text);
    int nlen   = (int)strlen(needle);
    int rlen   = (int)strlen(replace);
    int newlen = tlen - nlen + rlen;
    if (newlen < 0 || newlen > NOTES_MAX) return;

    char* buf = (char*)malloc(newlen + 1);
    if (!buf) return;
    memcpy(buf,                    text,                     _find_result);
    memcpy(buf + _find_result,     replace,                  rlen);
    memcpy(buf + _find_result + rlen, text + _find_result + nlen,
           tlen - _find_result - nlen);
    buf[newlen] = '\0';
    lv_textarea_set_text(_textarea, buf);
    free(buf);

    _find_result = -1;
    _updateStatusbar();
}

void NotesApp::_doReplaceAll() {
    if (!_textarea || !_ta_find || !_ta_replace) return;
    const char* needle  = lv_textarea_get_text(_ta_find);
    const char* replace = lv_textarea_get_text(_ta_replace);
    if (!needle || strlen(needle) == 0) return;

    const char* text = lv_textarea_get_text(_textarea);
    if (!text) return;

    int nlen  = (int)strlen(needle);
    int rlen  = replace ? (int)strlen(replace) : 0;
    int count = 0;
    const char* p = text;
    while ((p = strcasestr(p, needle)) != nullptr) { count++; p += nlen; }
    if (count == 0) { PDA.toast("Not found"); return; }

    int oldlen = (int)strlen(text);
    int newlen = oldlen + count * (rlen - nlen);
    if (newlen < 0 || newlen > NOTES_MAX) { PDA.toast("Too large"); return; }

    char* buf = (char*)malloc(newlen + 1);
    if (!buf) return;

    const char* src = text;
    char*       dst = buf;
    const char* hit;
    while ((hit = strcasestr(src, needle)) != nullptr) {
        int before = (int)(hit - src);
        memcpy(dst, src, before); dst += before;
        if (rlen) { memcpy(dst, replace, rlen); dst += rlen; }
        src = hit + nlen;
    }
    strcpy(dst, src);
    lv_textarea_set_text(_textarea, buf);
    free(buf);

    char msg[32];
    snprintf(msg, sizeof(msg), "Replaced: %d", count);
    PDA.toast(msg);
    _find_result = -1;
    _updateStatusbar();
}

// ─────────────────────────────────────────────────────────────
//  File panel
// ─────────────────────────────────────────────────────────────

void NotesApp::_scanFiles() {
    _file_count = 0;
    File root = SPIFFS.open("/");
    if (!root || !root.isDirectory()) {
        strncpy(_files[0], "note.txt", 31); _files[0][31] = '\0';
        _file_count = 1;
        return;
    }
    File f = root.openNextFile();
    while (f && _file_count < MAX_FILES) {
        if (!f.isDirectory()) {
            const char* name = f.name();
            const char* base = (name[0] == '/') ? name + 1 : name;
            if (_isTextExt(base)) {
                strncpy(_files[_file_count], base, 31);
                _files[_file_count][31] = '\0';
                _file_count++;
            }
        }
        f = root.openNextFile();
    }
    if (_file_count == 0) {
        strncpy(_files[0], "note.txt", 31); _files[0][31] = '\0';
        _file_count = 1;
    }
}

void NotesApp::_updateRoller() {
    if (!_roller_files) return;
    char opts[MAX_FILES * 33 + 1] = "";
    for (int i = 0; i < _file_count; i++) {
        if (i > 0) strcat(opts, "\n");
        strcat(opts, _files[i]);
    }
    lv_roller_set_options(_roller_files, opts, LV_ROLLER_MODE_NORMAL);
    const char* cur = (_cur_file[0] == '/') ? _cur_file + 1 : _cur_file;
    for (int i = 0; i < _file_count; i++) {
        if (strcmp(_files[i], cur) == 0) {
            lv_roller_set_selected(_roller_files, i, LV_ANIM_OFF);
            break;
        }
    }
}

void NotesApp::_showFilePanel(FilePanelMode mode) {
    _file_mode = mode;
    _scanFiles();
    _updateRoller();
    if (_ta_newfile) lv_textarea_set_text(_ta_newfile, "");
    lv_obj_remove_flag(_file_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(_file_panel);
}

void NotesApp::_hideFilePanel() {
    if (_file_panel) lv_obj_add_flag(_file_panel, LV_OBJ_FLAG_HIDDEN);
    _hideKb();
}

void NotesApp::_applyFilePanel() {
    char selected[64] = {};
    const char* typed = _ta_newfile ? lv_textarea_get_text(_ta_newfile) : "";

    if (typed && strlen(typed) > 0) {
        if (_isTextExt(typed)) snprintf(selected, sizeof(selected), "/%s", typed);
        else                   snprintf(selected, sizeof(selected), "/%s.txt", typed);
    } else if (_roller_files) {
        char picked[32];
        lv_roller_get_selected_str(_roller_files, picked, sizeof(picked));
        snprintf(selected, sizeof(selected), "/%s", picked);
    }

    if (strlen(selected) == 0) { _hideFilePanel(); return; }
    _hideFilePanel();

    if (_file_mode == FilePanelMode::OPEN) {
        _doSave();
        strncpy(_cur_file, selected, sizeof(_cur_file));
        _cur_file[sizeof(_cur_file) - 1] = '\0';
        _updateFilenameLabel();
        _startLoad();
        PDA_LOGI("notes", "Open: %s", _cur_file);
    } else {
        strncpy(_cur_file, selected, sizeof(_cur_file));
        _cur_file[sizeof(_cur_file) - 1] = '\0';
        _doSave();
        _updateFilenameLabel();
        PDA.toast("Saved As");
        PDA_LOGI("notes", "Save As: %s", _cur_file);
    }
}

// ─────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────

void NotesApp::_updateStatusbar() {
    if (!_lbl_chars || !_textarea) return;
    const char* txt = lv_textarea_get_text(_textarea);
    int len = txt ? (int)strlen(txt) : 0;
    if (_find_vis && _find_result >= 0)
        lv_label_set_text_fmt(_lbl_chars, "Found at: %d", _find_result);
    else
        lv_label_set_text_fmt(_lbl_chars, "%d chars", len);
}

void NotesApp::_updateFilenameLabel() {
    if (!_lbl_fname) return;
    const char* name = (_cur_file[0] == '/') ? _cur_file + 1 : _cur_file;
    lv_label_set_text(_lbl_fname, name);
}

void NotesApp::_showKb(lv_obj_t* ta) {
    if (!_keyboard) return;
    lv_keyboard_set_textarea(_keyboard, ta);
    _kb_vis = true;
    _resizeTextarea();
}

void NotesApp::_hideKb() {
    _kb_vis = false;
    _resizeTextarea();
}