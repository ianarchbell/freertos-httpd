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
    printf("\r%s      ", datetime_str);
}

/**
 * 
 * Helper to extract local time zone from a json buffer
 * 
*/
void getLocalTime(char* buffer){
    //printf(buffer);
    enum { MAX_FIELDS = 16 };
    json_t pool[ MAX_FIELDS ];
    //printf("Before first json\n");
    json_t const* parent = json_create( buffer, pool, MAX_FIELDS );
    if ( parent == NULL ) 
        printf ("EXIT_FAILURE\n");
    //printf("Before second json\n");
    json_t const* abbrevProperty = json_getProperty( parent, "abbreviation" );
    if ( abbrevProperty == NULL ) 
        printf("json failure\n");
    //printf("Before last json\n");    
    char const* abbreviation = json_getValue( abbrevProperty );
    printf("Local Time Zone: %s\n", abbreviation);
    
}

/**
 * 
 * Make the Wi-Fi connection in station mode
 * Requires WIFI_SSID and WIFI_PASSWORD to be set in the environment
 * 
*/
void start_wifi(){

        if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return;
    }

    printf("Connecting to WiFi SSID: %s \n", WIFI_SSID);

    cyw43_arch_enable_sta_mode();
    printf("Connecting to WiFi...\n");

    for(int i = 0;i<4;i++){
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK, 30000) == 0) {
            printf("\nReady, preparing to run httpd at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
            break;
        }
        printf("Wi-Fi failed to start, retrying\n");
    }
}

/**
 * 
 * Can be used to validate Wi-FI connection to the router
 * 
*/
void do_ping(){
    
    ipaddr_aton(PING_ADDR, &ping_addr);
    ping_init(&ping_addr);
}

/**
 * 
 * Test of a route without using HTTTP
 * 
*/
void testRoute(){
    
    char buf[1024]; 
    char buffer[] = "/readlog/2023-10-17";
    NameFunction* fun_ptr = isRoute(buffer, HTTP_GET);
    if (fun_ptr)
        route(fun_ptr, buf, sizeof buf, buffer);

}

#if CHECKUP
void checkup(){
    runCheck = true;
}

void doCheckup(){
    
        printf("Routine checkup\n");
        //runTimeStats();
        int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        printf("Link status: %d\n", status);
        //do_ping();
        //testRoute();
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

    //getHTTPClientResponse(HOST, URL_REQUEST, &getLocalTime); // get local time - this requires Peter Harper's PICO-SDK

    httpd_init();

    bool rc = logger_init();
    printf("Initializing data logger: %s\n", rc ? "true" : "false");

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
    printf("\nMalloc failed! Task: %s\n", pcTaskGetName(NULL));
    //__disable_irq(); /* Disable global interrupts. */
    vTaskSuspendAll();
    //__BKPT(5);
}
//#endif

// void vApplicationStackOverflowHook( TaskHandle_t xTask,
//                                     char *pcTaskName ){
//     printf("Task stack overflow: %s\n", pcTaskName);
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
    printf("Starting %s on both cores:\n", rtos_name);

    vLaunch();
#elif ( RUN_FREE_RTOS_ON_CORE == 1 )
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif
    return 0;
}