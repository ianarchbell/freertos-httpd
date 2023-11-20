#include <FreeRTOS.h>
#include <task.h>

#include <httpd.h>

#include "idb_websocket.h"
#include "idb_http.h"
#include "idb_network.h"

/**
 * 
 * HTTP task that registers websocket callbacks and initializes the HTTP Daemon
 */
void httpd_task(void *pvParameters)
{
    websocket_init();
    httpd_init();
    printf("Web server initialized\n");
    for (;;){
        vTaskDelay(100); // allow other tasks in
    };
}

/**
 * 
 * Create the HTTP task
 * 
 */
void http_init(){
    xTaskCreate(&httpd_task, "HTTP Daemon", 4000, NULL, 3, NULL);
}

/* TO DO this should be called if http_task ends - not normal behavior */
void http_deinit(){
    wifi_deinit();
}