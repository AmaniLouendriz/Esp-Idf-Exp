#pragma once

#include "esp_wifi.h"
//#include <atomic>
#include <mutex>

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

    Wifi()
    {
        //static esp_err_t val = _get_mac(); // static is heavy in term of computations
        // static bool first_call = false;

        // May get problematic when you have multi threads, because if running in parallel, they may both have first_call initialized to false, then they will both 
        // execute the code block inside, meaning the code block would be executed twice. it's a critical section so just one thread should be doing it at a time.

        // first acquire the lock
        std::lock_guard<std::mutex> guard(first_call_mutex); // scope based lock. Lock is released when we go out of scope
        // only one thread will run this part
        if (!first_call) {
            if (ESP_OK != _get_mac()) {
                esp_restart(); // reboot the chip
            }
            first_call = true;
        }
        //if (ESP_OK != _get_mac()) esp_restart();
        // abort() and esp_restart() reboots the processor
        // we can also use std::call_once
    }
    esp_err_t init(void); // set everything up
    esp_err_t begin(void); // start wifi, connect,etc

    state_e get_state(void); //initialized, not initialized, connected, not, got ip address, not an ip address
    const char* get_mac() 
        {return mac_addr_cstr;}

private:
    void state_machine(void);// internal machine

    static char mac_addr_cstr[13];// all instances of this class share the same mac address.
    esp_err_t _get_mac(void); // the user should call this function directly

    //static std::atomic_bool first_call; // std::atomic_bool means the boolean is atomic, meaning if a thread wants to change its value, it should first 
    // get a lock, that's ensure that the critical section above is well protected.

    static std::mutex first_call_mutex;
    static bool first_call;

};

}// namespace WIFI