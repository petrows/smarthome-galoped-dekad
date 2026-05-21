#include "rgb_led.h"

#include "esp_log.h"
#include "led_strip_spi.h"

static const char *TAG = "RgbLed";

RgbLed::RgbLed(gpio_num_t data_gpio, spi_host_device_t spi_host, uint32_t led_count)
    : pin_(data_gpio),
      spi_host_(spi_host),
      led_count_(led_count),
      strip_(nullptr),
      on_(false),
      brightness_(255),
      hue_(0),
      saturation_(0)
{
}

RgbLed::~RgbLed()
{
    if (strip_) {
        led_strip_del(strip_);
    }
}

esp_err_t RgbLed::init()
{
    led_strip_config_t strip_cfg = {};
    strip_cfg.strip_gpio_num = pin_;
    strip_cfg.max_leds = led_count_;
    strip_cfg.led_model = LED_MODEL_WS2812;
    strip_cfg.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB;
    strip_cfg.flags.invert_out = false;

    led_strip_spi_config_t spi_cfg = {};
    spi_cfg.clk_src = SPI_CLK_SRC_DEFAULT;
    spi_cfg.spi_bus = spi_host_;
    spi_cfg.flags.with_dma = true;

    esp_err_t err = led_strip_new_spi_device(&strip_cfg, &spi_cfg, &strip_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "led_strip_new_spi_device: %s", esp_err_to_name(err));
        return err;
    }

    led_strip_clear(strip_);
    ESP_LOGI(TAG, "init gpio=%d host=%d leds=%lu", pin_, spi_host_, (unsigned long)led_count_);
    return ESP_OK;
}

void RgbLed::set_on(bool on)
{
    on_ = on;
    render();
}

void RgbLed::set_brightness(uint8_t brightness)
{
    brightness_ = brightness;
    render();
}

void RgbLed::set_hue_sat(uint16_t hue, uint8_t saturation)
{
    hue_ = hue;
    saturation_ = saturation;
    render();
}

void RgbLed::set_hsb(uint16_t hue, uint8_t saturation, uint8_t brightness)
{
    hue_ = hue;
    saturation_ = saturation;
    brightness_ = brightness;
    render();
}

void RgbLed::render()
{
    if (!strip_) {
        return;
    }
    if (!on_ || brightness_ == 0) {
        led_strip_clear(strip_);
        return;
    }

    uint8_t r, g, b;
    hsv_to_rgb(hue_, saturation_, brightness_, r, g, b);

    for (uint32_t i = 0; i < led_count_; ++i) {
        led_strip_set_pixel(strip_, i, r, g, b);
    }
    led_strip_refresh(strip_);
}

void RgbLed::hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b)
{
    h %= 360;
    uint32_t rgb_max = v;
    uint32_t rgb_min = rgb_max * (255 - s) / 255;
    uint32_t i = h / 60;
    uint32_t diff = h % 60;
    uint32_t adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        r = rgb_max;
        g = rgb_min + adj;
        b = rgb_min;
        break;
    case 1:
        r = rgb_max - adj;
        g = rgb_max;
        b = rgb_min;
        break;
    case 2:
        r = rgb_min;
        g = rgb_max;
        b = rgb_min + adj;
        break;
    case 3:
        r = rgb_min;
        g = rgb_max - adj;
        b = rgb_max;
        break;
    case 4:
        r = rgb_min + adj;
        g = rgb_min;
        b = rgb_max;
        break;
    default:
        r = rgb_max;
        g = rgb_min;
        b = rgb_max - adj;
        break;
    }
}
