/**
 * 
 * Create main task and run loop
 * Initializes RTC from NTP
 * Initializes data logger
 * Creates checkup interval timer
 * 
*/

/**
 * Pico includes
*/
#include <pico/stdlib.h>
#include "pico/cyw43_arch.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"

#include "lwip/ip4_addr.h"
#include "lwip/apps/httpd.h"

/**
 * FreeRTOS includes
*/
#include "FreeRTOS.h"
#include "task.h"
#include <timers.h>

/**
 * Other library includes
*/
#include "ff_headers.h"
#include "tiny-json.h"

/**
 * 
 * idb includes
 * 
*/
#include "idb_config.h"
#include "idb_ntp_client.h"
#include "idb_data_logger.h"
#include "idb_ntp_client.h"
#include "idb_runtime_stats.h"
#include "idb_http_client.h"
#include "idb_ping.h"
#include "idb_router.h"

/**
 * Link status codes
*/
#define CYW43_LINK_DOWN         (0)     ///< link is down
#define CYW43_LINK_JOIN         (1)     ///< Connected to wifi
#define CYW43_LINK_NOIP         (2)     ///< Connected to wifi, but no IP address
#define CYW43_LINK_UP           (3)     ///< Connected to wifi with an IP address
#define CYW43_LINK_FAIL         (-1)    ///< Connection failed
#define CYW43_LINK_NONET        (-2)    ///< No matching SSID found (could be out of range, or down)
#define CYW43_LINK_BADAUTH      (-3)    ///< Authenticatation failure

/**
 * Task configuration
*/
#define MAIN_TASK_PRIORITY 5
#define CHECKUP IDB_CHECKUP
#define CHECKUP_INTERVAL_MS IDB_CHECKUP_INTERVAL * 1000 // once a minute ish
#define PING_ADDR IDB_PING_ADDR
#define PING_ON IDB_PING_ON
#define PING_COUNT IDB_PING_COUNT
#define STATS IDB_STATS
#define ROUTE_TEST IDB_ROUTE_TEST
#define WIFI_SCAN IDB_WIFI_SCAN
#define WIFI_SCAN_COUNT IDB_WIFI_SCAN_COUNT
#define WORLD_TIME_API IDB_WORLD_TIME_API

/**
 * Used to get local time zone
*/
#define HOST "worldtimeapi.org"
#define URL_REQUEST "/api/ip"

/**
 * Globals
*/
bool runCheck = false;
ip_addr_t ping_addr;
volatile TimerHandle_t checkup_timer;
static TaskHandle_t th;
struct netif netif;

/**
 * 
 * Callback to set the RTC using the result from NPT
 * 
*/
void setRTC(NTP_T* npt_t, int status, time_t *result){
    printf("NTP callback received\n");
    if (status == 0 && result) {
        struct tm *utc = gmtime(result);
         printf("NTP response: %02d/%02d/%04d %02d:%02d:%02d\n", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
                utc->tm_hour, utc->tm_min, utc->tm_sec);

        // Start on Friday 5th of June 2020 15:45:00
        datetime_t t = {
                .year  = utc->tm_year + 1900,
                .month = utc->tm_mon + 1,
                .day   = utc->tm_mday,
                .dotw  = utc->tm_wday, // 0 is Sunday, so 5 is Friday
                .hour  = utc->tm_hour,
                .min   = utc->tm_min,
                .sec   = utc->tm_sec
        };

        // Start the RTC
        rtc_init();
        rtc_set_datetime(&t);

        // clk_sys is >2000x faster than clk_rtc, so datetime is not updated immediately when rtc_get_datetime() is called.
        // tbe delay is up to 3 RTC clock cycles (which is 64us with the default clock settings)
        sleep_us(64);
    }
}

/**
 * 
 * Helper to print date from the RTC
 * 
*/
void print_date(){
    datetime_t t;
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    rtc_get_datetime(&t);
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    TRACE_PRINTF("\r%s      ", datetime_str);
}

/**
 * 
 * Helper to extract local time zone from a json buffer
 * 
*/
void getLocalTime(char* buffer){
    //TRACE_PRINTF(buffer);
    enum { MAX_FIELDS = 16 };
    json_t pool[ MAX_FIELDS ];
    //TRACE_PRINTF("Before first json\n");
    json_t const* parent = json_create( buffer, pool, MAX_FIELDS );
    if ( parent == NULL ) 
        TRACE_PRINTF ("EXIT_FAILURE\n");
    //TRACE_PRINTF("Before second json\n");
    json_t const* abbrevProperty = json_getProperty( parent, "abbreviation" );
    if ( abbrevProperty == NULL ) 
        TRACE_PRINTF("json failure to get local time zone name\n");
    json_t const* utc_offset_property = json_getProperty( parent, "utc_offset" );    
    if ( utc_offset_property == NULL ) 
        TRACE_PRINTF("json failure to get local time zone UTC offset\n");
    //TRACE_PRINTF("Before last json\n");    
    char const* abbreviation = json_getValue( abbrevProperty );
    char const* utc_offset = json_getValue( utc_offset_property );
    TRACE_PRINTF("Local Time Zone: %s, UTC offset: %s\n", abbreviation, utc_offset);
    
}


static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
    if (result) {
        TRACE_PRINTF("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n",
            result->ssid, result->rssi, result->channel,
            result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5],
            result->auth_mode);
    }
    return 0;
}

void wifi_scan(){
    absolute_time_t scan_time = nil_time;
    bool scan_in_progress = false;
    for (int i = 0; i < WIFI_SCAN_COUNT; i++) {
        if (absolute_time_diff_us(get_absolute_time(), scan_time) < 0) {
            if (!scan_in_progress) {
                cyw43_wifi_scan_options_t scan_options = {0};
                int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result);
                if (err == 0) {
                    TRACE_PRINTF("Performing wifi scan\n");
                    scan_in_progress = true;
                } else {
                    TRACE_PRINTF("Failed to start scan: %d\n", err);
                    scan_time = make_timeout_time_ms(3000); // wait 3s and try again
                }
            } else if (!cyw43_wifi_scan_active(&cyw43_state)) {
                scan_time = make_timeout_time_ms(10000); // wait 10s and scan again
                scan_in_progress = false; 
            }
        }
        // the following #ifdef is only here so this same example can be used in multiple modes;
        // you do not need it in your code
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        cyw43_arch_wait_for_work_until(scan_time);
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
       sleep_ms(1000);
#endif
    }
}

/**
 * 
 * Make the Wi-Fi connection in station mode
 * Requires WIFI_SSID and WIFI_PASSWORD to be set in the environment
 * 
*/
void start_wifi(){

        if (cyw43_arch_init()) {
        TRACE_PRINTF("Failed to initialise Wi-Fi\n");
        return;
    }

    TRACE_PRINTF("Connecting to WiFi SSID: %s \n", WIFI_SSID);

    cyw43_arch_enable_sta_mode();
    uint32_t pm;
    cyw43_wifi_get_pm(&cyw43_state, &pm);
    cyw43_wifi_pm(&cyw43_state, CYW43_DEFAULT_PM & ~0xf); // disable power management
    TRACE_PRINTF("Connecting to WiFi...\n");

    for(int i = 0;i<4;i++){
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK, 30000) == 0) {
            TRACE_PRINTF("\nReady, preparing to run httpd at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
            break;
        }
        TRACE_PRINTF("Wi-Fi failed to start, retrying\n");
    }
    //cyw43_wifi_pm(&cyw43_state, pm); // reset power management
}

/**
 * 
 * Can be used to validate Wi-FI connection to the router
 * 
*/
void do_ping(){
    
    ipaddr_aton(PING_ADDR, &ping_addr);
    ping_init(&ping_addr, PING_COUNT);
}

/**
 * 
 * Test of a route without using HTTTP
 * 
*/
void testRoute(){
    
    
    char buffer[] = "/readlog/2023-10-17";
    NameFunction* fun_ptr = isRoute(buffer, HTTP_GET);
    if (fun_ptr){
        char buf[1024]; 
        route(fun_ptr, buf, sizeof buf, buffer);
    }
}

#if CHECKUP
void checkup(){
    runCheck = true;
}

void doCheckup(){
    
        TRACE_PRINTF("Routine checkup\n");
        int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        TRACE_PRINTF("Link status: %d\n", status);
    #if PING_ON
        do_ping();
    #endif
    #if STATS
        runTimeStats();
    #endif
    #if ROUTE_TEST
        //testRoute();
    #endif  
    #if WIFI_SCAN
        wifi_scan();
    #endif      
        runCheck = false;
        //cyw43_arch_deinit();
        //start_wifi();  
}

void create_checkup_timer() {
    // Create a repeating (parameter 3) timer
    checkup_timer = xTimerCreate("CHECKUP TIMER",
                                pdMS_TO_TICKS(CHECKUP_INTERVAL_MS),
                                pdTRUE,
                                (void*) 0,
                                checkup
                                );
    
    // Start the repeating timer
    if (checkup_timer != NULL){
        xTimerStart(checkup_timer, 0);
    } 
}
#endif //CHECKUP

/**
 * 
 * Main task starts Wi-Fi, creates checkup timer, sets the RTC from NPT, initializes the HTTP server
 * and lists the running tasks stats, then enters task loop. If checkup required, perform it in the loop
 * 
*/
void main_task(__unused void *params) {
    
    start_wifi();
#if CHECKUP
    create_checkup_timer();
#endif
    getNTPtime(&setRTC); // get UTC time
#if WORLD_TIME_API
// this requires the dev branch of PICO-SDK and there is an example in PICO-EXAMPLES branch 'origin/add_http_example'
    getHTTPClientResponse(HOST, URL_REQUEST, &getLocalTime); // get local time - this requires Peter Harper's PICO-SDK
#endif
    httpd_init();

    bool rc = logger_init();
    TRACE_PRINTF("Initializing data logger: %s\n", rc ? "true" : "false");

    runTimeStats();

    while(true) {
#if CHECKUP        
        if(runCheck)
            doCheckup();
#endif            
        vTaskDelay(100); // allow other tasks in
    }
    print_date();

    cyw43_arch_deinit();
}

/**
 * 
 * Create main task and start the scheduler
 * 
*/
void vLaunch( void) {
    TaskHandle_t task;
    xTaskCreate(main_task, "MainThread", 8192, NULL, MAIN_TASK_PRIORITY, &task);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUM_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

// These hooks are defiend as part of the PICO-SDK implementation
//#ifndef NDEBUG
// Note: All pvPortMallocs should be checked individually,
// but we don't expect any to fail,
// so this can help flag problems in Debug builds.
void vApplicationMallocFailedHook(void) {
    TRACE_PRINTF("\nMalloc failed! Task: %s\n", pcTaskGetName(NULL));
    //__disable_irq(); /* Disable global interrupts. */
    vTaskSuspendAll();
    //__BKPT(5);
}
//#endif

// void vApplicationStackOverflowHook( TaskHandle_t xTask,
//                                     char *pcTaskName ){
//     TRACE_PRINTF("Task stack overflow: %s\n", pcTaskName);
//     vTaskSuspendAll();  
//     //__BKPT(5);                                 
// }

/**
 * 
 * Initialize IO and launch FreeRTOS
 * 
*/
int main( void )
{
    stdio_init_all();
    sleep_ms (2000); // wait to access to USB serial
    /* Configure the hardware ready to run the demo. */
    const char *rtos_name;
#if ( portSUPPORT_SMP == 1 )
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif

#if ( portSUPPORT_SMP == 1 ) && ( configNUM_CORES == 2 )
    TRACE_PRINTF("Starting %s on both cores:\n", rtos_name);

    vLaunch();
#elif ( RUN_FREE_RTOS_ON_CORE == 1 )
    TRACE_PRINTF("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else
    TRACE_PRINTF("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif
    return 0;
}