// ════════════════════════════════════════════════════════
//  PDA 2 — Rtc_Class.cpp
//  Общая часть для ESP32 и sim: часовой пояс, переход на летнее
//  время (EU), перевод стандартное ↔ местное.
//  Железо — в Rtc_Class_esp32.cpp, фейк — в Rtc_Class_sim.cpp.
//
//  Чип/«чип» хранит стандартное время = UTC + tz_off.
//  Летнее время: [последнее вс. марта 01:00 UTC; последнее вс. октября 01:00 UTC).
// ════════════════════════════════════════════════════════

#include "Rtc_Class.h"
#include "../pda2_log.h"
#include "../PDA2.h"     // PDA.Prefs (только в .cpp)

// ── Календарь (алгоритмы Говарда Хиннанта, без libc) ────

static int64_t _days_from_civil(int y, int m, int d) {
    y -= (m <= 2) ? 1 : 0;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const int yoe = y - era * 400;                         // 0..399
    const int mp  = (m > 2) ? m - 3 : m + 9;               // март = 0
    const int doy = (153 * mp + 2) / 5 + d - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (int64_t)era * 146097 + doe - 719468;
}

static void _civil_from_days(int64_t z, int& y, int& m, int& d) {
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const int doe = (int)(z - era * 146097);
    const int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int mp  = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = (mp < 10) ? mp + 3 : mp - 9;
    y = (int)(yoe + era * 400) + (m <= 2 ? 1 : 0);
}

static int64_t _floor_div(int64_t a, int64_t b) {
    int64_t q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) q--;
    return q;
}

int64_t Rtc_Class::_to_epoch(const pda2_time_t& t) {
    return _days_from_civil(t.year, t.month, t.day) * 86400
         + (int64_t)t.hour * 3600 + (int64_t)t.minute * 60 + t.second;
}

void Rtc_Class::_from_epoch(int64_t e, pda2_time_t& t) {
    int64_t days = _floor_div(e, 86400);
    int sec_of_day = (int)(e - days * 86400);
    int y, m, d;
    _civil_from_days(days, y, m, d);
    t.year   = (uint16_t)y;
    t.month  = (uint8_t)m;
    t.day    = (uint8_t)d;
    t.hour   = (uint8_t)(sec_of_day / 3600);
    t.minute = (uint8_t)((sec_of_day % 3600) / 60);
    t.second = (uint8_t)(sec_of_day % 60);
    // 1970-01-01 — четверг. 1=Пн ... 7=Вс
    t.weekday = (uint8_t)((((days + 3) % 7) + 7) % 7 + 1);
}

// ── Переход на летнее время ─────────────────────────────

// Последнее воскресенье месяца (март/октябрь — по 31 дню), 01:00 UTC
static int64_t _last_sunday_utc(int year, int month) {
    int64_t d = _days_from_civil(year, month, 31);
    int wd = (int)((((d + 3) % 7) + 7) % 7);               // 0=Пн ... 6=Вс
    d -= (wd + 1) % 7;                                     // назад до воскресенья
    return d * 86400 + 3600;
}

static bool _dst_active(int64_t utc, uint8_t rule) {
    if (rule != PDA2_DST_EU) return false;
    int y, m, d;
    _civil_from_days(_floor_div(utc, 86400), y, m, d);
    return utc >= _last_sunday_utc(y, 3) && utc < _last_sunday_utc(y, 10);
}

// ── API ─────────────────────────────────────────────────

bool       Rtc_Class::ok() const        { return _ok; }
int16_t    Rtc_Class::tzOffset() const  { return _tz_off; }
pda2_dst_t Rtc_Class::dstRule() const   { return _dst_rule; }

void Rtc_Class::begin() {
    int16_t off = PDA.Prefs.getTzOffset();
    uint8_t rule = PDA.Prefs.getDstRule();
    _tz_off   = (off >= -720 && off <= 840) ? off : 120;
    _dst_rule = (rule <= PDA2_DST_EU) ? (pda2_dst_t)rule : PDA2_DST_EU;

    _ok = _hw_begin();
    PDA_LOGI("rtc", "tz: UTC%+d min, dst rule=%s", _tz_off,
             _dst_rule == PDA2_DST_EU ? "EU" : "OFF");
}

pda2_time_t Rtc_Class::get() const {
    pda2_time_t t = {};             // valid = false по умолчанию
    if (!_ok) return t;

    pda2_time_t raw = {};
    bool osc_stopped = false;
    if (!_hw_read(raw, osc_stopped)) return t;
    if (!_is_valid(raw)) return t;                        // битые данные

    int64_t std_e = _to_epoch(raw);
    bool dst = _dst_active(std_e - (int64_t)_tz_off * 60, _dst_rule);
    _from_epoch(std_e + (dst ? 3600 : 0), t);
    t.dst   = dst;
    t.valid = !osc_stopped;
    return t;
}

bool Rtc_Class::set(const pda2_time_t& t) {
    if (!_ok) return false;
    if (!_is_valid(t)) {
        PDA_LOGE("rtc", "set(): date out of range");
        return false;
    }

    // Два кандидата стандартного времени: std = local (зима) и std = local − 1 ч (лето).
    int64_t local = _to_epoch(t);
    int64_t off   = (int64_t)_tz_off * 60;
    bool std_ok = !_dst_active(local - off, _dst_rule);
    bool dst_ok =  _dst_active(local - 3600 - off, _dst_rule);

    bool use_dst;
    if (std_ok && dst_ok) use_dst = t.dst;       // осень: час повторяется — решает флаг
    else if (dst_ok)      use_dst = true;
    else if (std_ok)      use_dst = false;
    else {                                       // весна: этого часа не существует
        PDA_LOGE("rtc", "set(): time skipped by DST change");
        return false;
    }

    pda2_time_t raw = {};
    _from_epoch(local - (use_dst ? 3600 : 0), raw);
    if (raw.year < 2000 || raw.year > 2099) {
        PDA_LOGE("rtc", "set(): date out of range");
        return false;
    }
    if (!_hw_write(raw)) return false;

    PDA_LOGI("rtc", "Time set: %04d-%02d-%02d %02d:%02d:%02d%s",
             t.year, t.month, t.day, t.hour, t.minute, t.second, use_dst ? " (DST)" : "");
    return true;
}

bool Rtc_Class::setTimezone(int16_t offsetMin, pda2_dst_t rule) {
    if (offsetMin < -720 || offsetMin > 840 || (uint8_t)rule > PDA2_DST_EU) return false;

    // Чип хранит UTC + tz_off: при смене пояса переписать, чтобы абсолютное время не сдвинулось.
    if (_ok) {
        pda2_time_t raw = {};
        bool osc_stopped = false;
        if (_hw_read(raw, osc_stopped) && !osc_stopped && _is_valid(raw)) {
            int64_t utc = _to_epoch(raw) - (int64_t)_tz_off * 60;
            pda2_time_t nr = {};
            _from_epoch(utc + (int64_t)offsetMin * 60, nr);
            if (nr.year < 2000 || nr.year > 2099 || !_hw_write(nr)) {
                PDA_LOGE("rtc", "setTimezone(): failed to rewrite clock");
                return false;
            }
        }
    }

    _tz_off   = offsetMin;
    _dst_rule = rule;
    PDA.Prefs.setTzOffset(offsetMin);
    PDA.Prefs.setDstRule((uint8_t)rule);
    PDA_LOGI("rtc", "tz set: UTC%+d min, dst rule=%s", _tz_off,
             _dst_rule == PDA2_DST_EU ? "EU" : "OFF");
    return true;
}