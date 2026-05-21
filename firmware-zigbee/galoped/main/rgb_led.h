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

    // hue 0..360, saturation 0..255
    void set_hue_sat(uint16_t hue, uint8_t saturation);
    uint16_t hue() const
    {
        return hue_;
    }
    uint8_t saturation() const
    {
        return saturation_;
    }

    void set_hsb(uint16_t hue, uint8_t saturation, uint8_t brightness);

   private:
    void render();
    static void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b);

    gpio_num_t pin_;
    spi_host_device_t spi_host_;
    uint32_t led_count_;

    led_strip_handle_t strip_;

    bool on_;
    uint8_t brightness_;
    uint16_t hue_;
    uint8_t saturation_;
};
