#ifndef IDB_CONFIG_H
#define IDB_CONFIG_H

#include "hardware/rtc.h"

#define PDT -7
#define PST -8
#define ZONE PST

// app config
#define IDB_CHECKUP 1
#define IDB_CHECKUP_INTERVAL 120
#define IDB_PING_ADDR "192.168.50.1"
#define IDB_PING_ON 0
#define IDB_PING_COUNT 1
#define IDB_STATS 0
#define IDB_ROUTE_TEST 0
#define IDB_WIFI_SCAN 1
#define IDB_WIFI_SCAN_COUNT 2
#define IDB_WORLD_TIME_API 0 // requires a special branch of pico-sdk
#define IDB_DATA_LOGGING 0
#define IDB_INI_TEST 0

#define WS_KEEPALIVE_INTERVAL_MS 5000
#define MAX_WS_MESSAGE 256

/**
 * 
 * Pinout - we really only have 12 pins available wthout an extender
 * if we keep two free for an I2C connector for extendability
 * plus the UART for debugging
 * The other pins are taken for SPI / SD Card
 * 
*/

// Digitial Input
#define DI01 19  
#define DI02 20 // this is K3 on Pico Expansion Plus S1
#define DI03 21 // this is K4 on Pico Expansion Plus S1
//#define DI04 21 //19

// Digital Output
#define DO01 2 
#define DO02 3 
#define DO03 6
#define DO04 7
#define DO05 17
#define DO06 18

// Analog Output
#define AO01 13 
#define AO02 14
#define AO03 16 

#define AI01 26 // this is K2 on Pico Expansion Plus S1 (K1 is RUN)
#define AI01_ADC 0
#define AI02 27
#define AI02_ADC 1
#define AI03 28
#define AI03_ADC 3
#define AI04 29 // system pin for measuring VSYS
#define AI04_ADC 4

// Events
#define GPIO_EVENT 1

#define MAX_BUFF 256
static char buff[MAX_BUFF]; // general purpose buffer 
static char messageBuff[MAX_BUFF]; // general purpose buffer 

static char* print_time(){
    datetime_t t = {0, 0, 0, 0, 0, 0, 0};
    rtc_get_datetime(&t);
    t.hour += ZONE;
    if(t.hour < 0){
        t.hour += 24;
        t.day -= 1;
        t.dotw -= 1;
    }
    snprintf((char*)&buff, sizeof buff, "%04d-%02d-%02d %02d:%02d:%02d:", t.year, t.month,t.day, t.hour, t.min, t.sec); 
    return buff;
}

#define print_log(f_, ...) printf("%s ", print_time()), printf((f_), ##__VA_ARGS__)/*, printf("\n")*/
#if NDEBUG
#define TRACE_PRINTF(fmt, args...) // Disable tracing
#else
#define TRACE_PRINTF print_log // Trace with printf
#endif

#endif 