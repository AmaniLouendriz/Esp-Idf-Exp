#include "Wifi.h"
#include <esp_mac.h>

namespace WIFI
{

char Wifi::mac_addr_cstr[]{};

//std::atomic_bool Wifi::first_call {false};

std::mutex Wifi::first_call_mutex;

bool Wifi::first_call {false};
/* get_mac() is a method that uses the Espressif IDF to get the mac address of the chip
*/
esp_err_t Wifi::_get_mac()
    {
        // static char mac[13] {}; 
        // we have 6 bytes, each  byte is worth 2 hex digits, so we need 12 hex digits, plus the null terminator. which is 13.
        // important to make this static because the function returns a pointer to a char, if not static, once we leave the function, the memory is going to be 
        // delocated for the variable mac, meaning if we want to access mac after the function returns, we would mainly get an undefined behavior. Making it static 
        // means that we allocate the memory once, and it's going to be accessible through the code as a global variable, it is going to get destroyed when the program 
        // ends. A bit heavy if used inside a loop but ofc it's better than undefined behavior.

        // It's better to make this variable an attribute of the class, because it is expensive to check each and every time whether the memory for it has been 
        // allocated before or not.
    
        uint8_t mac_byte_buffer[6] {};// espressif website says length should be 6. A mac is generally 6 bytes.
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