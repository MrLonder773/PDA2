#pragma once

// ════════════════════════════════════════════════════════
//  PDA 2 — Imu_Class.h
//  BMI160 (accel+gyro) + QMC5883L (магнитометр, опционален).
//  Raw I2C, без внешних библиотек.
//
//  Использование: PDA.update() вызывает update() раз за тик,
//  приложение читает get()/pitch()/roll()/heading() — это кэш,
//  шину они не трогают.
//
//  Система осей: считается совпадающей с экраном
//  (x — вправо, y — вверх по экрану, z — из экрана).
//  Ремап осей BMI160/QMC5883L — после пересборки платы.
// ════════════════════════════════════════════════════════

#include <stdint.h>

struct pda2_imu_t {
    float ax, ay, az;   // акселерометр (g)
    float gx, gy, gz;   // гироскоп (°/s)
    float mx, my, mz;   // магнитометр (µT), с вычтенным hard-iron offset;
                        // 0, пока mag_valid=false
    bool  valid;        // accel+gyro: false — чтение не удалось или ещё не было.
                        // Поля ax..gz тогда — последние удачные значения (или 0).
    bool  mag_valid;    // магнитометр: был хотя бы один свежий отсчёт и magOk()
};

class Imu_Class {
public:
    void begin();                       // ошибка — в лог, загрузку не блокирует
    void update();                      // один раз за тик: читает шину, обновляет кэш

    bool ok() const;                    // BMI160 (accel+gyro) найден и настроен
    bool magOk() const;                 // магнитометр найден и отвечает
    pda2_imu_t get() const;             // последний снимок из кэша, проверять .valid / .mag_valid

    // Производные (при совпадении осей с экраном). NAN — данных нет.
    float pitch() const;                // ° ; >0 — верхний край экрана поднят
    float roll() const;                 // ° ; >0 — правый край экрана поднят
    float heading() const;              // 0..360°, куда смотрит верх экрана
                                        // (0=север, по часовой), с компенсацией
                                        // наклона; NAN при !ok() или !magOk().
                                        // Работает и без калибровки — приложение
                                        // проверяет magCalibrated()

    // Калибровка магнитометра (hard-iron, подгон сферы МНК). Вращать устройство
    // во все стороны.
    bool  magCalibStart();              // false: магнитометра нет, калибровка не запущена
    bool  magCalibStop();               // true: offset принят и сохранён;
                                        // false: не идёт калибровка / мало данных
    void  magCalibReset();              // offset = 0, сохранённый в Prefs удаляется
    float magCalibProgress() const;     // 0..1
    bool  magCalibrating() const;       // калибровка идёт сейчас
    bool  magCalibrated() const;        // есть принятый offset (загружен из Prefs
                                        // или получен magCalibStop())

private:
    static constexpr float   MAG_LSB_PER_UT   = 30.0f;   // ±8G: 3000 LSB/Гс, 1 Гс = 100 µT
    static constexpr uint8_t MAG_FAIL_LIMIT   = 10;      // подряд сбоев шины → magOk()=false
    static constexpr int32_t MAG_CALIB_MIN_SPAN = 1200;   // мин. размах по каждой оси (коды, ~40 µT)
    static constexpr float   MAG_CALIB_RATIO    = 0.85f;  // мин. размах оси / макс. размах (у сферы равны)
    static constexpr float   MAG_FIELD_MIN_UT   = 15.0f;  // правдоподобный радиус поля Земли
    static constexpr float   MAG_FIELD_MAX_UT   = 80.0f;

    bool       _ok = false;
    bool       _mag_present = false;
    bool       _mag_fresh = false;            // был хотя бы один отсчёт магнитометра
    uint8_t    _mag_fail = 0;
    pda2_imu_t _snap = {};

    int16_t    _mag_off[3] = {0, 0, 0};       // hard-iron offset, сырые коды
    bool       _mag_calibrated = false;
    bool       _calibrating = false;
    int16_t    _cal_min[3] = {0, 0, 0};       // для magCalibProgress()
    int16_t    _cal_max[3] = {0, 0, 0};
    double     _cal_sum[14] = {};             // суммы для подгона сферы (µT)

    // Общая логика (Imu_Class.cpp) — вызывается из _esp32/_sim
    void _set_accel_gyro(float ax, float ay, float az, float gx, float gy, float gz);
    void _accel_gyro_failed();
    void _mag_bus_result(bool ok);
    void _set_mag_raw(int16_t x, int16_t y, int16_t z);
    void _load_mag_offset();
    void _save_mag_offset();
};