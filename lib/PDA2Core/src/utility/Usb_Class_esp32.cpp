// ════════════════════════════════════════════════════════
//  PDA 2 — Usb_Class_esp32.cpp
//  USB Host: клавиатура + мышь + геймпад через OTG порт.
//  Библиотека: esp32beans/ESP32_USB_Host_HID (C API)
//  Потоки: колбэки библиотеки стреляют из USB task →
//          буфер → main task → колбэки приложения.
//  Компилируется, только если PDA2_SIM НЕ определён —
//  в сборке симулятора тело пустое (см. Usb_Class_sim.cpp).
// ════════════════════════════════════════════════════════

#include "Usb_Class.h"
#include "../pda2_log.h"
#include "../pda2_platform.h"

#ifndef PDA2_SIM

// ESP-IDF USB Host + HID
#include "usb/usb_host.h"
#include "hid_host.h"
#include "hid_usage_keyboard.h"
#include "hid_usage_mouse.h"

// ── USB Host daemon task ─────────────────────────────────
static void usb_host_task(void* arg) {
    while (true) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
    }
}

// ── Статический указатель на экземпляр ──────────────────
static pda2::Usb_Class* _instance = nullptr;

// ── Предыдущий репорт клавиатуры (для diff нажатий) ─────
static hid_keyboard_input_report_boot_t _last_kb = {};

// ════════════════════════════════════════════════════════
//  Колбэк HID интерфейса — данные от устройства
//  Вызывается из USB task библиотеки.
// ════════════════════════════════════════════════════════
static void hid_interface_cb(hid_host_device_handle_t handle,
                              const hid_host_interface_event_t event,
                              void* arg)
{
    if (!_instance) return;
    uint8_t data[16] = {};
    size_t  len      = 0;

    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
        hid_host_device_get_raw_input_report_data(handle, data, sizeof(data), &len);

        if ((uintptr_t)arg == HID_PROTOCOL_KEYBOARD &&
            len >= sizeof(hid_keyboard_input_report_boot_t)) {
            auto* rep = reinterpret_cast<hid_keyboard_input_report_boot_t*>(data);
            for (int i = 0; i < 6; i++) {
                uint8_t kc = _last_kb.key[i];
                if (kc == 0) continue;
                bool still = false;
                for (int j = 0; j < 6; j++)
                    if (rep->key[j] == kc) { still = true; break; }
                if (!still)
                    _instance->_pushKeyEvent(kc, rep->modifier.val, false);
            }
            for (int i = 0; i < 6; i++) {
                uint8_t kc = rep->key[i];
                if (kc == 0) continue;
                bool was = false;
                for (int j = 0; j < 6; j++)
                    if (_last_kb.key[j] == kc) { was = true; break; }
                if (!was)
                    _instance->_pushKeyEvent(kc, rep->modifier.val, true);
            }
            _last_kb = *rep;

        } else if ((uintptr_t)arg == HID_PROTOCOL_MOUSE && len >= 5) {
            uint8_t buttons = data[1];
            int16_t x = (int16_t)(data[2] | ((data[3] & 0x0F) << 8));
            int16_t y = (int16_t)((data[3] >> 4) | (data[4] << 4));
            if (x & 0x800) x |= 0xF000;
            if (y & 0x800) y |= 0xF000;
            int8_t scroll = (len >= 6) ? (int8_t)data[5] : 0;
            _instance->_pushMouseEvent((int8_t)x, (int8_t)y, buttons, scroll);

        } else {
            uint8_t gtype = _instance->_getGamepadType();

            if (gtype == 0) {
                // Тип не задан — hex лог для анализа raw репортов
                char hex[64] = {};
                for (size_t i = 0; i < len && i < 16; i++)
                    sprintf(hex + i*3, "%02X ", data[i]);
                PDA_LOGI("usb", "raw [%d]: %s", (int)len, hex);

            } else if (gtype == 1 && len >= 7) {
                // Геймпад без стиков
                // data[0]     — report ID (0x01)
                // data[1]     — LX (0x7F = нет стика, игнорируем)
                // data[2]     — LY (0x7F = нет стика, игнорируем)
                // data[3]     — D-pad X: 0x00=влево, 0x7F=центр, 0xFF=вправо
                // data[4]     — D-pad Y: 0x00=вверх, 0x7F=центр, 0xFF=вниз
                // data[5] hi  — X=0x10, A=0x20, B=0x40, Y=0x80
                // data[6]     — L1=0x01, R1=0x02, L2=0x04, R2=0x08, Select=0x10, Start=0x20
                pda2::GamepadState gs = {};
                gs.dpad_x = (data[3] < 0x40) ? -1 : (data[3] > 0xC0) ?  1 : 0;
                gs.dpad_y = (data[4] < 0x40) ? -1 : (data[4] > 0xC0) ?  1 : 0;
                gs.x      = data[5] & 0x10;
                gs.a      = data[5] & 0x20;
                gs.b      = data[5] & 0x40;
                gs.y      = data[5] & 0x80;
                gs.l1     = data[6] & 0x01;
                gs.r1     = data[6] & 0x02;
                gs.l2     = data[6] & 0x04;
                gs.r2     = data[6] & 0x08;
                gs.select = data[6] & 0x10;
                gs.start  = data[6] & 0x20;
                _instance->_pushGamepadEvent(gs);

            } else if (gtype == 2 && len >= 7) {
                // Геймпад со стиками (тип 2, протестировано)
                // data[0] — report ID (0x01)
                // data[1] — RX: 0x00=влево, 0x80=центр, 0xFF=вправо
                // data[2] — RY: 0x00=вверх, 0x80=центр, 0xFF=вниз
                // data[3] — LX: 0x00=влево, 0x80=центр, 0xFF=вправо
                // data[4] — LY: 0x00=вверх, 0x80=центр, 0xFF=вниз
                // data[5] lo: hat (0=вверх,2=вправо,4=вниз,6=влево,F=нейтраль)
                // data[5] hi: X=0x80, A=0x40, B=0x20, Y=0x10
                // data[6]: L1=0x01,R1=0x02,L2=0x04,R2=0x08,Select=0x10,Start=0x20,L3=0x40,R3=0x80
                pda2::GamepadState gs = {};
                gs.rx = (int8_t)(data[1] - 0x80);
                gs.ry = (int8_t)(data[2] - 0x80);
                gs.lx = (int8_t)(data[3] - 0x80);
                gs.ly = (int8_t)(data[4] - 0x80);
                uint8_t hat = data[5] & 0x0F;
                if (hat != 0x0F) {
                    gs.dpad_x = (hat == 1 || hat == 2 || hat == 3) ?  1 :
                                (hat == 5 || hat == 6 || hat == 7) ? -1 : 0;
                    gs.dpad_y = (hat == 7 || hat == 0 || hat == 1) ? -1 :
                                (hat == 3 || hat == 4 || hat == 5) ?  1 : 0;
                }
                gs.x      = data[5] & 0x80;
                gs.a      = data[5] & 0x40;
                gs.b      = data[5] & 0x20;
                gs.y      = data[5] & 0x10;
                gs.l1     = data[6] & 0x01;
                gs.r1     = data[6] & 0x02;
                gs.l2     = data[6] & 0x04;
                gs.r2     = data[6] & 0x08;
                gs.select = data[6] & 0x10;
                gs.start  = data[6] & 0x20;
                gs.l3     = data[6] & 0x40;
                gs.r3     = data[6] & 0x80;
                _instance->_pushGamepadEvent(gs);
            }
        }
        break;

    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        _instance->_setConnected(false);
        hid_host_device_close(handle);
        PDA_LOGI("usb", "device disconnected");
        break;

    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        PDA_LOGW("usb", "transfer error");
        break;

    default:
        break;
    }
}

// ════════════════════════════════════════════════════════
//  Колбэк HID драйвера — устройство подключено
// ════════════════════════════════════════════════════════
static void hid_driver_cb(hid_host_device_handle_t handle,
                           const hid_host_driver_event_t event,
                           void* arg)
{
    if (event != HID_HOST_DRIVER_EVENT_CONNECTED) return;
    if (!_instance) return;

    hid_host_dev_params_t params = {};
    if (hid_host_device_get_params(handle, &params) != ESP_OK) return;

    PDA_LOGI("usb", "HID connected proto=%d sub=%d", params.proto, params.sub_class);

    // Открываем любое HID устройство
    hid_host_device_config_t cfg = {
        .callback     = hid_interface_cb,
        .callback_arg = (void*)(uintptr_t)params.proto,
    };
    if (hid_host_device_open(handle, &cfg) != ESP_OK) {
        PDA_LOGE("usb", "hid_host_device_open failed");
        return;
    }
    // Boot protocol только для клавиатуры и мыши
    if (params.sub_class == HID_SUBCLASS_BOOT_INTERFACE &&
        (params.proto == HID_PROTOCOL_KEYBOARD || params.proto == HID_PROTOCOL_MOUSE)) {
        if (hid_class_request_set_protocol(handle, HID_REPORT_PROTOCOL_BOOT) != ESP_OK) {
            PDA_LOGW("usb", "set boot protocol failed");
        }
    }
    hid_host_device_start(handle);
    _instance->_setConnected(true);
}

// ════════════════════════════════════════════════════════
//  Usb_Class — ESP32-реализация
// ════════════════════════════════════════════════════════

void pda2::Usb_Class::begin() {
    _instance = this;

    usb_host_config_t host_cfg = {
        .skip_phy_setup = false,
        .intr_flags     = ESP_INTR_FLAG_LEVEL1,
    };
    esp_err_t err = usb_host_install(&host_cfg);
    if (err != ESP_OK) {
        PDA_LOGE("usb", "usb_host_install failed: %s", esp_err_to_name(err));
        return;
    }

    xTaskCreatePinnedToCore(usb_host_task, "usb_daemon",
                            4096, nullptr, 5, nullptr, tskNO_AFFINITY);

    hid_host_driver_config_t hid_cfg = {
        .create_background_task = true,
        .task_priority          = 5,
        .stack_size             = 4096,
        .core_id                = tskNO_AFFINITY,
        .callback               = hid_driver_cb,
        .callback_arg           = nullptr,
    };
    err = hid_host_install(&hid_cfg);
    if (err != ESP_OK) {
        PDA_LOGE("usb", "hid_host_install failed: %s", esp_err_to_name(err));
        return;
    }

    PDA_LOGI("usb", "USB Host started, waiting for devices...");
}

void pda2::Usb_Class::_platformPoll(Usb_Class* /*self*/) {
    // Пусто — на ESP32 события приходят асинхронно через
    // hid_interface_cb, выше по файлу.
}

#endif // !PDA2_SIM