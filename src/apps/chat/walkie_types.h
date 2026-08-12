/*
#pragma once
#include <stdint.h>

// ── Нотификации audio task ────────────────────────────────
#define WAL_PLAY_BIT    0x01u
#define WAL_RECORD_BIT  0x02u
#define WAL_STOP_BIT    0x04u

// ── Нотификации nrf task ──────────────────────────────────
#define WAL_NRF_TX_BIT  0x01u
#define WAL_NRF_STOP    0x04u

namespace Walkie {
    constexpr uint16_t PCM_SAMPLES = 54;    // было 108
    constexpr uint16_t ADPCM_BYTES = 27;    // было 54
    constexpr uint32_t AUDIO_STACK = 4096;
    constexpr uint32_t NRF_STACK   = 3072;
    constexpr uint32_t RX_FLASH_MS = 800;
    constexpr uint32_t LOG_MS      = 1000;
    constexpr uint32_t BG          = 0x1a1a2e;

    struct NrfPacket {
        uint8_t  step_idx;
        int16_t  pred;
        uint8_t  data[27];
    } __attribute__((packed)); // 1+2+27 = 30 байт

    struct TxFrame {
        int16_t pred;
        uint8_t step_idx;
        uint8_t data[27];
    };
    struct RxFrame {
        int16_t pred;
        uint8_t step_idx;
        uint8_t data[27];
    };
}
*/