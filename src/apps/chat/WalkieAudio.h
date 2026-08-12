/*
#pragma once
#include <PDA2.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "walkie_types.h"
#include "adpcm.h"

// ════════════════════════════════════════════════════════
//  WalkieAudio — I2S + audio task
//  Core 0. Получает tx_q / rx_q от WalkieApp.
//  mic → ADPCM encode → tx_q → (WalkieNrf отправляет)
//  rx_q → ADPCM decode → speaker
// ════════════════════════════════════════════════════════

class WalkieAudio {
public:
    // begin() — I2S init + запуск таска. false если I2S не стартовал.
    bool begin(QueueHandle_t tx_q, QueueHandle_t rx_q);
    void end();

    // Вызвать после begin() WalkieNrf, чтобы audio task мог нотифицировать nrf task
    void setNrfTask(TaskHandle_t h) { _nrf_task = h; }

    TaskHandle_t taskHandle() const { return _task; }

    // Вызываются из onTick (Core 1) при смене PTT
    void startCapture();  // PTT нажата → mic → encode → tx_q
    void stopCapture();   // PTT отпущена

private:
    static constexpr i2s_port_t SPK_PORT = I2S_NUM_0;
    static constexpr i2s_port_t MIC_PORT = I2S_NUM_1;

    static QueueHandle_t _tx_q;
    static QueueHandle_t _rx_q;
    static TaskHandle_t  _task;
    static TaskHandle_t  _nrf_task;
    static volatile bool _capturing;

    static bool _i2s_init();
    static void _i2s_deinit();
    static void _task_fn(void* arg);
};
*/