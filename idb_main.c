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
#include <pico/util/datetime.h>
#include <hardware/rtc.h>

/**
 * FreeRTOS includes
*/
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

/**
 * Other library includes
*/

#include <tiny-json.h>

/**
 * 
 * idb includes
 * 
*/
#include "idb_config.h"
#include "idb_ntp_client.h"
#include "idb_data_logger.h"
#include "idb_runtime_stats.h"
#include "idb_http_client.h"
#include "idb_ping.h"
#include "idb_router.h"
//#include "idb_ini.h"
#include "idb_network.h"
#include "idb_http.h"
#include "idb_websocket.h"
//#include "idb_test.h"
#include "idb_hardware.h"
#include "idb_state.h"

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
#define MAIN_TASK_PRIORITY      5
#define MAIN_TASK_STACK_SIZE 8096
#define CHECKUP IDB_CHECKUP
#define CHECKUP_INTERVAL_MS IDB_CHECKUP_INTERVAL * 1000 // once a minute ish
#define STATS IDB_STATS
#define ROUTE_TEST IDB_ROUTE_TEST
#define WORLD_TIME_API IDB_WORLD_TIME_API
#define INI_TEST IDB_INI_TEST
#define DATA_LOGGING IDB_DATA_LOGGING

/**
 * Used to get local time zone
*/
#define HOST "worldtimeapi.org"
#define URL_REQUEST "/api/ip"

/**
 * Globals
*/
bool runCheck = false;
volatile TimerHandle_t checkup_timer;
static TaskHandle_t th;

//extern void testRoutes();
//extern void testaRoute();

/**
 * 
 * Callback to set the RTC using the result from NPT
 * 
*/
void setRTC(NTP_T* npt_t, int status, time_t *result){
    TRACE_PRINTF("NTP callback received\n");
    if (status == 0 && result) {
        struct tm *utc = gmtime(result);
        TRACE_PRINTF("NTP response: %02d/%02d/%04d %02d:%02d:%02d\n", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
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

#if CHECKUP
void checkup(){
    runCheck = true;
}

void doCheckup(){


    int status = getLinkStatus();

    printf("Link status: %d", status);
    if (status == 3)
        printf(" (link up)\n");
    else
        printf(" (link not up)\n");    

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

#if INI_TEST    
    testIni();
 #endif   
    
    start_wifi();
    
    getNTPtime(&setRTC); // get UTC time

#if WORLD_TIME_API
// this requires the dev branch of PICO-SDK and there is an example in PICO-EXAMPLES branch 'origin/add_http_example'
    getHTTPClientResponse(HOST, URL_REQUEST, &getLocalTime); // get local time - this requires Peter Harper's PICO-SDK
#endif

    http_init();

#if CHECKUP
    create_checkup_timer();
#endif

#if DATA_LOGGING
    bool rc = logger_init();
    TRACE_PRINTF("Initializing data logger: %s\n", rc ? "true" : "false");
#endif

    runTimeStats();

    hardware_init(); // last thing

    state_init();

    //fsm_init();

    print_date();

    while(true) {
#if CHECKUP        
        if(runCheck)
            doCheckup();
#endif            
        //vTaskDelay(100); // allow other tasks in
        vTaskDelay(1000); // allow other tasks in
    }
    print_date();

    http_deinit();

}

/**
 * 
 * Create main task and start the scheduler
 * 
*/
void vLaunch( void) {
    TaskHandle_t task;
    xTaskCreate(main_task, "MainThread", MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, &task);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUM_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

/**
 * 
 * These hooks are defined in various places and have the weak attribute here in case 
 * it is not defined as we have specified to call hook in our config
 * 
 */ 

void __attribute__((weak)) vApplicationMallocFailedHook(void) {
    TRACE_PRINTF("\nMalloc failed! Task: %s\n", pcTaskGetName(NULL));
    //__disable_irq(); /* Disable global interrupts. */
    vTaskSuspendAll();
    //__BKPT(5);
}

void __attribute__((weak)) vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    char *pcTaskName ){
    TRACE_PRINTF("\nStack overflow! Task: %s\n", pcTaskName);
    vTaskSuspendAll();  
    //__BKPT(5);                                 
}

/**
 * 
 * Launch FreeRTOS
 * 
*/
int main( void )
{
    stdio_init_all();

    //testRoutes(); 

    //testaRoute();

    //sleep_ms (2000); // only need this when not using probe
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