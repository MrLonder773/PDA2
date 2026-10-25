#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Rtc_Class.h
//  DS3231, raw I2C. Без внешних библиотек.
//
//  Часы хранят СТАНДАРТНОЕ (зимнее) время = UTC + tz_off.
//  Летнее время считается при чтении по правилу (OFF/EU), поэтому
//  переход не зависит от того, был ли прибор включён в момент
//  перевода. get()/set() работают с местным временем.
//
//  get() обращается к шине (~2 мс), кэша нет: вызывать не чаще
//  раза в секунду (ClockApp — из onTick() по счётчику).
// ════════════════════════════════════════════════════════

#include <stdint.h>

enum pda2_dst_t : uint8_t {
    PDA2_DST_OFF = 0,   // без перехода
    PDA2_DST_EU  = 1    // EU: последнее вс. марта 01:00 UTC → последнее вс. октября 01:00 UTC
};

struct pda2_time_t {
    uint16_t year;      // 2000–2099
    uint8_t  month;     // 1–12
    uint8_t  day;       // 1–31
    uint8_t  hour;      // 0–23
    uint8_t  minute;    // 0–59
    uint8_t  second;    // 0–59
    uint8_t  weekday;   // 1=Пн ... 7=Вс; get() и set() считают из даты
    bool     valid;     // false: сбой шины / часы не заведены (флаг остановки
                        // осциллятора DS3231) / битые данные. Поля тогда ненадёжны.
                        // При передаче в set() игнорируется.
    bool     dst;       // get(): в возвращённом времени действует летнее время.
                        // set(): выбор для осени, когда час повторяется дважды
                        // (true — первый проход, летнее); в остальных случаях игнорируется.
};

class Rtc_Class {
public:
    void        begin();                          // ошибка — в лог, загрузку не блокирует
    bool        ok() const;                       // чип отвечает на шине
    pda2_time_t get() const;                      // МЕСТНОЕ время; читает шину (~2 мс),
                                                  // не чаще 1 раза в сек; проверять .valid
    bool        set(const pda2_time_t& t);        // местное время. false: нет чипа / дата вне
                                                  // диапазона / время из пропущенного весной часа /
                                                  // сбой шины

    // Часовой пояс: стандартное смещение от UTC в минутах (Киев = 120) и правило перехода.
    // Хранится в Prefs (по умолчанию 120 + EU). Абсолютное время при смене сохраняется.
    bool        setTimezone(int16_t offsetMin, pda2_dst_t rule);  // false: смещение вне -720..840 / сбой шины
    int16_t     tzOffset() const;
    pda2_dst_t  dstRule() const;

private:
    bool        _ok = false;
    int16_t     _tz_off = 120;
    pda2_dst_t  _dst_rule = PDA2_DST_EU;

    // Железо (Rtc_Class_esp32.cpp / Rtc_Class_sim.cpp): сырое СТАНДАРТНОЕ время.
    bool _hw_begin();                                           // true: чип отвечает
    bool _hw_read(pda2_time_t& raw, bool& osc_stopped) const;   // false: сбой шины
    bool _hw_write(const pda2_time_t& raw);                     // пишет время, сбрасывает OSF

    // Календарь без libc-часовых поясов (Rtc_Class.cpp). Секунды от 1970-01-01.
    static int64_t _to_epoch(const pda2_time_t& t);
    static void    _from_epoch(int64_t e, pda2_time_t& t);      // заполняет year..second, weekday

    // День недели по дате: 1=Пн ... 7=Вс (алгоритм Сакамото).
    // month должен быть 1..12 (проверять _is_valid() до вызова).
    static uint8_t _weekday_of(uint16_t y, uint8_t m, uint8_t d) {
        static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        if (m < 3) y -= 1;
        uint8_t w = (uint8_t)((y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7); // 0=Вс
        return w == 0 ? 7 : w;
    }

    static bool _is_valid(const pda2_time_t& t) {
        if (t.year < 2000 || t.year > 2099) return false;
        if (t.month < 1 || t.month > 12)    return false;
        if (t.hour > 23 || t.minute > 59 || t.second > 59) return false;
        static const uint8_t dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        uint8_t max_d = dim[t.month - 1];
        bool leap = (t.year % 4 == 0);   // 2000–2099: правило 100/400 не влияет
        if (t.month == 2 && leap) max_d = 29;
        return t.day >= 1 && t.day <= max_d;
    }
};