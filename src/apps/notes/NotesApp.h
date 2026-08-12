// // ─────────────────────────────────────────────────────────────
// // apps/notes/NotesApp.h  —  NotesApp v1.0.2  (PDA 2)
// //
// // Изменения v1.0.2:
// //   - NOTES_MAX / MAX_FILES / W / H → из pda2_config.h
// //   - _drop_cb[8] → _drop_cb[16]  (переполнение при File: 5 пунктов)
// //   - Select All: lv_textarea_set_cursor_pos(END) вместо пустого set_text
// //   - _lbl_chars / _lbl_fname: bg_color зафиксирован через _sb_bg_color
// //   - _find_result задокументирован как byte offset (ASCII only)
// // ─────────────────────────────────────────────────────────────
// #pragma once
// #include <PDA2.h>

// class NotesApp : public PDA2App {
// public:
//     NotesApp() { name = "Notes"; }

//     void onInit()  override;
//     void onOpen()  override;
//     void onClose() override;
//     void onTick(uint32_t delta_ms) override;

// private:
//     // ── Константы ────────────────────────────────────────────
//     // Размеры — из pda2_config.h, не хардкодим
//     static constexpr int W         = PDA2_SCREEN_W;
//     static constexpr int H         = PDA2_SCREEN_H;
//     static constexpr int NOTES_MAX = PDA2_NOTES_MAX_SIZE;
//     static constexpr int MAX_FILES = PDA2_NOTES_MAX_FILES;

//     static constexpr int H_MENU      = 28;
//     static constexpr int H_STAT      = 22;
//     static constexpr int H_KB        = 160;
//     static constexpr int H_FIND      = 36;    // только строка Find
//     static constexpr int H_REPLACE   = 96;    // Find + Replace + кнопки
//     static constexpr int DROP_W      = 150;
//     static constexpr int DROP_ITEM_H = 30;

//     // ── Цвета ────────────────────────────────────────────────
//     static constexpr uint32_t BG       = 0x1a1a2e;
//     static constexpr uint32_t BG_MENU  = 0x12122a;
//     static constexpr uint32_t BG_TA    = 0x0d0d1f;
//     static constexpr uint32_t BG_DROP  = 0x0f0f1f;
//     static constexpr uint32_t C_BORDER = 0x2a2a4a;
//     static constexpr uint32_t C_TEXT   = 0xffffff;
//     static constexpr uint32_t C_DIM    = 0x888888;
//     static constexpr uint32_t C_GREEN  = 0x2a6a2a;
//     static constexpr uint32_t C_RED    = 0x6a2a2a;
//     static constexpr uint32_t C_BLUE   = 0x2a2a5a;

//     // ── Главный UI ───────────────────────────────────────────
//     lv_obj_t* _menubar      = nullptr;
//     lv_obj_t* _btn_file     = nullptr;
//     lv_obj_t* _btn_edit     = nullptr;
//     lv_obj_t* _btn_view     = nullptr;
//     lv_obj_t* _textarea     = nullptr;
//     lv_obj_t* _lbl_loading  = nullptr;
//     lv_obj_t* _keyboard     = nullptr;
//     lv_obj_t* _statusbar    = nullptr;
//     lv_obj_t* _lbl_chars    = nullptr;   // dynamic → ПРАВИЛО 10
//     lv_obj_t* _lbl_fname    = nullptr;   // dynamic → ПРАВИЛО 10

//     // ── Dropdown ─────────────────────────────────────────────
//     lv_obj_t* _drop_blocker = nullptr;
//     lv_obj_t* _drop_panel   = nullptr;
//     int       _drop_which   = 0;         // 1=File 2=Edit 3=View

//     struct DropCbData { NotesApp* app; int item; };
//     // [0..2]  — кнопки меню (File/Edit/View)
//     // [3..15] — пункты dropdown (макс. 13 активных пунктов)
//     DropCbData _drop_cb[16];

//     // ── Find / Replace ───────────────────────────────────────
//     lv_obj_t* _find_panel      = nullptr;
//     lv_obj_t* _ta_find         = nullptr;
//     lv_obj_t* _ta_replace      = nullptr;
//     lv_obj_t* _btn_replace     = nullptr;
//     lv_obj_t* _btn_replace_all = nullptr;
//     lv_obj_t* _lbl_repl        = nullptr;
//     bool      _find_vis        = false;
//     bool      _replace_mode    = false;
//     // ⚠️ byte offset в ASCII-тексте. lv_textarea_set_cursor_pos принимает
//     //    char offset — для ASCII совпадает. При кириллице потребуется пересчёт.
//     int       _find_result     = -1;

//     // ── File panel ───────────────────────────────────────────
//     lv_obj_t* _file_panel   = nullptr;
//     lv_obj_t* _ta_newfile   = nullptr;
//     lv_obj_t* _roller_files = nullptr;
//     enum class FilePanelMode { OPEN, SAVE_AS };
//     FilePanelMode _file_mode = FilePanelMode::OPEN;

//     // ── Состояние ────────────────────────────────────────────
//     char  _cur_file[64]    = "/note.txt";
//     char  _files[MAX_FILES][32];
//     int   _file_count      = 0;
//     char* _pending_buf     = nullptr;
//     bool  _word_wrap       = true;
//     bool  _statusbar_on    = true;
//     bool  _kb_vis          = false;

//     // ── State machine загрузки (ПРАВИЛО 6 — lv_timer запрещены) ──
//     enum class LoadSt { IDLE, SET_TEXT, FIX_CURSOR };
//     LoadSt   _load_st  = LoadSt::IDLE;
//     uint32_t _load_acc = 0;

//     // ── Build ────────────────────────────────────────────────
//     void _buildUI();
//     void _buildMenubar();
//     void _buildTextarea();
//     void _buildStatusbar();
//     void _buildFindPanel();
//     void _buildFilePanel();
//     void _styleBtn(lv_obj_t* btn, uint32_t color);

//     // ── Dropdown ─────────────────────────────────────────────
//     void _openDropdown(int which, lv_obj_t* anchor);
//     void _closeDropdown();
//     void _onDropItem(int which, int item);

//     // ── File actions ─────────────────────────────────────────
//     void _doNew();
//     void _doSave();
//     void _startLoad();
//     void _tickLoad(uint32_t delta_ms);

//     // ── Find / Replace ───────────────────────────────────────
//     void _showFindPanel(bool replace_mode);
//     void _hideFindPanel();
//     void _doFind(bool forward);
//     void _doReplace();
//     void _doReplaceAll();

//     // ── File panel ───────────────────────────────────────────
//     void _showFilePanel(FilePanelMode mode);
//     void _hideFilePanel();
//     void _applyFilePanel();
//     void _scanFiles();
//     void _updateRoller();

//     // ── Layout & helpers ─────────────────────────────────────
//     void _resizeTextarea();
//     void _showKb(lv_obj_t* ta);
//     void _hideKb();
//     void _updateStatusbar();
//     void _updateFilenameLabel();

//     // ── FS ───────────────────────────────────────────────────
//     char* _fsLoad(const char* path);
//     bool  _fsSave(const char* path, const char* text);
//     static bool _isTextExt(const char* fn);
// };