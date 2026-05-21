#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "hal/gpio_types.h"

#include <cstdint>

// Driver for the combined BMP280 (pressure+temp) + AHT20 (humidity+temp) I2C
// breakout. BMP280 @ 0x76, AHT20 @ 0x38. AHT20 temperature wins (cleaner
// ambient reading); BMP280 temperature is consumed only to compensate pressure.
class ClimateSensor
{
   public:
    struct Reading {
        bool temp_valid;
        bool humidity_valid;
        bool pressure_valid;
        float temperature_c;
        float humidity_pct;
        float pressure_hpa;
    };

    ClimateSensor(gpio_num_t sda, gpio_num_t scl, i2c_port_num_t port = I2C_NUM_0);
    ~ClimateSensor();

    ClimateSensor(const ClimateSensor &) = delete;
    ClimateSensor &operator=(const ClimateSensor &) = delete;

    esp_err_t init();
    Reading read();

   private:
    static constexpr uint8_t AHT20_ADDR = 0x38;
    // BMP280 / BME280 ship at either 0x76 or 0x77 depending on SDO pull
    static constexpr uint8_t BMP280_ADDR_PRIMARY = 0x76;
    static constexpr uint8_t BMP280_ADDR_ALT = 0x77;

    void bus_scan();
    esp_err_t bmp280_probe(uint8_t addr);
    esp_err_t bmp280_init();
    esp_err_t bmp280_read_raw(int32_t &t_raw, int32_t &p_raw);
    float bmp280_compensate_temp(int32_t adc_T, int32_t &t_fine);
    float bmp280_compensate_pressure(int32_t adc_P, int32_t t_fine);

    esp_err_t aht20_init();
    esp_err_t aht20_read(float &humidity, float &temperature);

    gpio_num_t sda_;
    gpio_num_t scl_;
    i2c_port_num_t port_;
    i2c_master_bus_handle_t bus_;
    i2c_master_dev_handle_t bmp_;
    i2c_master_dev_handle_t aht_;
    bool bmp_present_;
    bool aht_present_;
    uint8_t bmp_addr_;
    uint8_t bmp_chip_id_;

    struct {
        uint16_t T1;
        int16_t T2, T3;
        uint16_t P1;
        int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    } bmp_cal_;
};
