#include "main.h"


#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#include "esp_log.h"

#define LOG_TAG "MAIN"

static Main my_main;// what does static mean here?

esp_err_t Main::setup(void)
{
    esp_err_t status {ESP_OK};
    ESP_LOGI(LOG_TAG, "Set up!");

    status |= led.init();
    status |= wifi.init();

    if (ESP_OK == status) {
        status |= wifi.begin();
    }
    status |= sntp.init();
    return status;
}

void Main::loop(void)
{
    ESP_LOGI(LOG_TAG, "Hello World!");

    ESP_LOGI(LOG_TAG, "Led on");
    led.set(true);
    vTaskDelay(pdSECOND);
    ESP_LOGI(LOG_TAG, "Led off");
    led.set(false);
    vTaskDelay(pdSECOND);
}

extern "C" void app_main(void)
{
    ESP_LOGI(LOG_TAG,"Creating default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(LOG_TAG,"Initializing NVS");// not sure if this flushes anything

    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(my_main.setup());
    while (true)
    {
        my_main.loop();
    }
}