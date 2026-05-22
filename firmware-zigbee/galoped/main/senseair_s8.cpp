#include "senseair_s8.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "S8";

namespace
{
constexpr int UART_BAUD = 9600;
constexpr int UART_RX_BUF = 256;
constexpr int RESPONSE_TIMEOUT_MS = 200;
constexpr size_t REQUEST_LEN = 8;
constexpr size_t RESPONSE_LEN = 7;
}  // namespace

SenseAirS8::SenseAirS8(uart_port_t port, gpio_num_t tx, gpio_num_t rx)
    : port_(port), tx_(tx), rx_(rx), installed_(false)
{
}

SenseAirS8::~SenseAirS8()
{
    if (installed_) {
        uart_driver_delete(port_);
    }
}

esp_err_t SenseAirS8::init()
{
    uart_config_t cfg = {};
    cfg.baud_rate = UART_BAUD;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;

    ESP_RETURN_ON_ERROR(uart_driver_install(port_, UART_RX_BUF, 0, 0, nullptr, 0), TAG, "install");
    ESP_RETURN_ON_ERROR(uart_param_config(port_, &cfg), TAG, "config");
    ESP_RETURN_ON_ERROR(uart_set_pin(port_, tx_, rx_, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG, "pin");
    installed_ = true;
    ESP_LOGI(TAG, "SenseAir S8 init: port=%d tx=%d rx=%d", port_, tx_, rx_);
    return ESP_OK;
}

uint16_t SenseAirS8::crc16_modbus(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

SenseAirS8::Reading SenseAirS8::read()
{
    Reading r = {};
    if (!installed_) {
        return r;
    }

    // Modbus RTU: addr=0xFE (any-slave), func 0x04 (read input registers),
    // start=0x0003 (CO2 ppm), count=0x0001
    uint8_t req[REQUEST_LEN] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00};
    uint16_t crc = crc16_modbus(req, REQUEST_LEN - 2);
    req[6] = crc & 0xFF;
    req[7] = (crc >> 8) & 0xFF;

    uart_flush_input(port_);
    int n = uart_write_bytes(port_, req, sizeof(req));
    if (n != static_cast<int>(sizeof(req))) {
        ESP_LOGW(TAG, "uart write %d/%zu", n, sizeof(req));
        return r;
    }

    uint8_t resp[RESPONSE_LEN];
    int got = uart_read_bytes(port_, resp, sizeof(resp), pdMS_TO_TICKS(RESPONSE_TIMEOUT_MS));
    if (got != static_cast<int>(sizeof(resp))) {
        ESP_LOGW(TAG, "uart read timeout (%d bytes)", got);
        return r;
    }
    if (resp[0] != 0xFE || resp[1] != 0x04 || resp[2] != 0x02) {
        ESP_LOGW(TAG, "bad header: %02x %02x %02x", resp[0], resp[1], resp[2]);
        return r;
    }
    uint16_t want = crc16_modbus(resp, RESPONSE_LEN - 2);
    uint16_t have = static_cast<uint16_t>(resp[5]) | (static_cast<uint16_t>(resp[6]) << 8);
    if (want != have) {
        ESP_LOGW(TAG, "crc mismatch want=%04x have=%04x", want, have);
        return r;
    }
    r.co2_ppm = (static_cast<uint16_t>(resp[3]) << 8) | resp[4];
    r.valid = true;
    return r;
}
