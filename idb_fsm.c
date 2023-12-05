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

char* fsm_file = "/sd0/fsm.json";

FF_Disk_t *pxDisk2 = NULL;

char* model = "brewery";

char* currentState;
json_t* currentStateProperty;

// initialized in fsmRead()
json_t const* json;
json_t const* fsmProperty;
json_t const* statesProperty;
json_t const* initialStateProperty;
json_t const* nameProperty;
json_t const* deviceMapProperty;
json_t const* variablesProperty;
json_t const* timersProperty;


/**
 * 
 * Posts an event message to the queue
 * 
*/
BaseType_t sendMessageFromISR(EventMessage eventMsg){
    /* Send the entire structure to the queue created to hold 10 structures. */
   if(xEventQueue != NULL){

        xQueueSendToBackFromISR( /* The handle of the queue. */
                xEventQueue,
                /* The address of the xMessage variable.  sizeof( struct AMessage )
                bytes are copied from here into the queue. */
                ( void * ) &eventMsg,
                /* Block time of 0 says don't block if the queue is already full.
                Check the value returned by xQueueSend() to know if the message
                was sent to the queue successfully. */
                ( TickType_t ) 0 );
   }
}

/**
 * 
 * Set the initial model
 * 
 */
void setModel(char* model){

}

/**
 * 
 * 
 * Execute current state
 * 
 * 
*/
void executeCurrentState(){
    TRACE_PRINTF("Executing current state: %s\n", currentState);
}

/**
 * 
 * Set the initial state
 * 
 */
void setInitialState(){
    const char* initialState = json_getValue( initialStateProperty );
    if(initialState){
        json_t const* stateProperty;
        for( stateProperty = json_getChild( fsmProperty ); stateProperty != 0; stateProperty = json_getSibling( stateProperty ) ) {
            const char* state = json_getValue( stateProperty );
            if(!strcmp(state, initialState)){
                TRACE_PRINTF("FSM found initial stateProperty: %s.\n", state );
                currentState = (char*) state;
                currentStateProperty = (json_t*) stateProperty;
                executeCurrentState();
                break;
            }
        }
    }
}

const char*  getMappedName(const char* eventDescriptor){

    json_t const* deviceProperty;
    for( deviceProperty = json_getChild( deviceMapProperty ); deviceProperty != 0; deviceProperty = json_getSibling( deviceProperty ) ) {
        const char* device = json_getName( deviceProperty );
        const char* mappedTo = json_getValue( deviceProperty );
        if(!strcmp(eventDescriptor, mappedTo)){
            TRACE_PRINTF("Device mapped: %s to %s\n", device, mappedTo );
            return device;
        }    
    }
    return NULL;
}

/**
 * 
 * Determine transition from event and state
 * 
*/
json_t const* getTransitionState(EventMessage event){
 
    json_t const* transitionsProperty = json_getProperty( currentStateProperty, "transitions" );     
    if ( !transitionsProperty || JSON_ARRAY != json_getType( transitionsProperty ) ) {           
        TRACE_PRINTF("json failure to get transitions property\n");
        return NULL;
    }
    // iterate transitions
    json_t const* transitionProperty;
    for( transitionProperty = json_getChild( transitionsProperty ); transitionProperty != 0; transitionProperty = json_getSibling( transitionProperty ) ) {
        json_t const* eventProperty = json_getProperty( transitionProperty, "event" );
        json_t const* eventTypeProperty = json_getProperty(eventProperty, "type");
        json_t const* eventNameProperty = json_getProperty(eventProperty, "name");
        json_t const* eventValueProperty = json_getProperty(eventProperty, "value");
        const char * name = json_getValue(eventNameProperty);
        const char * type = json_getValue(eventTypeProperty);
        const char * valueString = json_getValue(eventValueProperty);

        const char* mappedName = getMappedName(event.descriptor);

        if(mappedName && type && valueString){
             if (!strcmp(mappedName, name)){
                if (!strcmp(type, "float")){
                    float fValue = atof(valueString);
                    if (fValue == event.val.float_value){
                        TRACE_PRINTF("Matched transition event name: %s, type: %s, value %s against event %s, value %.3f\n", name, type, valueString, event.descriptor, event.val.float_value ); 
                        json_t const* stateProperty = json_getProperty(transitionProperty, "state"); 
                        return stateProperty;                     
                    }
                }
                else{
                    int iValue = atoi(valueString);
                    if (iValue == event.val.int_value){
                        TRACE_PRINTF("Matched transition event name: %s, type: %s, value %s against event %s, value %u\n", name, type, valueString, event.descriptor, event.val.int_value ); 
                        json_t const* stateProperty = json_getProperty(transitionProperty, "state");  
                        return stateProperty;                                                                 
                    }
                }
            }
        }
    }
    return NULL;
}

/**
 * 
 * FSM Engine
 * 
 * 1. Set the current state - initial if no state
 * 2. Execute the state settings
 * 3. Wait for events
 * 4. Check the event against available transitions
 * 5. if available transition, take it
 * 6. Loop
 * 
 */
void fsm_task(void *pvParameters)
{   
   
   EventMessage eventMsg;

    for(;;){
    // wait for events on the event queue
    // determine transition
    // execute state    

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
                /* eventMsg now contains a copy of eventsMessage. */
                static uint32_t eventId = 0;
                eventId++;

                json_t const* transitionStateProperty = getTransitionState(eventMsg);
                if (transitionStateProperty != NULL){
                    const char* transitionState = json_getValue(transitionStateProperty);
                    TRACE_PRINTF("Processing transition event event for: %s, transitioning from state: %s to state: %s\n", eventMsg.descriptor, currentState, transitionState);
                    currentState = (char*)transitionState;
                    currentStateProperty = (json_t*)transitionStateProperty;
                    executeCurrentState();
                } 
                else{
                    TRACE_PRINTF("No transition for this event: %s for state: %s\n", eventMsg.descriptor, currentState);
                }                  
            }
        } 
        vTaskDelay(100);   
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

    json = json_create( fileBuff, pool, MAX_FIELDS );
    if ( json != NULL ){
        TRACE_PRINTF("json parsed\n");
        fsmProperty = json_getProperty( json, "fsm" );

        // states
        statesProperty = json_getProperty(fsmProperty , "states" );
        if ( !statesProperty || JSON_ARRAY != json_getType( statesProperty ) )             
            TRACE_PRINTF("json failure to get states property\n");

        json_t const* stateProperty;
        for( stateProperty = json_getChild( statesProperty ); stateProperty != 0; stateProperty = json_getSibling( stateProperty ) ) {
            const char* state = json_getValue( stateProperty );
            if(state)
                TRACE_PRINTF("FSM State: %s.\n", state );
        }
        // initial state
        initialStateProperty = json_getProperty( fsmProperty, "initial" );
        if (initialStateProperty){
            const char* state = json_getValue( initialStateProperty );
            if(state)
                TRACE_PRINTF("Initial FSM State: %s.\n", state );
        }
        // name
        nameProperty = json_getProperty( fsmProperty, "name" );
        if (nameProperty){
            const char* name = json_getValue( nameProperty );
            if(name)
                TRACE_PRINTF("Initial FSM name: %s.\n", name );
        }
        // device map
        deviceMapProperty = json_getProperty( fsmProperty, "device-map" );

        json_t const* deviceProperty;
        for( deviceProperty = json_getChild( deviceMapProperty ); deviceProperty != 0; deviceProperty = json_getSibling( deviceProperty ) ) {
            const char* device = json_getName( deviceProperty );
            const char* mappedTo = json_getValue( deviceProperty );
            if(device)
                TRACE_PRINTF("Device map: %s to %s\n", device, mappedTo );
        }
        // variables
        variablesProperty = json_getProperty( fsmProperty, "variables" );

        json_t const* variableProperty;
        for( variableProperty = json_getChild( variablesProperty ); variableProperty != 0; variableProperty = json_getSibling( variableProperty ) ) {
            const char* variable = json_getName( variableProperty );
            const char* value = json_getValue( variableProperty );
            if(variable)
                TRACE_PRINTF("Variable: %s value %s\n", variable, value );
        }

        // timers
        timersProperty = json_getProperty( fsmProperty, "timers" );

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
    readFSM(fsm_file);   
    // set the initial state
    setInitialState();
    // create the state task
    xTaskCreate(&fsm_task, "FSM task", 1024, NULL, 3, NULL);
}
