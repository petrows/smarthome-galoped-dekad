/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: LicenseRef-Included
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Espressif Systems
 *    integrated circuit in a product or a software update for such product,
 *    must reproduce the above copyright notice, this list of conditions and
 *    the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "led_strip.h"
#include "light_driver.h"

static const char *TAG = "LIGHT_DRIVER";

typedef enum {
    COLOR_MODE_HS = 0,  /* Hue / Saturation */
    COLOR_MODE_XY = 1,  /* CIE 1931 XY      */
} light_color_mode_t;

static led_strip_handle_t s_led_strip;
static bool    s_power      = false;
static uint8_t s_hue        = 0;       /* ZCL hue: 0-254 */
static uint8_t s_saturation = 0;       /* ZCL saturation: 0-254 */
static uint8_t s_level      = 254;     /* ZCL level: 0-254 */
static uint16_t s_color_x   = 0x616B;  /* CIE X default (D65 white ~0.3127) */
static uint16_t s_color_y   = 0x607D;  /* CIE Y default (D65 white ~0.3290) */
static light_color_mode_t s_color_mode = COLOR_MODE_HS;

/* Blink state */
static esp_timer_handle_t s_blink_timer = NULL;
static volatile bool      s_blink_active = false;
static volatile bool      s_blink_phase  = false;  /* true = LED on */

/* Blink colour: dim blue (R=0, G=0, B=32) */
#define BLINK_R  0
#define BLINK_G  0
#define BLINK_B  32

/* Convert HSV (hue 0-360, sat 0-255, val 0-255) to RGB (each 0-255) */
static void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v,
                       uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (s == 0) {
        *r = *g = *b = v;
        return;
    }
    uint16_t region  = h / 60;
    uint16_t rem     = (h - region * 60) * 255 / 60;
    uint8_t  p       = (uint32_t)v * (255 - s) / 255;
    uint8_t  q       = (uint32_t)v * (255 - ((uint32_t)s * rem) / 255) / 255;
    uint8_t  t_val   = (uint32_t)v * (255 - ((uint32_t)s * (255 - rem)) / 255) / 255;
    switch (region) {
    case 0:  *r = v;     *g = t_val; *b = p;     break;
    case 1:  *r = q;     *g = v;     *b = p;     break;
    case 2:  *r = p;     *g = v;     *b = t_val; break;
    case 3:  *r = p;     *g = q;     *b = v;     break;
    case 4:  *r = t_val; *g = p;     *b = v;     break;
    default: *r = v;     *g = p;     *b = q;     break;
    }
}

/* Convert CIE 1931 XY + brightness to RGB.
 * x, y  — ZCL values (0-65279), brightness — 0-255.
 * Uses sRGB D65 matrix and gamma correction.
 */
static void xy_to_rgb(uint16_t zcl_x, uint16_t zcl_y, uint8_t brightness,
                      uint8_t *r, uint8_t *g, uint8_t *b)
{
    float x = (float)zcl_x / 65536.0f;
    float y = (float)zcl_y / 65536.0f;
    float Y = (float)brightness / 255.0f;

    if (y < 1e-6f) {
        *r = *g = *b = 0;
        return;
    }

    /* XY + Y → XYZ */
    float X = (Y / y) * x;
    float Z = (Y / y) * (1.0f - x - y);

    /* XYZ → linear sRGB (D65 white point) */
    float r_lin =  3.2404542f * X - 1.5371385f * Y - 0.4985314f * Z;
    float g_lin = -0.9692660f * X + 1.8760108f * Y + 0.0415560f * Z;
    float b_lin =  0.0556434f * X - 0.2040259f * Y + 1.0572252f * Z;

    /* Clamp negatives */
    if (r_lin < 0.0f) r_lin = 0.0f;
    if (g_lin < 0.0f) g_lin = 0.0f;
    if (b_lin < 0.0f) b_lin = 0.0f;

    /* Normalise so the brightest channel is 1.0 (preserves hue) */
    float max_c = r_lin;
    if (g_lin > max_c) max_c = g_lin;
    if (b_lin > max_c) max_c = b_lin;
    if (max_c > 1.0f) {
        r_lin /= max_c;
        g_lin /= max_c;
        b_lin /= max_c;
    }

    /* sRGB gamma correction */
#define GAMMA(c) ((c) <= 0.0031308f ? 12.92f * (c) : 1.055f * powf((c), 1.0f / 2.4f) - 0.055f)
    *r = (uint8_t)(GAMMA(r_lin) * 255.0f + 0.5f);
    *g = (uint8_t)(GAMMA(g_lin) * 255.0f + 0.5f);
    *b = (uint8_t)(GAMMA(b_lin) * 255.0f + 0.5f);
#undef GAMMA
}

static void light_driver_update(void)
{
    if (s_blink_active) {
        return; /* LED is owned by the blink timer; skip normal updates */
    }
    uint8_t r = 0, g = 0, b = 0;
    if (s_power) {
        if (s_color_mode == COLOR_MODE_XY) {
            xy_to_rgb(s_color_x, s_color_y, s_level, &r, &g, &b);
            ESP_LOGD(TAG, "LED update (XY): x=%d y=%d level=%d -> r=%d g=%d b=%d",
                     s_color_x, s_color_y, s_level, r, g, b);
        } else {
            /* ZCL hue 0-254 → degrees 0-360 */
            uint16_t hue_deg = (uint16_t)s_hue * 360 / 254;
            hsv_to_rgb(hue_deg, s_saturation, s_level, &r, &g, &b);
            ESP_LOGD(TAG, "LED update (HS): hue=%d sat=%d level=%d -> r=%d g=%d b=%d",
                     s_hue, s_saturation, s_level, r, g, b);
        }
    }
    ESP_ERROR_CHECK(led_strip_set_pixel(s_led_strip, 0, r, g, b));
    ESP_ERROR_CHECK(led_strip_refresh(s_led_strip));
}

void light_driver_init(bool power)
{
    led_strip_config_t led_strip_conf = {
        .max_leds = CONFIG_EXAMPLE_STRIP_LED_NUMBER,
        .strip_gpio_num = CONFIG_EXAMPLE_STRIP_LED_GPIO,
    };
    led_strip_rmt_config_t rmt_conf = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&led_strip_conf, &rmt_conf, &s_led_strip));
    s_power = power;
    light_driver_update();
}

void light_driver_set_power(bool power)
{
    s_power = power;
    light_driver_update();
}

void light_driver_set_level(uint8_t level)
{
    s_level = level;
    light_driver_update();
}

void light_driver_set_color_hue(uint8_t hue)
{
    s_hue        = hue;
    s_color_mode = COLOR_MODE_HS;
    light_driver_update();
}

void light_driver_set_color_saturation(uint8_t saturation)
{
    s_saturation = saturation;
    s_color_mode = COLOR_MODE_HS;
    light_driver_update();
}

void light_driver_set_color_xy(uint16_t x, uint16_t y)
{
    s_color_x    = x;
    s_color_y    = y;
    s_color_mode = COLOR_MODE_XY;
    light_driver_update();
}

uint16_t light_driver_get_color_x(void)
{
    return s_color_x;
}

uint16_t light_driver_get_color_y(void)
{
    return s_color_y;
}

static void blink_timer_cb(void *arg)
{
    s_blink_phase = !s_blink_phase;
    if (s_blink_phase) {
        led_strip_set_pixel(s_led_strip, 0, BLINK_R, BLINK_G, BLINK_B);
    } else {
        led_strip_set_pixel(s_led_strip, 0, 0, 0, 0);
    }
    led_strip_refresh(s_led_strip);
}

void light_driver_blink_start(uint32_t period_ms)
{
    if (s_blink_active) {
        return;
    }
    if (s_blink_timer == NULL) {
        esp_timer_create_args_t args = {
            .callback = blink_timer_cb,
            .name     = "led_blink",
        };
        ESP_ERROR_CHECK(esp_timer_create(&args, &s_blink_timer));
    }
    s_blink_phase  = true;
    s_blink_active = true;
    led_strip_set_pixel(s_led_strip, 0, BLINK_R, BLINK_G, BLINK_B);
    led_strip_refresh(s_led_strip);
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_blink_timer, (period_ms / 2) * 1000ULL));
    ESP_LOGI(TAG, "Blink started (%lu ms period)", period_ms);
}

void light_driver_blink_stop(void)
{
    if (!s_blink_active) {
        return;
    }
    esp_timer_stop(s_blink_timer);
    s_blink_active = false;
    s_blink_phase  = false;
    light_driver_update(); /* restore actual light state */
    ESP_LOGI(TAG, "Blink stopped");
}
