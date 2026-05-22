#pragma once

#include "driver/spi_common.h"
#include "esp_err.h"
#include "hal/gpio_types.h"
#include "led_strip.h"

#include <cstdint>

class RgbLed
{
   public:
    RgbLed(gpio_num_t data_gpio, spi_host_device_t spi_host = SPI2_HOST, uint32_t led_count = 1);
    ~RgbLed();

    RgbLed(const RgbLed &) = delete;
    RgbLed &operator=(const RgbLed &) = delete;

    esp_err_t init();

    void set_on(bool on);
    bool is_on() const
    {
        return on_;
    }

    // brightness 0..255
    void set_brightness(uint8_t brightness);
    uint8_t brightness() const
    {
        return brightness_;
    }

    // CIE 1931 xy chromaticity, each 0..65535 mapping 0..1
    void set_xy(uint16_t color_x, uint16_t color_y);
    uint16_t color_x() const
    {
        return color_x_;
    }
    uint16_t color_y() const
    {
        return color_y_;
    }

   private:
    void render();
    static void xy_to_rgb(uint16_t x16, uint16_t y16, uint8_t bri, uint8_t &r, uint8_t &g, uint8_t &b);

    gpio_num_t pin_;
    spi_host_device_t spi_host_;
    uint32_t led_count_;

    led_strip_handle_t strip_;

    bool on_;
    uint8_t brightness_;
    uint16_t color_x_;
    uint16_t color_y_;
};
