#include "string.h"

/**
 * FreeRTOS includes
*/
#include <FreeRTOS.h>
#include <queue.h>
#include <lwip/apps/httpd.h>

#include "idb_config.h"
#include "idb_websocket.h"

/* Queue used to send and receive complete struct AMessage structures. */
QueueHandle_t xStructQueue = NULL;

void vCreateWSQueue( void )
{

   /* Create the queue used to send complete struct AMessage structures.  This can
   also be created after the schedule starts, but care must be task to ensure
   nothing uses the queue until after it has been created. */
   xStructQueue = xQueueCreate(
                         /* The number of items the queue can hold. */
                         10,
                         /* Size of each item is big enough to hold the
                         whole structure. */
                         sizeof( wsMessage ) );

   if(  xStructQueue == NULL )
   {
      /* One or more queues were not created successfully as there was not enough
      heap memory available.  Handle the error here.  Queues can also be created
      statically. */
   }
}

/**
 * 
 * Posts a websocket message to the queue
 * 
*/
BaseType_t sendWSMessage(wsMessage wsMsg){
    /* Send the entire structure to the queue created to hold 10 structures. */
   xQueueSend( /* The handle of the queue. */
               xStructQueue,
               /* The address of the xMessage variable.  sizeof( struct AMessage )
               bytes are copied from here into the queue. */
               ( void * ) &wsMsg,
               /* Block time of 0 says don't block if the queue is already full.
               Check the value returned by xQueueSend() to know if the message
               was sent to the queue successfully. */
               ( TickType_t ) 0 );
}

/**
 * 
 * Posts a websocket message to the queue
 * 
*/
BaseType_t sendWSMessageFromISR(wsMessage wsMsg){
    /* Send the entire structure to the queue created to hold 10 structures. */
   xQueueSendToBackFromISR( /* The handle of the queue. */
               xStructQueue,
               /* The address of the xMessage variable.  sizeof( struct AMessage )
               bytes are copied from here into the queue. */
               ( void * ) &wsMsg,
               /* Block time of 0 says don't block if the queue is already full.
               Check the value returned by xQueueSend() to know if the message
               was sent to the queue successfully. */
               ( TickType_t ) 0 );
}

void websocket_task(void *pvParameter)
{
    struct tcp_pcb *pcb = (struct tcp_pcb *) pvParameter;

    wsMessage wsMsg;

    for (;;) {
        if (pcb == NULL || pcb->state != ESTABLISHED) {
            TRACE_PRINTF("Websocket connection closed, deleting task\n");
            break;
        }

        if( xStructQueue != NULL )
        {
            /* Receive a message from the created queue to hold complex struct AMessage
            structure.  Block for 10 ticks if a message is not immediately available.
            The value is read into a struct AMessage variable, so after calling
            xQueueReceive() xRxedStructure will hold a copy of xMessage. */
            if( xQueueReceive( xStructQueue,
                                &( wsMsg ),
                                ( TickType_t ) 10 ) == pdPASS )
            {
                /* wMsg now contains a copy of wsMessage. */
                static uint32_t messageId = 0;
                messageId++;

                char buff[72];

        //        int n = snprintf(buff, sizeof buff, "{ \"messageId\" : \"%d\", \"eventId\": \"%d\", \"descriptor\" : \"%d\", \"event\" : \"%d\" }", wsMsg.ulMessageID, wsMsg.ulEventId, wsMsg.ulDescriptor, wsMsg.ulEvent);
                int n = snprintf(buff, sizeof buff, "{ \"messageId\" : %d, \"eventId\": %d, \"descriptor\" : %d, \"event\" : %d }", messageId, wsMsg.ulEventId, wsMsg.ulDescriptor, wsMsg.ulEvent);
                printf("Websocket message received for ...: %d\n", wsMsg.ulDescriptor);
                websocket_write(pcb, buff, n, WS_TEXT_MODE);
                printf("Websocket write sent: %s\n", buff);
            }
        }

        //int uptime = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
        //int heap = (int) xPortGetFreeHeapSize();
        //int led = !gpio_read(LED_PIN);

        /* Generate response in JSON format */
        // char response[64];
        // int len = snprintf(response, sizeof (response),
        //         "{\"uptime\" : \"%d\","
        //         " \"heap\" : \"%d\","
        //         " \"led\" : \"%d\"}", uptime, heap, "led_value");
        // if (len < sizeof (response))
        //     websocket_write(pcb, (unsigned char *) response, len, WS_TEXT_MODE);
        // Five seconds here seems to be fine but two is not...seems to overwhelm lwip    
        //vTaskDelay(20000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

/**
 * 
 * This function is called when websocket frame is received.
 * Note: this function is executed on TCP thread and should return as soon as possible.
 * 
 */
void websocket_cb(struct tcp_pcb *pcb, uint8_t *data, u16_t data_len, uint8_t mode)
{
    TRACE_PRINTF("[Websocket callback]:\n%.*s\n", (int) data_len, (char*) data);

    uint8_t response[2];
    uint16_t val;

    switch (data[0]) {
        case 'A': // ADC
            /* This should be done on a separate thread in 'real' applications */
            val = 0xFFFF;
            break;
        case 'D': // Disable LED
            //gpio_write(LED_PIN, true);
            val = 0xDEAD;
            break;
        case 'E': // Enable LED
            //gpio_write(LED_PIN, false);
            val = 0xBEEF;
            break;
        default:
            printf("Unknown command\n");
            val = 0;
            break;
    }

    response[1] = (uint8_t) val;
    response[0] = val >> 8;

    websocket_write(pcb, response, 2, WS_BIN_MODE);
}

/**
 * This function is called when new websocket is open and
 * creates a new websocket_task if requested URI equals '/stream'.
 */
void websocket_open_cb(struct tcp_pcb *pcb, const char *uri)
{
    TRACE_PRINTF("WS URI: %s\n", uri);
    if (!strcmp(uri, "/stream")) {
        TRACE_PRINTF("Websocket request for streaming\n");
        xTaskCreate(&websocket_task, "websocket_task", 256, (void *) pcb, 5, NULL);
    }
}

void websocket_init(){

    // create websocket message queue
    vCreateWSQueue();

    // register websockets callbacks before HTTPD is initialized
    websocket_register_callbacks((tWsOpenHandler) websocket_open_cb, (tWsHandler) websocket_cb);       

}