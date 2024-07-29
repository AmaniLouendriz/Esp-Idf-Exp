#pragma once

#include "esp_wifi.h"

#include "freertos/semphr.h"

#define pdSECOND pdMS_TO_TICKS(1000)

namespace WIFI
{

class Wifi
{
public:

    enum class state_e
    {//strongly typed enum, number is meaningless, word is important

        NOT_INITIALISED,
        INITIALISED,
        WAITING_FOR_CREDENTIALS,
        READY_TO_CONNECT,//doesn't say if network exists or if credentials are correct. Attempt to connect to the network
        CONNECTING,
        WAITING_FOR_IP,//in between states, connected to the constructor but not been assigned an ip address
        CONNECTED,
        DISCONNECTED,
        ERROR
    };

    Wifi();
    esp_err_t init(void); // set everything up
    esp_err_t begin(void); // start wifi, connect,etc

    state_e get_state(void); //initialized, not initialized, connected, not, got ip address, not an ip address
    const char* get_mac() 
        {return mac_addr_cstr;}

protected:
    static SemaphoreHandle_t first_call_mutex;



private:
    void state_machine(void);// internal machine

    static char mac_addr_cstr[13];// all instances of this class share the same mac address.
    esp_err_t _get_mac(void); // the user should NOT call this function directly

    //static std::atomic_bool first_call; // std::atomic_bool means the boolean is atomic, meaning if a thread wants to change its value, it should first 
    // get a lock, that's ensure that the critical section above is well protected.

    //static SemaphoreHandle_t first_call_mutex; // pointer to the mutex or handle as free rtos calls it, memory getting allocated at runTime because it's on the 
    //heap memory
    static StaticSemaphore_t first_call_mutex_buffer;
    static bool first_call;

};

}// namespace WIFI