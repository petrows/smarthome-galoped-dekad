/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: LicenseRef-Included
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "button.h"

static const char *TAG = "BUTTON";

static void button_poll_task(void *arg)
{
    QueueHandle_t queue = (QueueHandle_t)arg;

    gpio_config_t io_conf = {
        .pin_bit_mask  = (1ULL << BUTTON_GPIO),
        .mode          = GPIO_MODE_INPUT,
        .pull_up_en    = GPIO_PULLUP_ENABLE,
        .pull_down_en  = GPIO_PULLDOWN_DISABLE,
        .intr_type     = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_LOGI(TAG, "Button polling started on GPIO%d", BUTTON_GPIO);

    bool last_level   = true;   /* pulled high → not pressed */
    bool long_fired   = false;
    int64_t press_us  = 0;

    while (1) {
        bool level = gpio_get_level(BUTTON_GPIO);  /* active-low */

        if (!level && last_level) {
            /* Falling edge — press start */
            press_us   = esp_timer_get_time();
            long_fired = false;
            ESP_LOGD(TAG, "Press start");

        } else if (!level && !last_level && !long_fired) {
            /* Still held — check for long press threshold */
            int64_t held_ms = (esp_timer_get_time() - press_us) / 1000;
            if (held_ms >= BUTTON_LONG_PRESS_MS) {
                long_fired = true;
                ESP_LOGI(TAG, "Long press detected (%lld ms)", held_ms);
                button_event_t evt = BUTTON_EVT_LONG_PRESS;
                xQueueSend(queue, &evt, 0);
            }

        } else if (level && !last_level) {
            /* Rising edge — release */
            if (!long_fired) {
                int64_t held_ms = (esp_timer_get_time() - press_us) / 1000;
                ESP_LOGI(TAG, "Short press detected (%lld ms)", held_ms);
                button_event_t evt = BUTTON_EVT_SHORT_PRESS;
                xQueueSend(queue, &evt, 0);
            }
        }

        last_level = level;
        vTaskDelay(pdMS_TO_TICKS(20));  /* 20 ms — poll interval + debounce */
    }
}

void button_start(QueueHandle_t event_queue)
{
    xTaskCreate(button_poll_task, "button", 2048, event_queue, 5, NULL);
}
