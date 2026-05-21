#include "button.h"

#include "esp_log.h"

static const char *TAG = "BTN";

namespace
{
constexpr uint32_t DEBOUNCE_MS = 30;
}

Button::Button(gpio_num_t pin, uint32_t long_press_ms)
    : pin_(pin),
      long_press_ms_(long_press_ms),
      evt_queue_(nullptr),
      long_press_timer_(nullptr),
      task_handle_(nullptr),
      long_press_fired_(false)
{
}

Button::~Button()
{
    if (task_handle_) {
        vTaskDelete(task_handle_);
    }
    if (long_press_timer_) {
        esp_timer_stop(long_press_timer_);
        esp_timer_delete(long_press_timer_);
    }
    gpio_isr_handler_remove(pin_);
    if (evt_queue_) {
        vQueueDelete(evt_queue_);
    }
}

esp_err_t Button::init()
{
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << pin_;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_ANYEDGE;
    ESP_ERROR_CHECK(gpio_config(&io));

    evt_queue_ = xQueueCreate(8, sizeof(uint32_t));
    if (!evt_queue_) {
        return ESP_ERR_NO_MEM;
    }

    esp_timer_create_args_t timer_args = {};
    timer_args.callback = &Button::long_press_timer_cb;
    timer_args.arg = this;
    timer_args.dispatch_method = ESP_TIMER_TASK;
    timer_args.name = "btn_lp";
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &long_press_timer_));

    if (xTaskCreate(&Button::task_trampoline, "btn_task", 3072, this, 5, &task_handle_) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    // ISR service may already be installed elsewhere; tolerate that
    esp_err_t isr_err = gpio_install_isr_service(0);
    if (isr_err != ESP_OK && isr_err != ESP_ERR_INVALID_STATE) {
        return isr_err;
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(pin_, &Button::isr_handler, this));

    ESP_LOGI(TAG, "init pin=%d long_press=%lums", pin_, (unsigned long)long_press_ms_);
    return ESP_OK;
}

void IRAM_ATTR Button::isr_handler(void *arg)
{
    auto *self = static_cast<Button *>(arg);
    uint32_t level = gpio_get_level(self->pin_);
    BaseType_t hpw = pdFALSE;
    xQueueSendFromISR(self->evt_queue_, &level, &hpw);
    if (hpw == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void Button::task_trampoline(void *arg)
{
    static_cast<Button *>(arg)->task_loop();
}

void Button::long_press_timer_cb(void *arg)
{
    auto *self = static_cast<Button *>(arg);
    self->long_press_fired_ = true;
    if (self->long_press_cb_) {
        self->long_press_cb_();
    }
}

void Button::task_loop()
{
    uint32_t level;
    TickType_t last_change = 0;

    while (true) {
        if (xQueueReceive(evt_queue_, &level, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        TickType_t now = xTaskGetTickCount();
        if ((now - last_change) * portTICK_PERIOD_MS < DEBOUNCE_MS) {
            continue;
        }
        last_change = now;

        // Re-read after debounce window; ISR may have fired on bounce
        level = gpio_get_level(pin_);

        if (level == 0) {
            // pressed (active low with pull-up)
            long_press_fired_ = false;
            esp_timer_stop(long_press_timer_);
            esp_timer_start_once(long_press_timer_, (uint64_t)long_press_ms_ * 1000);
        } else {
            esp_timer_stop(long_press_timer_);
            if (!long_press_fired_ && click_cb_) {
                click_cb_();
            }
        }
    }
}
