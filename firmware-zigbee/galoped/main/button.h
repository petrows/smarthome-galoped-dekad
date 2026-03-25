/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: LicenseRef-Included
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUTTON_GPIO           9
#define BUTTON_LONG_PRESS_MS  3000  /* Hold duration to trigger long press */

typedef enum {
    BUTTON_EVT_SHORT_PRESS,
    BUTTON_EVT_LONG_PRESS,
} button_event_t;

/**
 * @brief Start button polling task.
 *
 * Sends button_event_t values to the provided queue:
 *   - BUTTON_EVT_SHORT_PRESS on tap (< BUTTON_LONG_PRESS_MS)
 *   - BUTTON_EVT_LONG_PRESS  after holding >= BUTTON_LONG_PRESS_MS
 *
 * @param event_queue  Queue to receive button_event_t events.
 */
void button_start(QueueHandle_t event_queue);

#ifdef __cplusplus
}
#endif
