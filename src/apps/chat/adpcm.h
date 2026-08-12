/*
#pragma once
#include <stdint.h>

// ════════════════════════════════════════════════════════
//  IMA ADPCM — header-only, encode + decode
//  4 бит на сэмпл → сжатие 4:1 относительно int16
//  120 сэмплов int16 → 60 байт ADPCM
// ════════════════════════════════════════════════════════

namespace ADPCM {

// ── Таблицы ──────────────────────────────────────────────
static const int8_t _step_adj[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};
static const int16_t _step_tab[89] = {
       7,    8,    9,   10,   11,   12,   13,   14,
      16,   17,   19,   21,   23,   25,   28,   31,
      34,   37,   41,   45,   50,   55,   60,   66,
      73,   80,   88,   97,  107,  118,  130,  143,
     157,  173,  190,  209,  230,  253,  279,  307,
     337,  371,  408,  449,  494,  544,  598,  658,
     724,  796,  876,  963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493,10442,11487,12635,13899,
    15289,16818,18500,20350,22385,24623,27086,29794,
       32767
};

// ── Состояние кодера/декодера ─────────────────────────────
struct State {
    int16_t pred  = 0;   // предсказанный сэмпл
    int8_t  index = 0;   // индекс в _step_tab
};

// ── Encode одного сэмпла → 4 бита ─────────────────────────
static inline uint8_t _encode_sample(int16_t sample, State& s) {
    int32_t step  = _step_tab[s.index];
    int32_t diff  = (int32_t)sample - s.pred;
    uint8_t code  = 0;

    if (diff < 0) { code = 8; diff = -diff; }

    int32_t tmp = step;
    if (diff >= tmp)      { code |= 4; diff -= tmp; }
    tmp >>= 1;
    if (diff >= tmp)      { code |= 2; diff -= tmp; }
    tmp >>= 1;
    if (diff >= tmp)      { code |= 1; }

    // обновляем предсказание
    int32_t d = step >> 3;
    if (code & 1) d += step >> 2;
    if (code & 2) d += step >> 1;
    if (code & 4) d += step;
    if (code & 8) d  = -d;

    int32_t p = (int32_t)s.pred + d;
    if (p >  32767) p =  32767;
    if (p < -32768) p = -32768;
    s.pred = (int16_t)p;

    s.index += _step_adj[code & 7];
    if (s.index < 0)  s.index = 0;
    if (s.index > 88) s.index = 88;

    return code & 0x0F;
}

// ── Decode одного 4-битного кода → int16 ──────────────────
static inline int16_t _decode_sample(uint8_t code, State& s) {
    int32_t step = _step_tab[s.index];
    int32_t d    = step >> 3;
    if (code & 1) d += step >> 2;
    if (code & 2) d += step >> 1;
    if (code & 4) d += step;
    if (code & 8) d  = -d;

    int32_t p = (int32_t)s.pred + d;
    if (p >  32767) p =  32767;
    if (p < -32768) p = -32768;
    s.pred = (int16_t)p;

    s.index += _step_adj[code & 7];
    if (s.index < 0)  s.index = 0;
    if (s.index > 88) s.index = 88;

    return s.pred;
}

// ── Encode: N сэмплов int16 → N/2 байт ADPCM ─────────────
// samples_count должен быть чётным
static inline void encode(const int16_t* in, uint8_t* out,
                           uint16_t samples_count, State& s) {
    for (uint16_t i = 0; i < samples_count; i += 2) {
        uint8_t hi = _encode_sample(in[i],     s);
        uint8_t lo = _encode_sample(in[i + 1], s);
        out[i >> 1] = (hi << 4) | lo;
    }
}

// ── Decode: N байт ADPCM → N*2 сэмплов int16 ─────────────
static inline void decode(const uint8_t* in, int16_t* out,
                           uint16_t bytes_count, State& s) {
    for (uint16_t i = 0; i < bytes_count; i++) {
        out[i * 2]     = _decode_sample(in[i] >> 4,   s);
        out[i * 2 + 1] = _decode_sample(in[i] & 0x0F, s);
    }
}

} // namespace ADPCM

*/