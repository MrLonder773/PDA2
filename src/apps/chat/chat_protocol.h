/*
#pragma once
#include <stdint.h>

// ════════════════════════════════════════════════════════
//  PDA 2 — chat_protocol.h
//  nRF24 пакет: 32 байта фиксированный payload.
// ════════════════════════════════════════════════════════

namespace ChatProtocol {

static constexpr uint8_t TYPE_TEXT  = 0x01;
static constexpr uint8_t TYPE_VOICE = 0x02;
static constexpr uint8_t TYPE_END   = 0x03;

// ── Статус peer-а (TYPE_STATUS) ───────────────────────────────────────────
// Пакет: type=0x04, device_id, msg_id=0, seq=0, payload[0]=STATUS_*
static constexpr uint8_t TYPE_STATUS = 0x04;

static constexpr uint8_t STATUS_IDLE      = 0x00;  // онлайн, ничего не делает
static constexpr uint8_t STATUS_TYPING    = 0x01;  // пишет...
static constexpr uint8_t STATUS_RECORDING = 0x02;  // записывает...

// ── Подтверждение прочтения (TYPE_READ) ───────────────────────────────────
// Пакет: type=0x05, device_id, msg_id=<прочитанный>, seq=0, payload не используется
static constexpr uint8_t TYPE_READ = 0x05;

static constexpr uint8_t PKT_SIZE = 32;
static constexpr uint8_t HDR_SIZE = 5;               // type+device_id+msg_id+seq(2)
static constexpr uint8_t PLD_SIZE = PKT_SIZE - HDR_SIZE; // 27

// Voice seq==0 payload: [total_pkts:2][pred:2][step_idx:1][adpcm:22]
static constexpr uint8_t VOICE_S0_META = 5;
static constexpr uint8_t VOICE_S0_DATA = PLD_SIZE - VOICE_S0_META; // 22 байта → 44 сэмпла

// Voice seq>0 payload:  [adpcm:27]
static constexpr uint8_t VOICE_SN_DATA = PLD_SIZE; // 27 байт → 54 сэмпла

// Text seq==0 payload:  [total_bytes:2][text:25]
static constexpr uint8_t TEXT_S0_META = 2;
static constexpr uint8_t TEXT_S0_DATA = PLD_SIZE - TEXT_S0_META; // 25

// Text seq>0 payload:   [text:27]
static constexpr uint8_t TEXT_SN_DATA = PLD_SIZE; // 27

struct Header {
    uint8_t  type;
    uint8_t  device_id;
    uint8_t  msg_id;
    uint16_t seq;
};

inline void pack_header(uint8_t* pkt, const Header& h) {
    pkt[0] = h.type;
    pkt[1] = h.device_id;
    pkt[2] = h.msg_id;
    pkt[3] = (uint8_t)(h.seq & 0xFF);
    pkt[4] = (uint8_t)(h.seq >> 8);
}

inline void unpack_header(const uint8_t* pkt, Header& h) {
    h.type      = pkt[0];
    h.device_id = pkt[1];
    h.msg_id    = pkt[2];
    h.seq       = (uint16_t)pkt[3] | ((uint16_t)pkt[4] << 8);
}

// nRF24 конфиг ChatApp
static constexpr uint8_t CHAT_CHANNEL   = 76;
static constexpr uint8_t CHAT_PA_LEVEL  = 0;   // RF24_PA_MIN
static constexpr uint8_t CHAT_DATA_RATE = 2;   // RF24_250KBPS
static const     uint8_t CHAT_ADDR[5]   = { 0xC3, 0xA1, 0xA1, 0xA1, 0xA1 };

} // namespace ChatProtocol

*/