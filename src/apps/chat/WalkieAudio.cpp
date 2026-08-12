/*
#include "WalkieAudio.h"
#include "pda2_config.h"
// ── Статические члены ─────────────────────────────────────
QueueHandle_t WalkieAudio::_tx_q      = nullptr;
QueueHandle_t WalkieAudio::_rx_q      = nullptr;
TaskHandle_t  WalkieAudio::_task      = nullptr;
TaskHandle_t  WalkieAudio::_nrf_task  = nullptr;
volatile bool WalkieAudio::_capturing = false;

// ── I2S init ──────────────────────────────────────────────

bool WalkieAudio::_i2s_init() {
    const i2s_config_t spk_cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = 8000,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags     = 0,
        .dma_buf_count        = 4,
        .dma_buf_len          = 256,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
        .fixed_mclk           = 0
    };
    const i2s_pin_config_t spk_pins = {
        .bck_io_num   = PDA2_PIN_I2S_SPK_BCK,
        .ws_io_num    = PDA2_PIN_I2S_SPK_WS,
        .data_out_num = PDA2_PIN_I2S_SPK_DATA,
        .data_in_num  = I2S_PIN_NO_CHANGE
    };
    if (i2s_driver_install(SPK_PORT, &spk_cfg, 0, NULL) != ESP_OK) {
        PDA_LOGE("audio", "spk install failed"); return false;
    }
    if (i2s_set_pin(SPK_PORT, &spk_pins) != ESP_OK) {
        PDA_LOGE("audio", "spk pin failed"); return false;
    }
    i2s_zero_dma_buffer(SPK_PORT);

    const i2s_config_t mic_cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate          = 8000,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len   = 64,   // 64 сэмпла × 4 байта = 256 байт, близко к 54 сэмплам
        .use_apll             = false,
        .tx_desc_auto_clear   = false,
        .fixed_mclk           = 0
    };
    const i2s_pin_config_t mic_pins = {
        .bck_io_num   = PDA2_PIN_I2S_MIC_SCK,
        .ws_io_num    = PDA2_PIN_I2S_MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = PDA2_PIN_I2S_MIC_SD
    };
    if (i2s_driver_install(MIC_PORT, &mic_cfg, 0, NULL) != ESP_OK) {
        PDA_LOGE("audio", "mic install failed"); return false;
    }
    if (i2s_set_pin(MIC_PORT, &mic_pins) != ESP_OK) {
        PDA_LOGE("audio", "mic pin failed"); return false;
    }
    i2s_zero_dma_buffer(MIC_PORT);

    PDA_LOGI("audio", "i2s ready");
    return true;
}

void WalkieAudio::_i2s_deinit() {
    i2s_driver_uninstall(SPK_PORT);
    i2s_driver_uninstall(MIC_PORT);
    PDA_LOGI("audio", "i2s uninstalled");
}

// ── Audio task — Core 0 ───────────────────────────────────
//
//  FIX 1: dec_state ресинхронизируется из state header каждого
//          RxFrame. Декодер не зависит от предыдущих сессий.
//
//  FIX 2: двухпроходное кодирование — state сохраняется ДО
//          encode каждой половины и передаётся в TxFrame.
//          WalkieNrf упакует state в NrfPacket.data-хедер.

void WalkieAudio::_task_fn() {
    static int32_t raw32[Walkie::PCM_SAMPLES];
    static int16_t pcm_buf[Walkie::PCM_SAMPLES];
    static int16_t dec_buf[Walkie::PCM_SAMPLES];
    size_t bytes_read, bytes_written;

    ADPCM::State enc_state;

    while (true) {
        uint32_t bits = 0;
        xTaskNotifyWait(0x00, ULONG_MAX, &bits, portMAX_DELAY);

        if (bits & WAL_STOP_BIT) break;

        // ── RX: ADPCM → speaker ───────────────────────────
        if (bits & WAL_PLAY_BIT) {
            Walkie::RxFrame f;
            while (xQueueReceive(_rx_q, &f, pdMS_TO_TICKS(50)) == pdTRUE) {
                if (_capturing) break;

                ADPCM::State dec_state;
                dec_state.pred  = f.pred;
                dec_state.index = (int8_t)f.step_idx;
                ADPCM::decode(f.data, dec_buf, Walkie::PCM_SAMPLES, dec_state);

                i2s_write(SPK_PORT, dec_buf,
                          Walkie::PCM_SAMPLES * sizeof(int16_t),
                          &bytes_written, pdMS_TO_TICKS(200));
            }
        }

        // ── TX: mic → ADPCM encode → tx_q ────────────────
        if (bits & WAL_RECORD_BIT) {
            static int32_t dc_offset = 0;
            const int GAIN = 12;

            enc_state = ADPCM::State();

            while (_capturing) {
                i2s_read(MIC_PORT, raw32,
                         sizeof(int32_t) * Walkie::PCM_SAMPLES,
                         &bytes_read, portMAX_DELAY);
                if (bytes_read == 0) continue;

                size_t count = bytes_read / sizeof(int32_t);
                for (size_t i = 0; i < count; i++)
                    pcm_buf[i] = (int16_t)(raw32[i] >> 16);

                for (size_t i = 0; i < count; i++) {
                    dc_offset  += ((int32_t)pcm_buf[i] - dc_offset) >> 8;
                    pcm_buf[i]  = (int16_t)(pcm_buf[i] - dc_offset);
                    int32_t amp = (int32_t)pcm_buf[i] * GAIN;
                    if (amp >  32767) amp =  32767;
                    if (amp < -32768) amp = -32768;
                    pcm_buf[i]  = (int16_t)amp;
                }

                Walkie::TxFrame tf;
                ADPCM::State s = enc_state;
                ADPCM::encode(pcm_buf, tf.data, Walkie::PCM_SAMPLES, enc_state);
                tf.pred     = s.pred;
                tf.step_idx = (uint8_t)s.index;

                if (xQueueSend(_tx_q, &tf, 0) == pdTRUE && _nrf_task)
                    xTaskNotify(_nrf_task, WAL_NRF_TX_BIT, eSetBits);
            }
        }
    }

    vTaskDelete(nullptr);
}

// ── API ───────────────────────────────────────────────────

bool WalkieAudio::begin(QueueHandle_t tx_q, QueueHandle_t rx_q) {
    _tx_q      = tx_q;
    _rx_q      = rx_q;
    _capturing = false;

    if (!_i2s_init()) return false;

    xTaskCreatePinnedToCore(_task_fn, "wlk_audio",
        Walkie::AUDIO_STACK, nullptr, 5, &_task, 0);

    PDA_LOGI("audio", "begin ok");
    return true;
}

void WalkieAudio::end() {
    _capturing = false;
    if (_task) {
        xTaskNotify(_task, WAL_STOP_BIT, eSetBits);
        vTaskDelay(pdMS_TO_TICKS(300));
        _task = nullptr;
    }
    _nrf_task = nullptr;
    _i2s_deinit();
    PDA_LOGI("audio", "end ok");
}

void WalkieAudio::startCapture() {
    _capturing = true;
    if (_task) xTaskNotify(_task, WAL_RECORD_BIT, eSetBits);
}

void WalkieAudio::stopCapture() {
    _capturing = false;
}
*/