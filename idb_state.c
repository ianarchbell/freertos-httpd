#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <ff_utils.h>

#include "minIni.h"

#include "idb_state.h"
#include "idb_config.h"
#include "idb_hardware.h"

/* Queue used to send and receive complete struct AMessage structures. */
QueueHandle_t xStateQueue = NULL;

#define DEVICENAME "sd0"
#define MOUNTPOINT "/sd0"

FF_Disk_t *pxDisk = NULL;

const char inifile[] = "/sd0/idb.ini";

stateItem states[15] = {

{"AO01\0", STATE_ANALOG_OUTPUT, AO01, 0}, 
{"AO02\0", STATE_ANALOG_OUTPUT, AO02, 1},
{"AO03\0", STATE_ANALOG_OUTPUT, AO03, 2},

// used in configuration but state not saved - always real time
{"AI01\0", STATE_ANALOG_INPUT, AI01, -1},
{"AI02\0", STATE_ANALOG_INPUT, AI02, -1},
{"AI03\0", STATE_ANALOG_INPUT, AI03, -1},

{"DO01\0", STATE_DIGITAL_OUTPUT, DO01, -1},
{"DO02\0", STATE_DIGITAL_OUTPUT, DO02, -1},
{"DO03\0", STATE_DIGITAL_OUTPUT, DO03, -1},
{"DO04\0", STATE_DIGITAL_OUTPUT, DO04, -1},
{"DO05\0", STATE_DIGITAL_OUTPUT, DO05, -1},
{"DO06\0", STATE_DIGITAL_OUTPUT, DO06, -1},

// used in configuration but state not saved - always real time
{"DI01\0", STATE_DIGITAL_INPUT, DI01, -1},
{"DI02\0", STATE_DIGITAL_INPUT, DI02, -1},
{"DI03\0", STATE_DIGITAL_INPUT, DI03, -1}

};

stateItem* getStates(){
    return (stateItem*) &states;
}

int getStatesCount(){
    return sizeof states/sizeof states[0];
}

stateItem* getStateItem(char* descriptor){
    for(int i=0 ; i < sizeof states; i++){
        if (!strcmp(descriptor, states[i].descriptor)){
            return &states[i];
        }
    }
    return NULL;
}

float getStateItemFloat(char* descriptor){
    stateItem* state = getStateItem(descriptor);
    return state->state_value.float_value;
}

int getStateItemInt(char* descriptor){
    stateItem* state = getStateItem(descriptor);
    if(state)
        return state->state_value.int_value;
    else
        return -1;    
}

int setStateItemFloat(char* descriptor, float value){
    stateItem* state = getStateItem(descriptor);
    if (state)
        state->state_value.float_value = value;
    else
        return -1;
}

int setStateItemInt(char* descriptor, float value){
    stateItem* state = getStateItem(descriptor);
    if (state)
        state->state_value.int_value = value;
    else
        return -1;
}

void vCreateStateQueue( void )
{
   /* Create the queue used to send complete struct AMessage structures.  This can
   also be created after the schedule starts, but care must be task to ensure
   nothing uses the queue until after it has been created. */
   xStateQueue = xQueueCreate(
                         /* The number of items the queue can hold. */
                         10,
                         /* Size of each item is big enough to hold the
                         whole structure. */
                         sizeof( stateMessage ) );

   if(  xStateQueue == NULL )
   {
      /* One or more queues were not created successfully as there was not enough
      heap memory available.  Handle the error here.  Queues can also be created
      statically. */
      panic("Failed to create state queue\n");
   }
}

/* This should be called if state_task ends - not normal behavior */
void state_deinit(){
   
   unmount(pxDisk, MOUNTPOINT);

}

/**
 * 
 * HTTP task that registers websocket callbacks and initializes the HTTP Daemon
 */
void state_task(void *pvParameters)
{   

    initialize_state();
    reflect_state();
    print_state();
   
   stateMessage stateMsg;

    for(;;){
        if( xStateQueue != NULL )
        {
            /* Receive a message from the created queue to hold complex struct AMessage
            structure.  Block for 10 ticks if a message is not immediately available.
            The value is read into a struct AMessage variable, so after calling
            xQueueReceive() xRxedStructure will hold a copy of xMessage. */
            if( xQueueReceive( xStateQueue,
                                &( stateMsg ),
                                ( TickType_t ) 10 ) == pdPASS )
            {
                printf("Receiving message for: %.4s\n", stateMsg.descriptor);
                if (stateMsg.ulMessageType == STATE_ANALOG_OUTPUT){
                    printf("Setting state for %s, value %.8f\n",stateMsg.descriptor, stateMsg.val.float_value);
                    getStateItem(stateMsg.descriptor)->state_value.float_value = stateMsg.val.float_value;
                    save_state_analog(stateMsg.descriptor);
                }
                else if  (stateMsg.ulMessageType == STATE_DIGITAL_OUTPUT){
                    printf("Setting state for %s, value %d\n",stateMsg.descriptor, stateMsg.val.int_value);
                    getStateItem(stateMsg.descriptor)->state_value.int_value = stateMsg.val.int_value; 
                    save_state_digital(stateMsg.descriptor);
                }
                print_state();  
            }
        }        
    }
    state_deinit();
}

int getGPIOFromDescriptor(char* descriptor){
    stateItem* state = getStateItem(descriptor);
    if(state)
        return state->gpio;
    else
        return -1;    
}

char* getDescriptorFromGPIO(int gpio){
    for(int i=0 ; i < sizeof states; i++){
        if (gpio == states[i].gpio){
            return (char*)&states[i].descriptor;
        }
    }
    return NULL;  
}

int getADCFromDescriptor(char* descriptor){
    stateItem* state = getStateItem(descriptor);
    if(state)
        return state->adc;
    else
        return -1;    
}

/**
 * 
 * Posts a state message to the queue
 * 
*/
BaseType_t sendStateMessage(stateMessage stateMsg){
    /* Send the entire structure to the queue created to hold 10 structures. */
   xQueueSend( /* The handle of the queue. */
               xStateQueue,
               /* The address of the xMessage variable.  sizeof( struct AMessage )
               bytes are copied from here into the queue. */
               ( void * ) &stateMsg,
               /* Block time of 0 says don't block if the queue is already full.
               Check the value returned by xQueueSend() to know if the message
               was sent to the queue successfully. */
               ( TickType_t ) 0 );
}

void print_state(){

    printf("\nANALOG OUT\n");
    printf("AO01 : %.5f\n", getStateItem("AO01")->state_value.float_value);
    printf("AO02 : %.5f\n", getStateItem("AO02")->state_value.float_value);
    printf("AO03 : %.5f\n", getStateItem("AO03")->state_value.float_value);

    printf("\nDIGITAL OUT\n");
    printf("DO01 : %d\n", getStateItem("DO01")->state_value.int_value);
    printf("DO02 : %d\n", getStateItem("DO02")->state_value.int_value);
    printf("DO03 : %d\n", getStateItem("DO03")->state_value.int_value);
    printf("DO04 : %d\n", getStateItem("DO04")->state_value.int_value);
    printf("DO05 : %d\n", getStateItem("DO05")->state_value.int_value);
    printf("DO06 : %d\n", getStateItem("DO06")->state_value.int_value);

}

void save_state_analog(char* descriptor){
    int rc;
    char buffer[32];

    sprintf(buffer, "%.8f", getStateItem(descriptor)->state_value.float_value);
    rc = ini_puts("ANALOG OUT", descriptor, buffer, inifile);
}

void save_state_digital(char* descriptor){
    int rc;
    char buffer[32];

    sprintf(buffer, "%d", getStateItem(descriptor)->state_value.int_value);
    rc = ini_puts("DIGITAL OUT", descriptor, buffer, inifile);
}

void save_state(){

    int rc;
    char buffer[32];

    sprintf(buffer, "%.8f", getStateItem("AO01")->state_value.float_value);
    rc = ini_puts("ANALOG OUT", "AO01", buffer, inifile);
    sprintf(buffer, "%.8f", getStateItem("AO02")->state_value.float_value);
    rc = ini_puts("ANALOG OUT", "AO02", buffer, inifile);
    sprintf(buffer, "%.8f", getStateItem("AO03")->state_value.float_value);
    rc = ini_puts("ANALOG OUT", "AO03", buffer, inifile);

    sprintf(buffer, "%d", getStateItem("DO01")->state_value.int_value);
    rc = ini_puts("DIGITAL OUT", "DO01", buffer, inifile);
    sprintf(buffer, "%d", getStateItem("DO02")->state_value.int_value);
    rc = ini_puts("DIGITAL OUT", "DO02", buffer, inifile);
    sprintf(buffer, "%d", getStateItem("DO03")->state_value.int_value);
    rc = ini_puts("DIGITAL OUT", "DO03", buffer, inifile);
    sprintf(buffer, "%d", getStateItem("DO04")->state_value.int_value);
    rc = ini_puts("DIGITAL OUT", "DO04", buffer, inifile);
    sprintf(buffer, "%d", getStateItem("DO05")->state_value.int_value);
    rc = ini_puts("DIGITAL OUT", "DO05", buffer, inifile);
    sprintf(buffer, "%d", getStateItem("DO06")->state_value.int_value);
    rc = ini_puts("DIGITAL OUT", "DO06", buffer, inifile);    
}

void initialize_state(){
 
    int i;
    char buffer[64];
    i = ini_gets("ANALOG OUT", "AO01", 0, buffer, sizeof buffer, inifile);
    getStateItem("AO01")->state_value.float_value = strtof(buffer, NULL);
    i = ini_gets("ANALOG OUT", "AO02", 0, buffer, sizeof buffer, inifile);
    getStateItem("AO02")->state_value.float_value = strtof(buffer, NULL);
    i = ini_gets("ANALOG OUT", "AO03", 0, buffer, sizeof buffer, inifile);
    getStateItem("AO03")->state_value.float_value = strtof(buffer, NULL);
    
    getStateItem("DO01")->state_value.int_value = ini_getl("DIGITAL OUT", "DO01", 0, inifile);
    getStateItem("DO02")->state_value.int_value = ini_getl("DIGITAL OUT", "DO02", 0, inifile);
    getStateItem("DO03")->state_value.int_value = ini_getl("DIGITAL OUT", "DO03", 0, inifile);
    getStateItem("DO04")->state_value.int_value = ini_getl("DIGITAL OUT", "DO04", 0, inifile);
    getStateItem("DO05")->state_value.int_value = ini_getl("DIGITAL OUT", "DO05", 0, inifile);
    getStateItem("DO06")->state_value.int_value = ini_getl("DIGITAL OUT", "DO06", 0, inifile);

}

void set_first_state(){
    setStateItemFloat("AO01", 0);
    setStateItemFloat("AO02", 0);
    setStateItemFloat("AO03", 0);

    setStateItemInt("DO01", 0);
    setStateItemInt("DO02", 0);
    setStateItemInt("DO03", 0);
    setStateItemInt("DO04", 0);
    setStateItemInt("DO05", 0);
    setStateItemInt("DO06", 0); 
}


/**
 * 
 * Create the state task
 * 
 */
void state_init(){

    if (!mount(&pxDisk, DEVICENAME, MOUNTPOINT)) 
        panic("Failed to mount disk for ini file\n");

    //initialize_state();    

    // create state queue
    vCreateStateQueue();
    // create the state task
    xTaskCreate(&state_task, "State task", 1024, NULL, 3, NULL);
}
