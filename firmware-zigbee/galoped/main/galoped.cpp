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
#include "climate_sensor.h"
#include "rgb_led.h"
#include "senseair_s8.h"
#include "esp32_vid6608_rmt.h"

#include <cmath>
#include <memory>

static const char *TAG = "GLP";

namespace
{

constexpr gpio_num_t BUTTON_GPIO = static_cast<gpio_num_t>(CONFIG_GALOPED_BUTTON_GPIO);
constexpr gpio_num_t LED_GPIO = static_cast<gpio_num_t>(CONFIG_GALOPED_LED_GPIO);
constexpr uint32_t LED_COUNT = CONFIG_GALOPED_LED_COUNT;
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
std::unique_ptr<esp32_vid6608_rmt> g_drive_1;
std::unique_ptr<esp32_vid6608_rmt> g_drive_2;
#if GALOPED_CLIMATE
std::unique_ptr<ClimateSensor> g_climate;
#endif
#if GALOPED_CO2
std::unique_ptr<SenseAirS8> g_co2;
#endif

#if GALOPED_CLIMATE
// --- Climate sensor Zigbee endpoint ----------------------------------------
constexpr gpio_num_t I2C_SDA_GPIO = static_cast<gpio_num_t>(CONFIG_GALOPED_I2C_SDA_GPIO);
constexpr gpio_num_t I2C_SCL_GPIO = static_cast<gpio_num_t>(CONFIG_GALOPED_I2C_SCL_GPIO);
constexpr uint8_t CLIMATE_ENDPOINT = 30;
constexpr uint32_t CLIMATE_REPORT_PERIOD_MS = 30000;

void add_climate_endpoint(esp_zb_ep_list_t *ep_list)
{
    esp_zb_basic_cluster_cfg_t basic_cfg = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_MAINS_SINGLE_PHASE,
    };
    esp_zb_attribute_list_t *basic = esp_zb_basic_cluster_create(&basic_cfg);
    esp_zb_basic_cluster_add_attr(basic, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
                                  const_cast<char *>(MANUFACTURER_NAME));
    esp_zb_basic_cluster_add_attr(basic, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
                                  const_cast<char *>(MODEL_IDENTIFIER));

    esp_zb_identify_cluster_cfg_t identify_cfg = {.identify_time = 0};
    esp_zb_attribute_list_t *identify = esp_zb_identify_cluster_create(&identify_cfg);

    // 0x8000 = "invalid/unknown" sentinel per ZCL spec
    esp_zb_temperature_meas_cluster_cfg_t temp_cfg = {
        .measured_value = static_cast<int16_t>(0x8000),
        .min_value = -4000,  // -40 °C in 0.01 °C
        .max_value = 8500,   //  85 °C
    };
    esp_zb_attribute_list_t *temp = esp_zb_temperature_meas_cluster_create(&temp_cfg);

    esp_zb_humidity_meas_cluster_cfg_t hum_cfg = {
        .measured_value = 0xFFFF,
        .min_value = 0,
        .max_value = 10000,  // 100.00 % RH
    };
    esp_zb_attribute_list_t *hum = esp_zb_humidity_meas_cluster_create(&hum_cfg);

    esp_zb_pressure_meas_cluster_cfg_t pres_cfg = {
        .measured_value = static_cast<int16_t>(0x8000),
        .min_value = 300,
        .max_value = 1100,
    };
    esp_zb_attribute_list_t *pres = esp_zb_pressure_meas_cluster_create(&pres_cfg);

    esp_zb_cluster_list_t *clusters = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_basic_cluster(clusters, basic, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(clusters, identify, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_temperature_meas_cluster(clusters, temp, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_humidity_meas_cluster(clusters, hum, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_pressure_meas_cluster(clusters, pres, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_endpoint_config_t ep_cfg = {
        .endpoint = CLIMATE_ENDPOINT,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
        .app_device_version = 0,
    };
    esp_zb_ep_list_add_ep(ep_list, clusters, ep_cfg);
}

void climate_task(void *)
{
    while (true) {
        if (g_climate) {
            auto r = g_climate->read();

            if (esp_zb_lock_acquire(pdMS_TO_TICKS(500))) {
                if (r.temp_valid) {
                    int16_t v = static_cast<int16_t>(r.temperature_c * 100.0f);
                    esp_zb_zcl_set_attribute_val(CLIMATE_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                                 ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &v, false);
                }
                if (r.humidity_valid) {
                    uint16_t v = static_cast<uint16_t>(r.humidity_pct * 100.0f);
                    esp_zb_zcl_set_attribute_val(CLIMATE_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
                                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                                 ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID, &v, false);
                }
                if (r.pressure_valid) {
                    int16_t v = static_cast<int16_t>(r.pressure_hpa);
                    esp_zb_zcl_set_attribute_val(CLIMATE_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_PRESSURE_MEASUREMENT,
                                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                                 ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_VALUE_ID, &v, false);
                }
                esp_zb_lock_release();
                ESP_LOGI(TAG, "Climate: T=%.2f°C H=%.1f%% P=%.1fhPa", r.temperature_c, r.humidity_pct, r.pressure_hpa);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(CLIMATE_REPORT_PERIOD_MS));
    }
}
// ---------------------------------------------------------------------------
#endif  // GALOPED_CLIMATE

#if GALOPED_CO2
// --- SenseAir S8 CO2 sensor Zigbee endpoint --------------------------------
constexpr gpio_num_t S8_TX_GPIO = static_cast<gpio_num_t>(CONFIG_GALOPED_S8_UART_TX_GPIO);
constexpr gpio_num_t S8_RX_GPIO = static_cast<gpio_num_t>(CONFIG_GALOPED_S8_UART_RX_GPIO);
constexpr uart_port_t S8_UART_PORT = UART_NUM_1;
constexpr uint8_t CO2_ENDPOINT = 40;
constexpr uint32_t CO2_REPORT_PERIOD_MS = 30000;

void add_co2_endpoint(esp_zb_ep_list_t *ep_list)
{
    esp_zb_basic_cluster_cfg_t basic_cfg = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_MAINS_SINGLE_PHASE,
    };
    esp_zb_attribute_list_t *basic = esp_zb_basic_cluster_create(&basic_cfg);
    esp_zb_basic_cluster_add_attr(basic, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
                                  const_cast<char *>(MANUFACTURER_NAME));
    esp_zb_basic_cluster_add_attr(basic, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
                                  const_cast<char *>(MODEL_IDENTIFIER));

    esp_zb_identify_cluster_cfg_t identify_cfg = {.identify_time = 0};
    esp_zb_attribute_list_t *identify = esp_zb_identify_cluster_create(&identify_cfg);

    // ZCL CO2 cluster reports as fraction of 1 (e.g. 1000 ppm -> 0.001).
    // NaN = "invalid/unknown" sentinel for the measured value.
    esp_zb_carbon_dioxide_measurement_cluster_cfg_t co2_cfg = {
        .measured_value = NAN,
        .min_measured_value = 0.0f,
        .max_measured_value = 0.01f,  // 10000 ppm — S8 range
    };
    esp_zb_attribute_list_t *co2 = esp_zb_carbon_dioxide_measurement_cluster_create(&co2_cfg);

    esp_zb_cluster_list_t *clusters = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_basic_cluster(clusters, basic, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(clusters, identify, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_carbon_dioxide_measurement_cluster(clusters, co2, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_endpoint_config_t ep_cfg = {
        .endpoint = CO2_ENDPOINT,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID,
        .app_device_version = 0,
    };
    esp_zb_ep_list_add_ep(ep_list, clusters, ep_cfg);
}

void co2_task(void *)
{
    while (true) {
        if (g_co2) {
            auto r = g_co2->read();
            if (r.valid) {
                float frac = r.co2_ppm / 1000000.0f;
                if (esp_zb_lock_acquire(pdMS_TO_TICKS(500))) {
                    esp_zb_zcl_set_attribute_val(
                        CO2_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_CARBON_DIOXIDE_MEASUREMENT, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                        ESP_ZB_ZCL_ATTR_CARBON_DIOXIDE_MEASUREMENT_MEASURED_VALUE_ID, &frac, false);
                    esp_zb_lock_release();
                    ESP_LOGI(TAG, "CO2: %u ppm", r.co2_ppm);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(CO2_REPORT_PERIOD_MS));
    }
}
// ---------------------------------------------------------------------------
#endif  // GALOPED_CO2

// --- Drive (VID6608) Zigbee endpoints --------------------------------------
constexpr uint8_t DRIVE_1_ENDPOINT = 20;
constexpr uint8_t DRIVE_2_ENDPOINT = 21;
constexpr uint16_t DRIVE_CLUSTER_ID = 0xFC10;         // manufacturer-specific
constexpr uint16_t DRIVE_ATTR_POSITION_ID = 0x0000;   // S32, RW, reportable
constexpr uint16_t DRIVE_ATTR_MAX_STEPS_ID = 0x0001;  // U16, RO
constexpr uint16_t DRIVE_ATTR_IS_MOVING_ID = 0x0002;  // BOOL, RO, reportable
constexpr uint8_t DRIVE_CMD_RESET = 0x00;
constexpr uint32_t POSITION_REPORT_PERIOD_MS = 500;

esp_timer_handle_t g_drive_report_timer = nullptr;

esp32_vid6608_rmt *drive_for_endpoint(uint8_t ep)
{
    if (ep == DRIVE_1_ENDPOINT) {
        return g_drive_1.get();
    }
    if (ep == DRIVE_2_ENDPOINT) {
        return g_drive_2.get();
    }
    return nullptr;
}

void apply_drive_attr(const esp_zb_zcl_set_attr_value_message_t *msg, esp32_vid6608_rmt *drive)
{
    if (msg->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ANALOG_OUTPUT) {
        if (msg->attribute.id == ESP_ZB_ZCL_ATTR_ANALOG_OUTPUT_PRESENT_VALUE_ID) {
            float v = *reinterpret_cast<float *>(msg->attribute.data.value);
            int32_t steps = static_cast<int32_t>(v + 0.5f);
            ESP_LOGI(TAG, "EP %u AO PresentValue=%.1f -> setPos(%ld)", msg->info.dst_endpoint, v, (long)steps);
            drive->setPos(steps);
        }
    } else if (msg->info.cluster == DRIVE_CLUSTER_ID) {
        if (msg->attribute.id == DRIVE_ATTR_POSITION_ID) {
            int32_t v = *reinterpret_cast<int32_t *>(msg->attribute.data.value);
            ESP_LOGI(TAG, "EP %u Position=%ld", msg->info.dst_endpoint, (long)v);
            drive->setPos(v);
        }
    }
}

void drive_reset_task(void *arg)
{
    auto *drive = static_cast<esp32_vid6608_rmt *>(arg);
    ESP_LOGI(TAG, "Drive reset: zero() begin");
    drive->zero();
    ESP_LOGI(TAG, "Drive reset: zero() done");
    vTaskDelete(nullptr);
}

void handle_custom_cmd(const esp_zb_zcl_custom_cluster_command_message_t *msg)
{
    if (!msg || msg->info.cluster != DRIVE_CLUSTER_ID) {
        return;
    }
    auto *drive = drive_for_endpoint(msg->info.dst_endpoint);
    if (!drive) {
        return;
    }
    switch (msg->info.command.id) {
    case DRIVE_CMD_RESET:
        ESP_LOGI(TAG, "EP %u: reset command", msg->info.dst_endpoint);
        // zero() blocks for the full sweep; run in its own task to keep Zigbee responsive
        xTaskCreate(drive_reset_task, "drv_reset", 3072, drive, 4, nullptr);
        break;
    default:
        ESP_LOGW(TAG, "EP %u: unknown custom cmd 0x%02x", msg->info.dst_endpoint, msg->info.command.id);
        break;
    }
}

void add_drive_endpoint(esp_zb_ep_list_t *ep_list, uint8_t endpoint, esp32_vid6608_rmt &drive)
{
    esp_zb_basic_cluster_cfg_t basic_cfg = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_MAINS_SINGLE_PHASE,
    };
    esp_zb_attribute_list_t *basic = esp_zb_basic_cluster_create(&basic_cfg);
    esp_zb_basic_cluster_add_attr(basic, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
                                  const_cast<char *>(MANUFACTURER_NAME));
    esp_zb_basic_cluster_add_attr(basic, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
                                  const_cast<char *>(MODEL_IDENTIFIER));

    esp_zb_identify_cluster_cfg_t identify_cfg = {
        .identify_time = 0,
    };
    esp_zb_attribute_list_t *identify = esp_zb_identify_cluster_create(&identify_cfg);

    esp_zb_analog_output_cluster_cfg_t ao_cfg = {
        .out_of_service = false,
        .present_value = static_cast<float>(drive.getCurrentPosition()),
        .status_flags = 0,
    };
    esp_zb_attribute_list_t *ao = esp_zb_analog_output_cluster_create(&ao_cfg);

    int32_t init_pos = drive.getCurrentPosition();
    uint16_t init_max = drive.getMaxSteps();
    uint8_t init_moving = 0;
    esp_zb_attribute_list_t *custom = esp_zb_zcl_attr_list_create(DRIVE_CLUSTER_ID);
    esp_zb_custom_cluster_add_custom_attr(custom, DRIVE_ATTR_POSITION_ID, ESP_ZB_ZCL_ATTR_TYPE_S32,
                                          ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
                                          &init_pos);
    esp_zb_custom_cluster_add_custom_attr(custom, DRIVE_ATTR_MAX_STEPS_ID, ESP_ZB_ZCL_ATTR_TYPE_U16,
                                          ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY, &init_max);
    esp_zb_custom_cluster_add_custom_attr(custom, DRIVE_ATTR_IS_MOVING_ID, ESP_ZB_ZCL_ATTR_TYPE_BOOL,
                                          ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
                                          &init_moving);

    esp_zb_cluster_list_t *clusters = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_basic_cluster(clusters, basic, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(clusters, identify, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_analog_output_cluster(clusters, ao, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_custom_cluster(clusters, custom, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_endpoint_config_t ep_cfg = {
        .endpoint = endpoint,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_LEVEL_CONTROLLABLE_OUTPUT_DEVICE_ID,
        .app_device_version = 0,
    };
    esp_zb_ep_list_add_ep(ep_list, clusters, ep_cfg);
}

void drive_report_tick(void *)
{
    struct {
        uint8_t ep;
        esp32_vid6608_rmt *drive;
    } entries[] = {
        {DRIVE_1_ENDPOINT, g_drive_1.get()},
        {DRIVE_2_ENDPOINT, g_drive_2.get()},
    };

    if (!esp_zb_lock_acquire(pdMS_TO_TICKS(100))) {
        return;
    }
    for (auto &e : entries) {
        if (!e.drive) {
            continue;
        }
        float pv = static_cast<float>(e.drive->getCurrentPosition());
        int32_t pos = e.drive->getCurrentPosition();
        uint8_t moving = e.drive->isMoving() ? 1 : 0;

        esp_zb_zcl_set_attribute_val(e.ep, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_OUTPUT, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                     ESP_ZB_ZCL_ATTR_ANALOG_OUTPUT_PRESENT_VALUE_ID, &pv, false);
        esp_zb_zcl_set_attribute_val(e.ep, DRIVE_CLUSTER_ID, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, DRIVE_ATTR_POSITION_ID,
                                     &pos, false);
        esp_zb_zcl_set_attribute_val(e.ep, DRIVE_CLUSTER_ID, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, DRIVE_ATTR_IS_MOVING_ID,
                                     &moving, false);
    }
    esp_zb_lock_release();
}

void start_drive_reporter()
{
    esp_timer_create_args_t args = {};
    args.callback = &drive_report_tick;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "drive_rpt";
    ESP_ERROR_CHECK(esp_timer_create(&args, &g_drive_report_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(g_drive_report_timer, POSITION_REPORT_PERIOD_MS * 1000));
}
// ---------------------------------------------------------------------------

void apply_light_attr(const esp_zb_zcl_set_attr_value_message_t *msg)
{
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
        if (msg->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID) {
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

void apply_attr(const esp_zb_zcl_set_attr_value_message_t *msg)
{
    if (!msg) {
        return;
    }
    if (msg->info.dst_endpoint == HA_ENDPOINT) {
        if (g_led) {
            apply_light_attr(msg);
        }
        return;
    }
    if (auto *drive = drive_for_endpoint(msg->info.dst_endpoint)) {
        apply_drive_attr(msg, drive);
    }
}

esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t cb_id, const void *msg)
{
    switch (cb_id) {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        apply_attr(static_cast<const esp_zb_zcl_set_attr_value_message_t *>(msg));
        break;
    case ESP_ZB_CORE_CMD_CUSTOM_CLUSTER_REQ_CB_ID:
        handle_custom_cmd(static_cast<const esp_zb_zcl_custom_cluster_command_message_t *>(msg));
        break;
    default:
        break;
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
    light_cfg.color_cfg.color_mode = 0x01;            // XY (matches z2m's default command)
    light_cfg.color_cfg.enhanced_color_mode = 0x01;   // XY
    light_cfg.color_cfg.color_capabilities = 0x0008;  // bit 3 XY only
    esp_zb_ep_list_t *ep_list = esp_zb_color_dimmable_light_ep_create(HA_ENDPOINT, &light_cfg);

    esp_zb_cluster_list_t *clusters = esp_zb_ep_list_get_ep(ep_list, HA_ENDPOINT);
    esp_zb_attribute_list_t *basic =
        esp_zb_cluster_list_get_cluster(clusters, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_basic_cluster_add_attr(basic, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
                                  const_cast<char *>(MANUFACTURER_NAME));
    esp_zb_basic_cluster_add_attr(basic, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
                                  const_cast<char *>(MODEL_IDENTIFIER));

    if (g_drive_1) {
        add_drive_endpoint(ep_list, DRIVE_1_ENDPOINT, *g_drive_1);
    }
    if (g_drive_2) {
        add_drive_endpoint(ep_list, DRIVE_2_ENDPOINT, *g_drive_2);
    }

#if GALOPED_CLIMATE
    if (g_climate) {
        add_climate_endpoint(ep_list);
    }
#endif

#if GALOPED_CO2
    if (g_co2) {
        add_co2_endpoint(ep_list);
    }
#endif

    esp_zb_device_register(ep_list);
    esp_zb_core_action_handler_register(zb_action_handler);
    esp_zb_set_primary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK);

    start_drive_reporter();
#if GALOPED_CLIMATE
    if (g_climate) {
        xTaskCreate(climate_task, "climate", 4096, nullptr, 4, nullptr);
    }
#endif
#if GALOPED_CO2
    if (g_co2) {
        xTaskCreate(co2_task, "co2", 4096, nullptr, 4, nullptr);
    }
#endif

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
    ESP_LOGI(TAG, "App startup");
    ESP_LOGI(TAG, "Config: model %s, axis %d", GALOPED_MODEL, GALOPED_AXIS);

    ESP_ERROR_CHECK(nvs_flash_init());

    esp_zb_platform_config_t platform = {};
    platform.radio_config.radio_mode = ZB_RADIO_MODE_NATIVE;
    platform.host_config.host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE;
    ESP_ERROR_CHECK(esp_zb_platform_config(&platform));

    // Activate drives
    // All devices have at least 1
    ESP_LOGI(TAG, "Init: drive 1");
    esp32_vid6608_rmt::Config cfg_1{
        .stepPin = static_cast<gpio_num_t>(CONFIG_GALOPED_DRIVE_1_STEP_GPIO),
        .dirPin = static_cast<gpio_num_t>(CONFIG_GALOPED_DRIVE_1_DIR_GPIO),
        .maxSteps = CONFIG_GALOPED_DRIVE_1_MAX_STEPS,
    };
    g_drive_1 = std::make_unique<esp32_vid6608_rmt>(cfg_1);

#if GALOPED_AXIS > 1
    // Devices with 2+
    ESP_LOGI(TAG, "Init: drive 2");
    esp32_vid6608_rmt::Config cfg_2{
        .stepPin = static_cast<gpio_num_t>(CONFIG_GALOPED_DRIVE_2_STEP_GPIO),
        .dirPin = static_cast<gpio_num_t>(CONFIG_GALOPED_DRIVE_2_DIR_GPIO),
        .maxSteps = CONFIG_GALOPED_DRIVE_2_MAX_STEPS,
    };
    g_drive_2 = std::make_unique<esp32_vid6608_rmt>(cfg_2);
#endif

    if (g_drive_1) {
        g_drive_1->zero();
    }
    if (g_drive_2) {
        g_drive_2->zero();
    }

    g_led = std::make_unique<RgbLed>(LED_GPIO, SPI2_HOST, LED_COUNT);
    ESP_ERROR_CHECK(g_led->init());

    g_button = std::make_unique<Button>(BUTTON_GPIO, LONG_PRESS_MS);
    g_button->on_long_press(factory_reset);
    ESP_ERROR_CHECK(g_button->init());

#if GALOPED_CLIMATE
    ESP_LOGI(TAG, "Init: climate sensor");
    g_climate = std::make_unique<ClimateSensor>(I2C_SDA_GPIO, I2C_SCL_GPIO);
    if (g_climate->init() != ESP_OK) {
        ESP_LOGE(TAG, "Climate sensor init failed; endpoint will not be added");
        g_climate.reset();
    }
#endif

#if GALOPED_CO2
    ESP_LOGI(TAG, "Init: SenseAir S8");
    g_co2 = std::make_unique<SenseAirS8>(S8_UART_PORT, S8_TX_GPIO, S8_RX_GPIO);
    if (g_co2->init() != ESP_OK) {
        ESP_LOGE(TAG, "SenseAir S8 init failed; endpoint will not be added");
        g_co2.reset();
    }
#endif

    xTaskCreate(zb_task, "zb_main", 4096, nullptr, 5, nullptr);
}
