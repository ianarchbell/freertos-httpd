#include <pico/cyw43_arch.h>

#include <lwip/ip4_addr.h>


/**
 * FreeRTOS includes
*/
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

#include "idb_config.h"
#include "idb_network.h"

#define PING_ADDR IDB_PING_ADDR
#define PING_ON IDB_PING_ON
#define PING_COUNT IDB_PING_COUNT

#define WIFI_SCAN IDB_WIFI_SCAN
#define WIFI_SCAN_COUNT IDB_WIFI_SCAN_COUNT

ip_addr_t ping_addr;

/**
 * 
 * Make the Wi-Fi connection in station mode
 * Requires WIFI_SSID and WIFI_PASSWORD to be set in the environment
 * 
*/
void start_wifi(){

    if (cyw43_arch_init()) {
        printf("Failed to initialise Wi-Fi\n");
        return;
    }

    printf("\n\nConnecting to WiFi SSID: %s \n", WIFI_SSID);

    cyw43_arch_enable_sta_mode();
    uint32_t pm;
    cyw43_wifi_get_pm(&cyw43_state, &pm);
    cyw43_wifi_pm(&cyw43_state, CYW43_DEFAULT_PM & ~0xf); // disable power management
    printf("Connecting to WiFi...\n");

    for(int i = 0;i<4;i++){
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK, 30000) == 0) {
            printf("\nReady, preparing to run httpd at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
            break;
        }
        printf("Wi-Fi failed to start, retrying\n");
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


int getLinkStatus(){
    TRACE_PRINTF("Routine checkup\n");
    int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    return status;
} 

void wifi_deinit(){
    cyw43_arch_deinit();
}

