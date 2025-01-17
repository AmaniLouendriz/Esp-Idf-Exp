#pragma once

// including ESP_IDF related headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define pdSECOND pdMS_TO_TICKS(1000)
#include "esp_event.h"
#include "nvs_flash.h"

// including our custom headers
#include "Gpio.h"
#include "Wifi.h"
#include "SntpTime.h"

class Main final
{
public:
    Main(void) :
        sntp {SNTP::Sntp::get_instance()} {}
    esp_err_t setup(void);
    void loop(void);

    Gpio::GpioOutput led {GPIO_NUM_23,false};
    WIFI::Wifi wifi;
    SNTP::Sntp& sntp;

};