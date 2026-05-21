/**
 * @file galoped.cpp
 * @author Petr Golovachev (petro@petro.ws)
 * @brief Galoped Zigbee firmware entry point
 */

#include "esp_check.h"
#include "esp_log.h"
#include "esp_zigbee_core.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "button.h"
#include "rgb_led.h"

#include <memory>

static const char *TAG = "GLP";

namespace
{

constexpr gpio_num_t BUTTON_GPIO = GPIO_NUM_9;
constexpr gpio_num_t LED_GPIO = GPIO_NUM_8;
constexpr uint8_t HA_ENDPOINT = 10;
constexpr uint32_t LONG_PRESS_MS = 3000;

// ZCL character strings are length-prefixed (first byte = length)
constexpr char MANUFACTURER_NAME[] =
    "\x07"
    "galoped";
constexpr char MODEL_IDENTIFIER[] =
    "\x07"
    "galoped";

std::unique_ptr<RgbLed> g_led;
std::unique_ptr<Button> g_button;

void apply_attr(const esp_zb_zcl_set_attr_value_message_t *msg)
{
    if (!msg || !g_led || msg->info.dst_endpoint != HA_ENDPOINT) {
        return;
    }

    switch (msg->info.cluster) {
    case ESP_ZB_ZCL_CLUSTER_ID_ON_OFF:
        if (msg->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID) {
            g_led->set_on(*reinterpret_cast<bool *>(msg->attribute.data.value));
        }
        break;

    case ESP_ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL:
        if (msg->attribute.id == ESP_ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID) {
            g_led->set_brightness(*reinterpret_cast<uint8_t *>(msg->attribute.data.value));
        }
        break;

    case ESP_ZB_ZCL_CLUSTER_ID_COLOR_CONTROL: {
        if (msg->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_HUE_ID) {
            uint8_t hue = *reinterpret_cast<uint8_t *>(msg->attribute.data.value);
            // Zigbee CurrentHue is 0..254 representing 0..360°
            uint16_t hue_deg = static_cast<uint16_t>((uint32_t)hue * 360u / 254u);
            g_led->set_hue_sat(hue_deg, g_led->saturation());
        } else if (msg->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_SATURATION_ID) {
            uint8_t sat = *reinterpret_cast<uint8_t *>(msg->attribute.data.value);
            uint8_t sat_8 = static_cast<uint8_t>((uint32_t)sat * 255u / 254u);
            g_led->set_hue_sat(g_led->hue(), sat_8);
        } else if (msg->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID) {
            uint16_t x16 = *reinterpret_cast<uint16_t *>(msg->attribute.data.value);
            g_led->set_xy(x16, g_led->color_y());
        } else if (msg->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID) {
            uint16_t y16 = *reinterpret_cast<uint16_t *>(msg->attribute.data.value);
            g_led->set_xy(g_led->color_x(), y16);
        }
        break;
    }

    default:
        break;
    }
}

esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t cb_id, const void *msg)
{
    if (cb_id == ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID) {
        apply_attr(static_cast<const esp_zb_zcl_set_attr_value_message_t *>(msg));
    }
    return ESP_OK;
}

void bdb_start_commissioning_void(uint8_t mode)
{
    esp_zb_bdb_start_top_level_commissioning(mode);
}

void log_network_info(const char *event)
{
    esp_zb_ieee_addr_t ext_pan_id;
    esp_zb_get_extended_pan_id(ext_pan_id);
    ESP_LOGI(TAG,
             "%s: short=0x%04x pan=0x%04x channel=%u "
             "ext_pan=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
             event, esp_zb_get_short_address(), esp_zb_get_pan_id(), esp_zb_get_current_channel(), ext_pan_id[7],
             ext_pan_id[6], ext_pan_id[5], ext_pan_id[4], ext_pan_id[3], ext_pan_id[2], ext_pan_id[1], ext_pan_id[0]);
}

void zb_signal_handler(esp_zb_app_signal_t *signal)
{
    uint32_t *p_sg = signal->p_app_signal;
    esp_err_t status = signal->esp_err_status;
    esp_zb_app_signal_type_t type = static_cast<esp_zb_app_signal_type_t>(*p_sg);

    switch (type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Starting Zigbee stack");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;

    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        if (status == ESP_OK) {
            ESP_LOGI(TAG, "First start: launching network steering");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        } else {
            ESP_LOGE(TAG, "BDB init failed: %s", esp_err_to_name(status));
            esp_zb_scheduler_alarm(bdb_start_commissioning_void, ESP_ZB_BDB_MODE_INITIALIZATION, 1000);
        }
        break;

    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (status == ESP_OK) {
            log_network_info("Rebooted on existing network");
        } else {
            ESP_LOGW(TAG, "Reboot status=%s, restarting steering", esp_err_to_name(status));
            esp_zb_scheduler_alarm(bdb_start_commissioning_void, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;

    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (status == ESP_OK) {
            log_network_info("Joined network");
        } else {
            ESP_LOGW(TAG, "Steering failed (%s); retrying in 1s", esp_err_to_name(status));
            esp_zb_scheduler_alarm(bdb_start_commissioning_void, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;

    case ESP_ZB_ZDO_SIGNAL_LEAVE:
        ESP_LOGW(TAG, "Left network");
        break;

    default:
        ESP_LOGI(TAG, "ZDO signal 0x%x status=%s", type, esp_err_to_name(status));
        break;
    }
}

void zb_task(void *)
{
    esp_zb_cfg_t zb_cfg = {};
    zb_cfg.esp_zb_role = ESP_ZB_DEVICE_TYPE_ED;
    zb_cfg.install_code_policy = false;
    zb_cfg.nwk_cfg.zed_cfg.ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_64MIN;
    zb_cfg.nwk_cfg.zed_cfg.keep_alive = 3000;
    esp_zb_init(&zb_cfg);

    esp_zb_color_dimmable_light_cfg_t light_cfg = ESP_ZB_DEFAULT_COLOR_DIMMABLE_LIGHT_CONFIG();
    light_cfg.basic_cfg.power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_MAINS_SINGLE_PHASE;
    // Advertise both XY and Hue/Saturation; coordinators (z2m, ZHA) typically drive
    // color via MoveToColor (XY) regardless of color_mode, so we must accept both.
    light_cfg.color_cfg.color_mode = 0x01;            // XY (matches z2m's default command)
    light_cfg.color_cfg.enhanced_color_mode = 0x01;   // XY
    light_cfg.color_cfg.color_capabilities = 0x0009;  // bit 0 HueSat | bit 3 XY
    esp_zb_ep_list_t *ep_list = esp_zb_color_dimmable_light_ep_create(HA_ENDPOINT, &light_cfg);

    esp_zb_cluster_list_t *clusters = esp_zb_ep_list_get_ep(ep_list, HA_ENDPOINT);
    esp_zb_attribute_list_t *basic =
        esp_zb_cluster_list_get_cluster(clusters, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_basic_cluster_add_attr(basic, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
                                  const_cast<char *>(MANUFACTURER_NAME));
    esp_zb_basic_cluster_add_attr(basic, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
                                  const_cast<char *>(MODEL_IDENTIFIER));

    // Color Control cluster created by the helper only carries CurrentX/CurrentY by
    // default; explicitly add CurrentHue and CurrentSaturation so commands targeting
    // them land in our ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID handler.
    esp_zb_attribute_list_t *color =
        esp_zb_cluster_list_get_cluster(clusters, ESP_ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    uint8_t hue_default = ESP_ZB_ZCL_COLOR_CONTROL_CURRENT_HUE_DEFAULT_VALUE;
    uint8_t sat_default = ESP_ZB_ZCL_COLOR_CONTROL_CURRENT_SATURATION_DEFAULT_VALUE;
    esp_zb_color_control_cluster_add_attr(color, ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_HUE_ID, &hue_default);
    esp_zb_color_control_cluster_add_attr(color, ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_SATURATION_ID, &sat_default);

    esp_zb_device_register(ep_list);
    esp_zb_core_action_handler_register(zb_action_handler);
    esp_zb_set_primary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK);

    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}

void factory_reset()
{
    ESP_LOGW(TAG, "Long press: factory reset");
    esp_zb_factory_reset();
}

}  // namespace

// Zigbee stack expects this symbol with C linkage
extern "C" void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    zb_signal_handler(signal_struct);
}

extern "C" void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());

    esp_zb_platform_config_t platform = {};
    platform.radio_config.radio_mode = ZB_RADIO_MODE_NATIVE;
    platform.host_config.host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE;
    ESP_ERROR_CHECK(esp_zb_platform_config(&platform));

    g_led = std::make_unique<RgbLed>(LED_GPIO);
    ESP_ERROR_CHECK(g_led->init());

    g_button = std::make_unique<Button>(BUTTON_GPIO, LONG_PRESS_MS);
    g_button->on_long_press(factory_reset);
    ESP_ERROR_CHECK(g_button->init());

    ESP_LOGI(TAG, "App startup");
    xTaskCreate(zb_task, "zb_main", 4096, nullptr, 5, nullptr);
}
