#include "Wifi.h"
#include <esp_mac.h>
#include <esp_log.h>
#include <mutex>

namespace WIFI
{

char                Wifi::mac_addr_cstr[] {};
std::mutex          Wifi::init_mutex {};
std::mutex          Wifi::connect_mutex {};
std::mutex          Wifi::state_mutex {};
Wifi::state_e       Wifi::_state {state_e::NOT_INITIALISED};
wifi_init_config_t  Wifi::wifi_init_config = WIFI_INIT_CONFIG_DEFAULT(); // TODO whats wrong here
wifi_config_t       Wifi::wifi_config {};

Wifi::Wifi()
{
    // only one thread should get here at a time
    std::lock_guard<std::mutex> guard(init_mutex);

    if (!get_mac()[0]) { // mac address is still they way we initialized it
        if (ESP_OK != _get_mac()) {
            esp_restart();
        }
    }// if not, then we already know mac address, no need to call again.
}

void Wifi::event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (WIFI_EVENT == event_base) {
        return wifi_event_handler(arg,event_base,event_id,event_data);
    }

    else if (IP_EVENT == event_base) {
        return ip_event_handler(arg,event_base,event_id,event_data);
    }

    else {
        ESP_LOGE("myWIFI", "Unexpected event: %s", event_base);
    }
}

void Wifi::wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (WIFI_EVENT == event_base) {
        const  wifi_event_t event_type {static_cast<wifi_event_t>(event_id)};

        switch(event_type)
        {
            case WIFI_EVENT_STA_START:
            {
                std::lock_guard<std::mutex> state_guard(state_mutex);// recursive mutex??
                _state = state_e::READY_TO_CONNECT;
                break;
            }
            case WIFI_EVENT_STA_CONNECTED: // we are connected to the router, that doesn't mean is that the wifi is ready to use. because we don't have an ip yet. That doens't mean I have 
            // internet or network access
            {    std::lock_guard<std::mutex> state_guard(state_mutex);// recursive mutex??
                _state = state_e::WAITING_FOR_IP;
                break;
            }
            default:
                break;
        }
    }
}

void Wifi::ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    const  ip_event_t event_type {static_cast<ip_event_t>(event_id)};

    switch(event_type)
    {
        case IP_EVENT_STA_GOT_IP:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);// recursive mutex??
            _state = state_e::CONNECTED;
            break;
        }
        case IP_EVENT_STA_LOST_IP:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);// recursive mutex??
            _state = state_e::WAITING_FOR_IP;
            break;
        }
        default:
            break;
    }
}



esp_err_t Wifi::init(void)
{
    return _init();
}

esp_err_t Wifi::_init(void)
{
    // this is being called once and once per thread
    std::lock_guard<std::mutex> init_guard(init_mutex);

    esp_err_t status {ESP_OK};

    std::lock_guard<std::mutex> state_guard(state_mutex);

    if (state_e::NOT_INITIALISED == _state) {
        // one time initialization
        status = esp_netif_init();
        if (ESP_OK == status) {
            const esp_netif_t* const p_netif =  esp_netif_create_default_wifi_sta();

            if (!p_netif) {
                status = ESP_FAIL;
            }
        }
        // default configuration for Wifi instance

        if (ESP_OK == status)
        {
            status = esp_wifi_init(&wifi_init_config); // pass by address
        }

        if (ESP_OK == status)
        {
            status = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler,nullptr,nullptr);
        }

        if (ESP_OK == status)
        {
            status = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler,nullptr,nullptr);
        }

        if (ESP_OK == status)
        {
            status = esp_wifi_set_mode(WIFI_MODE_STA);
        }

        if (ESP_OK == status)
        {
            // old C way
            // wifi_config_t wifi_config = {
            //     .sta = {// setting the wifi station
            //         .ssid = EXAMPLE_ESP_WIFI_SSID,
            //         .password = EXAMPLE_ESP_WIFI_PASS,
            //         .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            //         .pmf_cfg = {
            //             .capable = true,
            //             .required = false 
            //         },
            //     }
            // }; // we are passing the stuff to the driver
            const size_t ssid_length_to_copy {std::min(strlen(ssid),sizeof(wifi_config.sta.ssid))};
            memcpy(wifi_config.sta.ssid,ssid,ssid_length_to_copy);
            if (sizeof(wifi_config.sta.ssid) < strlen(ssid)) {
                status = ESP_FAIL;
            }
            const size_t pwd_length_to_copy {std::min(strlen(password),sizeof(wifi_config.sta.ssid))};

            memcpy(wifi_config.sta.password,password,pwd_length_to_copy);

            wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
            wifi_config.sta.pmf_cfg.capable = true;
            wifi_config.sta.pmf_cfg.required = false;
            
            status = esp_wifi_set_config(WIFI_IF_STA,&wifi_config); // pass by address, TODO: another param for the function is needed
        }

        if (ESP_OK == status) {
            status = esp_wifi_start();
        }

        if (ESP_OK == status) {
            _state = state_e::INITIALISED;
        }

    } else if(state_e::ERROR == _state)
    {
        status = ESP_FAIL;
    }

    return status;
}

esp_err_t Wifi::begin(void) 
{
    std::lock_guard<std::mutex> connect_guard(connect_mutex);
    esp_err_t status {ESP_OK};
    std::lock_guard<std::mutex> state_guard(state_mutex);

    switch(_state)
    {
        case state_e::READY_TO_CONNECT:
            status = esp_wifi_connect();

            if (ESP_OK == status) {
                _state = state_e::CONNECTING;
            }
            break;
        case state_e::CONNECTING:
        case state_e::WAITING_FOR_IP:
        case state_e::CONNECTED:
            break;
        case state_e::NOT_INITIALISED:
        case state_e::INITIALISED:
        case state_e::DISCONNECTED:
        case state_e::ERROR:
            status =  ESP_FAIL;
            break;
    }

    return status;

}
/* get_mac() is a method that uses the Espressif IDF to get the mac address of the chip, and it converts it to ASCII Hex c style string */
esp_err_t Wifi::_get_mac()
{    
    uint8_t mac_byte_buffer[6] {};// A mac is generally 6 bytes.
    // the mac coming from the api is going to be put in this variable

    const esp_err_t status { esp_efuse_mac_get_default(mac_byte_buffer)};// get the mac address from the idf

    if (ESP_OK == status) {
        snprintf(mac_addr_cstr,sizeof(mac_addr_cstr),"%02X%02X%02X%02X%02X%02X",
        mac_byte_buffer[0],
        mac_byte_buffer[1],
        mac_byte_buffer[2],
        mac_byte_buffer[3],
        mac_byte_buffer[4],
        mac_byte_buffer[5]);// format the mac coming from the api accordingly, by making sure every byte is output as a two digit hex.
    }
    return status;
}// not calling this a lot because underlined data is unlikely to change

}// namespace WIFI