#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <functional>

class Button
{
   public:
    using Callback = std::function<void()>;

    Button(gpio_num_t pin, uint32_t long_press_ms = 3000);
    ~Button();

    Button(const Button &) = delete;
    Button &operator=(const Button &) = delete;

    esp_err_t init();

    void on_click(Callback cb)
    {
        click_cb_ = std::move(cb);
    }
    void on_long_press(Callback cb)
    {
        long_press_cb_ = std::move(cb);
    }

   private:
    static void IRAM_ATTR isr_handler(void *arg);
    static void task_trampoline(void *arg);
    static void long_press_timer_cb(void *arg);

    void task_loop();

    gpio_num_t pin_;
    uint32_t long_press_ms_;

    QueueHandle_t evt_queue_;
    esp_timer_handle_t long_press_timer_;
    TaskHandle_t task_handle_;

    Callback click_cb_;
    Callback long_press_cb_;
    volatile bool long_press_fired_;
};
