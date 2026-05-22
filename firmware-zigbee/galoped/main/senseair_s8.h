#pragma once

#include "driver/uart.h"
#include "esp_err.h"
#include "hal/gpio_types.h"

#include <cstdint>
#include <cstddef>

// SenseAir S8 CO2 sensor (UART, Modbus RTU @ 9600 8N1).
// Reads input register 0x03 to retrieve the CO2 concentration in ppm.
class SenseAirS8
{
   public:
    struct Reading {
        bool valid;
        uint16_t co2_ppm;
    };

    SenseAirS8(uart_port_t port, gpio_num_t tx, gpio_num_t rx);
    ~SenseAirS8();

    SenseAirS8(const SenseAirS8 &) = delete;
    SenseAirS8 &operator=(const SenseAirS8 &) = delete;

    esp_err_t init();
    Reading read();

   private:
    static uint16_t crc16_modbus(const uint8_t *data, size_t len);

    uart_port_t port_;
    gpio_num_t tx_;
    gpio_num_t rx_;
    bool installed_;
};
