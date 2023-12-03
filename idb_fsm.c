#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <ff_utils.h>
#include <ff_headers.h>
#include <ff_stdio.h>

#include <tiny-json.h>

#include "idb_fsm.h"
#include "idb_config.h"
#include "idb_hardware.h"

/* Queue used to send and receive complete struct AMessage structures. */
QueueHandle_t xEventQueue = NULL;

#define DEVICENAME "sd0"
#define MOUNTPOINT "/sd0"

FF_Disk_t *pxDisk2 = NULL;

/**
 * 
 * HTTP task that registers websocket callbacks and initializes the HTTP Daemon
 */
void fsm_task(void *pvParameters)
{   
   
   EventMessage eventMsg;

    for(;;){
        if( xEventQueue != NULL )
        {
            /* Receive a message from the created queue to hold complex struct AMessage
            structure.  Block for 10 ticks if a message is not immediately available.
            The value is read into a struct AMessage variable, so after calling
            xQueueReceive() xRxedStructure will hold a copy of xMessage. */
            if( xQueueReceive( xEventQueue,
                                &( eventMsg ),
                                ( TickType_t ) 10 ) == pdPASS )
            {
                // does current tsate have a transition for this event
                // if so get that state and make current
                // dispatch that state
            }
        }        
    }
    fsm_deinit();
}

void vCreateEventQueue( void )
{
   /* Create the queue used to send complete struct AMessage structures.  This can
   also be created after the schedule starts, but care must be task to ensure
   nothing uses the queue until after it has been created. */
   xEventQueue = xQueueCreate(
                         /* The number of items the queue can hold. */
                         10,
                         /* Size of each item is big enough to hold the
                         whole structure. */
                         sizeof( EventMessage ) );

   if(  xEventQueue == NULL )
   {
      /* One or more queues were not created successfully as there was not enough
      heap memory available.  Handle the error here.  Queues can also be created
      statically. */
      panic("Failed to create FSM queue\n");
   }
}

/* This should be called if FSM_task ends - not normal behavior */
void fsm_deinit(){
   
   unmount(pxDisk2, MOUNTPOINT);

}


void readFSM(char* fsmFile){
    TRACE_PRINTF("Reading fsm\n");
    char fileBuff[12500];
    // Opening file in reading mode
    FF_FILE* ptr = ff_fopen(fsmFile, "r");

    ff_fseek(ptr, 0, SEEK_END);
    long fsize = ff_ftell(ptr);
    ff_fseek(ptr, 0, SEEK_SET);  /* same as rewind(f); */
    TRACE_PRINTF("fsm model size: %d\n", fsize);

    ff_fread(fileBuff, fsize, 1, ptr);
    ff_fclose(ptr);

    enum { MAX_FIELDS = 300 };
    json_t pool[ MAX_FIELDS ];

    json_t const* json = json_create( fileBuff, pool, MAX_FIELDS );
    if ( json != NULL ){
        TRACE_PRINTF("json parsed\n");
        json_t const* fsmProperty = json_getProperty( json, "fsm" );

        // states
        json_t const* statesProperty = json_getProperty(fsmProperty , "states" );
        if ( !statesProperty || JSON_ARRAY != json_getType( statesProperty ) )             
            TRACE_PRINTF("json failure to get states property\n");

        json_t const* stateProperty;
        for( stateProperty = json_getChild( statesProperty ); stateProperty != 0; stateProperty = json_getSibling( stateProperty ) ) {
            const char* state = json_getValue( stateProperty );
            if(state)
                TRACE_PRINTF("FSM State: %s.\n", state );
        }
        // initial state
        json_t const* initialProperty = json_getProperty( fsmProperty, "initial" );
        if (initialProperty){
            const char* state = json_getValue( initialProperty );
            if(state)
                TRACE_PRINTF("Initial FSM State: %s.\n", state );
        }
        // initial state
        json_t const* nameProperty = json_getProperty( fsmProperty, "name" );
        if (nameProperty){
            const char* name = json_getValue( nameProperty );
            if(name)
                TRACE_PRINTF("Initial FSM name: %s.\n", name );
        }
        // device map
        json_t const* deviceMapProperty = json_getProperty( fsmProperty, "device-map" );

        json_t const* deviceProperty;
        for( deviceProperty = json_getChild( deviceMapProperty ); deviceProperty != 0; deviceProperty = json_getSibling( deviceProperty ) ) {
            const char* device = json_getName( deviceProperty );
            const char* mappedTo = json_getValue( deviceProperty );
            if(device)
                TRACE_PRINTF("Device map: %s to %s\n", device, mappedTo );
        }
        // variables
        json_t const* variablesProperty = json_getProperty( fsmProperty, "variables" );

        json_t const* variableProperty;
        for( variableProperty = json_getChild( variablesProperty ); variableProperty != 0; variableProperty = json_getSibling( variableProperty ) ) {
            const char* variable = json_getName( variableProperty );
            const char* value = json_getValue( variableProperty );
            if(variable)
                TRACE_PRINTF("Variable: %s value %s\n", variable, value );
        }

        // timers
        json_t const* timersProperty = json_getProperty( fsmProperty, "timers" );

        json_t const* timerProperty;
        for( timerProperty = json_getChild( timersProperty ); timerProperty != 0; timerProperty = json_getSibling( timerProperty ) ) {
            const char* timer = json_getName( timerProperty );
            const char* value = json_getValue( timerProperty );
            if(timer)
                TRACE_PRINTF("Timer: %s value %s\n", timer, value );
        }

    } 
}

/**
 * 
 * Create the fsm task
 * 
 */
void fsm_init(){

    if (!mount(&pxDisk2, DEVICENAME, MOUNTPOINT)) 
        panic("Failed to mount disk for fsm file\n");

    // create state queue
    vCreateEventQueue();
    // read fsm model
    readFSM("/sd0/fsm.json");
    // create the state task
    xTaskCreate(&fsm_task, "FSM task", 1024, NULL, 3, NULL);
}
