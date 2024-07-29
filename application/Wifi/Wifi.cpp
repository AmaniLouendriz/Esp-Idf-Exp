#include "Wifi.h"
#include <esp_mac.h>
#include <esp_log.h>

namespace WIFI
{

char Wifi::mac_addr_cstr[]{};

//std::atomic_bool Wifi::first_call {false};

SemaphoreHandle_t Wifi::first_call_mutex {nullptr};
StaticSemaphore_t Wifi::first_call_mutex_buffer {};// default constructor, chunk of memory statically allocated at compile time

bool Wifi::first_call {true};

Wifi::Wifi()
{
    if (!first_call_mutex) { // if the mutext doesn't exist, create it
        //first_call_mutex = xSemaphoreCreateMutex(); // originally this mutex is on the heap memory, the semaphore is in a take state state
        first_call_mutex = xSemaphoreCreateRecursiveMutexStatic(&first_call_mutex_buffer);// a recursive mutex ensures the following:
        // ChildOfWifi
        // {

            // ...
            // xSemaphoreTake(first_call_mutx,pDsecond);// child class of wifi took the semaphore
            // Wifi(); // inside here, if it is the first call, the thread will try to acquire the semaphore, but it is already taken by itself, so it's gonna take
            // it again (it's behaving like a counter semaphore with 2 permits), but when it will give it back, it should give it the number of times it took it
            // so 2. if this wasn't a recursive semaphore, it wouldn't do that. it will block inside the base class constructor because we need the semaphore but we can't take
            // it again bc we already have it.
            // xSemaphoreGive(first_call_mutx);
        
        
        
        
        
        //}
        configASSERT(first_call_mutex);
        // if the thing inside ths assert is evaluated to false, espressif calls abort, but it gives you a trace on which line the problem happened
        // first_call_mutex != nullptr (that's what the above means)

        configASSERT(pdPASS == xSemaphoreGive(first_call_mutex));
    }

    // There is a bug here, if it's our first call, and for some reason we can't obtain the semaphore, then we exit the function first without turning
    // the first_call to false, and we don;t get any mac address. Depending on the number of times this constructor is executed, we may get different behaviors,
    // and what the variable is  claiming to be first-call might not be!
    // if (first_call && pdPASS == xSemaphoreTake(first_call_mutex,pdSECOND)) {
    //     if (ESP_OK != _get_mac()) {
    //         esp_restart(); // reboot the chip
    //     }
    //     first_call = false;
    //     xSemaphoreGive(first_call_mutex);
    // }

    bool it_worked {false};
    for (int i = 0; i<3 ; i++) { // max 3 times to acquire the semaphore
        if (pdPASS == xSemaphoreTake(first_call_mutex,pdSECOND)) {
            if (first_call) {
                if (ESP_OK != _get_mac()) {
                    esp_restart();
                }
                first_call = false;
            }
            xSemaphoreGive(first_call_mutex);
            it_worked = true;
            break;
        } else {
            // esp_restart();
            ESP_LOGW("WIFI","Failed to get mutex (attempt %u)",i+1);
            continue;
        }
    }

    if (!it_worked){
        esp_restart();
    }
        
}


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