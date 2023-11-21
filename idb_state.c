#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <ff_utils.h>

#include "minIni.h"

#include "idb_state.h"
#include "idb_config.h"

/* Queue used to send and receive complete struct AMessage structures. */
QueueHandle_t xStateQueue = NULL;

#define DEVICENAME "sd0"
#define MOUNTPOINT "/sd0"

FF_Disk_t *pxDisk = NULL;

const char inifile[] = "/sd0/idb.ini";

stateItem states[9] = {

{"AO01", AO01}, 
{"AO02", AO02},
{"AO03", AO03},

{"DO01", DO01, 1},
{"DO02", DO02, 0},
{"DO03", DO03, 1},
{"DO04", DO04, 0},
{"DO05", DO05, 1},
{"DO06", DO06, 0},

};

stateItem* getStateItem(char* descriptor){
    for(int i=0 ; i < sizeof states; i++){
        if (!strcmp(descriptor, states[i].descriptor)){
            return &states[i];
        }
    }
    return NULL;
}

int setStateItemFloat(char* descriptor, float value){
    for(int i=0 ; i < sizeof states; i++){
        if (!strcmp(descriptor, states[i].descriptor)){
            states[i].state_value.float_value = value;
        }
    }
    return -1;
}

int setStateItemInt(char* descriptor, int32_t value){
    for(int i=0 ; i < sizeof states; i++){
        if (!strcmp(descriptor, states[i].descriptor)){
            states[i].state_value.int_value = value;
        }
    }
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
   
    set_first_state();
    //print_state();
    save_state();
    //initialize_state();
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
                if (stateMsg.ulMessageType == ANALOG_VALUE)
                    getStateItem(stateMsg.descriptor)->state_value.float_value = stateMsg.val.float_value;
                else if  (stateMsg.ulMessageType == DIGITAL_VALUE)
                    getStateItem(stateMsg.descriptor)->state_value.int_value = stateMsg.val.int_value;   
            }
        }        
    }
    state_deinit();
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
    setStateItemFloat("AO01", 1.025);
    setStateItemFloat("AO02", 0.123456);
    setStateItemFloat("AO03", 3.14756);
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
