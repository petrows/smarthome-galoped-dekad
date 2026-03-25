/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier:  LicenseRef-Included
 *
 * Zigbee HA_color_dimmable_light Example
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "zcl_utility.h"
#include "esp_zb_light.h"
#include "button.h"

#if !defined ZB_ED_ROLE
#error Define ZB_ED_ROLE in idf.py menuconfig to compile light (End Device) source code.
#endif

static const char *TAG = "ESP_ZB_COLOR_LIGHT";

static bool s_light_power = false; /* local on/off state, kept in sync with ZCL */

/********************* Define functions **************************/
static esp_err_t deferred_driver_init(void)
{
    light_driver_init(LIGHT_DEFAULT_OFF);
    return ESP_OK;
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, , TAG, "Failed to start Zigbee commissioning");
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p       = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Initialize Zigbee stack");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
            if (esp_zb_bdb_is_factory_new()) {
                ESP_LOGI(TAG, "Start network steering");
                light_driver_blink_start(500);
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            } else {
                ESP_LOGI(TAG, "Device rebooted");
            }
        } else {
            ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
            // esp_restart();
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            light_driver_blink_stop();
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
        } else {
            ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message)
{
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG,
                        "Received message: error status(%d)", message->info.status);
    ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d)",
             message->info.dst_endpoint, message->info.cluster,
             message->attribute.id, message->attribute.data.size);

    if (message->info.dst_endpoint != HA_ESP_LIGHT_ENDPOINT) {
        return ret;
    }

    switch (message->info.cluster) {
    case ESP_ZB_ZCL_CLUSTER_ID_ON_OFF:
        if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID &&
            message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL) {
            bool power = message->attribute.data.value ? *(bool *)message->attribute.data.value : false;
            ESP_LOGI(TAG, "Light power: %s", power ? "On" : "Off");
            s_light_power = power;
            light_driver_set_power(power);
        }
        break;

    case ESP_ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL:
        if (message->attribute.id == ESP_ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID &&
            message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U8) {
            uint8_t level = message->attribute.data.value ? *(uint8_t *)message->attribute.data.value : 0;
            ESP_LOGI(TAG, "Light level: %d", level);
            light_driver_set_level(level);
        }
        break;

    case ESP_ZB_ZCL_CLUSTER_ID_COLOR_CONTROL:
        if (message->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_HUE_ID &&
            message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U8) {
            uint8_t hue = message->attribute.data.value ? *(uint8_t *)message->attribute.data.value : 0;
            ESP_LOGI(TAG, "Light hue: %d", hue);
            light_driver_set_color_hue(hue);
        } else if (message->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_SATURATION_ID &&
                   message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U8) {
            uint8_t saturation = message->attribute.data.value ? *(uint8_t *)message->attribute.data.value : 0;
            ESP_LOGI(TAG, "Light saturation: %d", saturation);
            light_driver_set_color_saturation(saturation);
        } else if (message->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID &&
                   message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U16) {
            uint16_t color_x = message->attribute.data.value ? *(uint16_t *)message->attribute.data.value : 0;
            ESP_LOGI(TAG, "Light color X: %d", color_x);
            light_driver_set_color_xy(color_x, light_driver_get_color_y());
        } else if (message->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID &&
                   message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U16) {
            uint16_t color_y = message->attribute.data.value ? *(uint16_t *)message->attribute.data.value : 0;
            ESP_LOGI(TAG, "Light color Y: %d", color_y);
            light_driver_set_color_xy(light_driver_get_color_x(), color_y);
        }
        break;

    default:
        ESP_LOGD(TAG, "Unhandled cluster 0x%x", message->info.cluster);
        break;
    }
    return ret;
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;
    switch (callback_id) {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    default:
        ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    return ret;
}

static void button_task(void *pvParameters)
{
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    button_event_t evt;

    while (xQueueReceive(queue, &evt, portMAX_DELAY)) {
        switch (evt) {
        case BUTTON_EVT_SHORT_PRESS:
            s_light_power = !s_light_power;
            ESP_LOGI(TAG, "Button short press: light %s", s_light_power ? "On" : "Off");
            light_driver_set_power(s_light_power);
            if (s_light_power) {
                light_driver_set_level(0xF0);
            }
            /* Update ZCL on/off attribute so coordinator sees the new state */
            if (esp_zb_lock_acquire(portMAX_DELAY)) {
                esp_zb_zcl_set_attribute_val(HA_ESP_LIGHT_ENDPOINT,
                    ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                    ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, &s_light_power, false);
                esp_zb_lock_release();
            }
            break;

        case BUTTON_EVT_LONG_PRESS:
            ESP_LOGI(TAG, "Button long press: factory reset");
            /* esp_zb_factory_reset() erases zb_storage and calls esp_restart() */
            esp_zb_factory_reset();
            break;
        }
    }
}

static void esp_zb_task(void *pvParameters)
{
    /* initialize Zigbee stack */
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZED_CONFIG();
    esp_zb_init(&zb_nwk_cfg);

    /* Color dimmable light: OnOff + Level + Color (Hue/Saturation) clusters */
    esp_zb_color_dimmable_light_cfg_t light_cfg = ESP_ZB_DEFAULT_COLOR_DIMMABLE_LIGHT_CONFIG();
    esp_zb_ep_list_t *esp_zb_light_ep = esp_zb_color_dimmable_light_ep_create(HA_ESP_LIGHT_ENDPOINT, &light_cfg);

    zcl_basic_manufacturer_info_t info = {
        .manufacturer_name = ESP_MANUFACTURER_NAME,
        .model_identifier = ESP_MODEL_IDENTIFIER,
    };
    esp_zcl_utility_add_ep_basic_manufacturer_info(esp_zb_light_ep, HA_ESP_LIGHT_ENDPOINT, &info);

    esp_zb_device_register(esp_zb_light_ep);
    esp_zb_core_action_handler_register(zb_action_handler);
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}

void app_main(void)
{
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));

    ESP_LOGI(TAG, "Deferred driver initialization %s", deferred_driver_init() ? "failed" : "successful");

    QueueHandle_t button_queue = xQueueCreate(5, sizeof(button_event_t));
    button_start(button_queue);
    xTaskCreate(button_task, "button_handler", 3072, button_queue, 5, NULL);

    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}
