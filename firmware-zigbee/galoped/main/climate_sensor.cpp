#include "climate_sensor.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "CLIMATE";

namespace
{
constexpr uint32_t I2C_FREQ_HZ = 100000;
constexpr int I2C_TIMEOUT_MS = 100;
}  // namespace

ClimateSensor::ClimateSensor(gpio_num_t sda, gpio_num_t scl, i2c_port_num_t port)
    : sda_(sda),
      scl_(scl),
      port_(port),
      bus_(nullptr),
      bmp_(nullptr),
      aht_(nullptr),
      bmp_present_(false),
      aht_present_(false),
      bmp_addr_(0),
      bmp_chip_id_(0),
      bmp_cal_{}
{
}

ClimateSensor::~ClimateSensor()
{
    if (bmp_) {
        i2c_master_bus_rm_device(bmp_);
    }
    if (aht_) {
        i2c_master_bus_rm_device(aht_);
    }
    if (bus_) {
        i2c_del_master_bus(bus_);
    }
}

void ClimateSensor::bus_scan()
{
    ESP_LOGI(TAG, "I2C scan on sda=%d scl=%d:", sda_, scl_);
    for (uint8_t addr = 0x08; addr < 0x78; ++addr) {
        if (i2c_master_probe(bus_, addr, 50) == ESP_OK) {
            ESP_LOGI(TAG, "  found 0x%02x", addr);
        }
    }
}

esp_err_t ClimateSensor::bmp280_probe(uint8_t addr)
{
    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.scl_speed_hz = I2C_FREQ_HZ;
    dev_cfg.device_address = addr;

    i2c_master_dev_handle_t h;
    if (i2c_master_bus_add_device(bus_, &dev_cfg, &h) != ESP_OK) {
        return ESP_FAIL;
    }
    uint8_t reg = 0xD0;  // chip_id register
    uint8_t id = 0;
    esp_err_t err = i2c_master_transmit_receive(h, &reg, 1, &id, 1, I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        i2c_master_bus_rm_device(h);
        return err;
    }
    // 0x58 = BMP280, 0x60 = BME280 (same temp/pressure registers + cal layout)
    if (id != 0x58 && id != 0x60) {
        ESP_LOGW(TAG, "  0x%02x: unexpected chip_id 0x%02x", addr, id);
        i2c_master_bus_rm_device(h);
        return ESP_ERR_NOT_FOUND;
    }
    bmp_ = h;
    bmp_addr_ = addr;
    bmp_chip_id_ = id;
    ESP_LOGI(TAG, "BMP280 at 0x%02x (chip_id=0x%02x %s)", addr, id, id == 0x60 ? "BME280" : "BMP280");
    return ESP_OK;
}

esp_err_t ClimateSensor::init()
{
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = port_;
    bus_cfg.sda_io_num = sda_;
    bus_cfg.scl_io_num = scl_;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &bus_), TAG, "i2c bus");

    bus_scan();

    // BMP280: try both possible addresses
    if (bmp280_probe(BMP280_ADDR_PRIMARY) == ESP_OK || bmp280_probe(BMP280_ADDR_ALT) == ESP_OK) {
        if (bmp280_init() == ESP_OK) {
            bmp_present_ = true;
        } else {
            ESP_LOGW(TAG, "BMP280 found but cal read failed");
        }
    } else {
        ESP_LOGW(TAG, "BMP280 not found at 0x76 or 0x77");
    }

    // AHT20
    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.scl_speed_hz = I2C_FREQ_HZ;
    dev_cfg.device_address = AHT20_ADDR;
    if (i2c_master_bus_add_device(bus_, &dev_cfg, &aht_) == ESP_OK && aht20_init() == ESP_OK) {
        aht_present_ = true;
        ESP_LOGI(TAG, "AHT20 at 0x%02x", AHT20_ADDR);
    } else {
        ESP_LOGW(TAG, "AHT20 not responding at 0x%02x", AHT20_ADDR);
    }

    if (!bmp_present_ && !aht_present_) {
        ESP_LOGE(TAG, "Neither sensor responded on the bus");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "init ok: bmp=%d aht=%d", bmp_present_, aht_present_);
    return ESP_OK;
}

// --- BMP280 ----------------------------------------------------------------

esp_err_t ClimateSensor::bmp280_init()
{
    uint8_t reg = 0x88;
    uint8_t cal[24];
    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(bmp_, &reg, 1, cal, sizeof(cal), I2C_TIMEOUT_MS), TAG, "cal read");

    bmp_cal_.T1 = static_cast<uint16_t>(cal[0] | (cal[1] << 8));
    bmp_cal_.T2 = static_cast<int16_t>(cal[2] | (cal[3] << 8));
    bmp_cal_.T3 = static_cast<int16_t>(cal[4] | (cal[5] << 8));
    bmp_cal_.P1 = static_cast<uint16_t>(cal[6] | (cal[7] << 8));
    bmp_cal_.P2 = static_cast<int16_t>(cal[8] | (cal[9] << 8));
    bmp_cal_.P3 = static_cast<int16_t>(cal[10] | (cal[11] << 8));
    bmp_cal_.P4 = static_cast<int16_t>(cal[12] | (cal[13] << 8));
    bmp_cal_.P5 = static_cast<int16_t>(cal[14] | (cal[15] << 8));
    bmp_cal_.P6 = static_cast<int16_t>(cal[16] | (cal[17] << 8));
    bmp_cal_.P7 = static_cast<int16_t>(cal[18] | (cal[19] << 8));
    bmp_cal_.P8 = static_cast<int16_t>(cal[20] | (cal[21] << 8));
    bmp_cal_.P9 = static_cast<int16_t>(cal[22] | (cal[23] << 8));

    // ctrl_meas: osrs_t=x1 (001), osrs_p=x1 (001), mode=normal (11) -> 0x27
    uint8_t ctrl[] = {0xF4, 0x27};
    ESP_RETURN_ON_ERROR(i2c_master_transmit(bmp_, ctrl, sizeof(ctrl), I2C_TIMEOUT_MS), TAG, "ctrl_meas");

    // config: t_sb=500ms (100), filter off (000), spi3w_en=0
    uint8_t cfg[] = {0xF5, (4 << 5)};
    return i2c_master_transmit(bmp_, cfg, sizeof(cfg), I2C_TIMEOUT_MS);
}

esp_err_t ClimateSensor::bmp280_read_raw(int32_t &t_raw, int32_t &p_raw)
{
    uint8_t reg = 0xF7;
    uint8_t buf[6];
    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(bmp_, &reg, 1, buf, sizeof(buf), I2C_TIMEOUT_MS), TAG, "bmp read");
    p_raw = (static_cast<int32_t>(buf[0]) << 12) | (static_cast<int32_t>(buf[1]) << 4) | (buf[2] >> 4);
    t_raw = (static_cast<int32_t>(buf[3]) << 12) | (static_cast<int32_t>(buf[4]) << 4) | (buf[5] >> 4);
    return ESP_OK;
}

// Bosch reference compensation; outputs t_fine for the pressure formula.
float ClimateSensor::bmp280_compensate_temp(int32_t adc_T, int32_t &t_fine)
{
    int32_t var1 =
        ((((adc_T >> 3) - (static_cast<int32_t>(bmp_cal_.T1) << 1))) * static_cast<int32_t>(bmp_cal_.T2)) >> 11;
    int32_t var2 =
        (((((adc_T >> 4) - static_cast<int32_t>(bmp_cal_.T1)) * ((adc_T >> 4) - static_cast<int32_t>(bmp_cal_.T1))) >>
          12) *
         static_cast<int32_t>(bmp_cal_.T3)) >>
        14;
    t_fine = var1 + var2;
    return ((t_fine * 5 + 128) >> 8) / 100.0f;
}

float ClimateSensor::bmp280_compensate_pressure(int32_t adc_P, int32_t t_fine)
{
    int64_t var1 = static_cast<int64_t>(t_fine) - 128000;
    int64_t var2 = var1 * var1 * static_cast<int64_t>(bmp_cal_.P6);
    var2 = var2 + ((var1 * static_cast<int64_t>(bmp_cal_.P5)) << 17);
    var2 = var2 + (static_cast<int64_t>(bmp_cal_.P4) << 35);
    var1 =
        ((var1 * var1 * static_cast<int64_t>(bmp_cal_.P3)) >> 8) + ((var1 * static_cast<int64_t>(bmp_cal_.P2)) << 12);
    var1 = (((static_cast<int64_t>(1) << 47) + var1)) * static_cast<int64_t>(bmp_cal_.P1) >> 33;
    if (var1 == 0) {
        return 0.0f;  // div by zero guard from datasheet
    }
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (static_cast<int64_t>(bmp_cal_.P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (static_cast<int64_t>(bmp_cal_.P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (static_cast<int64_t>(bmp_cal_.P7) << 4);
    return static_cast<float>(p) / 256.0f / 100.0f;  // Pa -> hPa
}

// --- AHT20 -----------------------------------------------------------------

esp_err_t ClimateSensor::aht20_init()
{
    vTaskDelay(pdMS_TO_TICKS(40));
    uint8_t cmd[] = {0xBE, 0x08, 0x00};
    return i2c_master_transmit(aht_, cmd, sizeof(cmd), I2C_TIMEOUT_MS);
}

esp_err_t ClimateSensor::aht20_read(float &humidity, float &temperature)
{
    uint8_t trigger[] = {0xAC, 0x33, 0x00};
    ESP_RETURN_ON_ERROR(i2c_master_transmit(aht_, trigger, sizeof(trigger), I2C_TIMEOUT_MS), TAG, "trig");

    vTaskDelay(pdMS_TO_TICKS(80));

    uint8_t d[7];
    ESP_RETURN_ON_ERROR(i2c_master_receive(aht_, d, sizeof(d), I2C_TIMEOUT_MS), TAG, "aht read");
    if (d[0] & 0x80) {
        return ESP_ERR_NOT_FINISHED;
    }

    uint32_t raw_h = (static_cast<uint32_t>(d[1]) << 12) | (static_cast<uint32_t>(d[2]) << 4) | (d[3] >> 4);
    uint32_t raw_t = (static_cast<uint32_t>(d[3] & 0x0F) << 16) | (static_cast<uint32_t>(d[4]) << 8) | d[5];

    humidity = (raw_h * 100.0f) / 1048576.0f;
    temperature = (raw_t * 200.0f) / 1048576.0f - 50.0f;
    return ESP_OK;
}

// --- Public read -----------------------------------------------------------

ClimateSensor::Reading ClimateSensor::read()
{
    Reading r = {};

    if (bmp_present_) {
        int32_t t_raw, p_raw, t_fine = 0;
        if (bmp280_read_raw(t_raw, p_raw) == ESP_OK) {
            // temperature needed only to derive t_fine for pressure compensation
            bmp280_compensate_temp(t_raw, t_fine);
            r.pressure_hpa = bmp280_compensate_pressure(p_raw, t_fine);
            r.pressure_valid = true;
        }
    }

    if (aht_present_) {
        float h, t;
        if (aht20_read(h, t) == ESP_OK) {
            r.humidity_pct = h;
            r.temperature_c = t;
            r.humidity_valid = true;
            r.temp_valid = true;
        }
    }

    return r;
}
