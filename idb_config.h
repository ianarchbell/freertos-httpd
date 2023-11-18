#ifndef IDB_CONFIG_H
#define IDB_CONFIG_H

#include "hardware/rtc.h"

#define PDT -7
#define PST -8
#define ZONE PST

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

static char buff[64]; // general purpose buffer 

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