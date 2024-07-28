#include "Gpio.h"

namespace Gpio
{
    [[nodiscard]] esp_err_t GpioBase::init(void)
    {
        esp_err_t status {ESP_OK};

        status |= gpio_config(&_cfg);// why the  &_cfg

        return status;
    }
    [[nodicard]] esp_err_t GpioOutput::init(void)
    {
        esp_err_t status {GpioBase::init()};

        if (ESP_OK == status ) {
            status |= set(_inverted_logic);
        }

        return status;
    }
    esp_err_t GpioOutput::set(const bool state)
    {
        _state = state;
        // if (state && _inverted_logic) {
        //     false
        // } else if(state)
        // {
        //     true
        // }
        // if (!state && _inverted_logic) {
        //     true
        // } else if(!state)
        // {
        //     false
        // }

        return gpio_set_level(_pin,_inverted_logic ? !state : state);
    }
}