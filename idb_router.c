/**
 * Handle uri routing for both GET and POST 
*/

#include <stdio.h>
#include <string.h>

#include <hardware/adc.h>
#include <stdlib.h>
#include "pico/cyw43_arch.h"

#include "ff_headers.h"
#include "ff_utils.h"
#include "ff_stdio.h"

#include "idb_router.h"

#include "idb_hardware.h"

#define DEVICENAME "sd0"
#define MOUNTPOINT "/sd0"

/* Choose 'C' for Celsius or 'F' for Fahrenheit. */
#define TEMPERATURE_UNITS 'F'

/**
 * 
 * 
 * Calls getCoreTemperature in adc.c to get the 
 * 
 * 
*/
float read_onboard_temperature(const char unit) {

    float temp = getCoreTemperature(unit);

    if (unit == 'C') {
        return temp;
    } else if (unit == 'F') {
        return temp * 9 / 5 + 32;
    }

    return -1.0f;
}

int printJSONHeaders(char* buffer, int count){
    strcpy(buffer, "HTTP/1.1 200 OK\n");
    strcat(buffer, "Content-Type: application/json\n");
    sprintf(buffer+strlen(buffer), "Content-Length: %d\n\n", count);
    printf(buffer);
    return strlen(buffer);
}

void returnTemperature(NameFunction* ptr, char* buffer, int count){

    char buf[64];

    float temperature = getCoreTemperature(TEMPERATURE_UNITS);

    printf("Onboard temperature =  %.3g %c\n", (double)temperature, TEMPERATURE_UNITS);

    if (buffer){
        // Output JSON very simply
        int len = sprintf(buf, "{\"temperature\": %.3g,\"temperature units\": \"%c\"}", (double)temperature, TEMPERATURE_UNITS);
        printJSONHeaders(buffer, len);
        strcat(buffer, buf);
    }
}

int getSetValue(NameFunction* ptr){

    char buff[64]; 
    strcpy(buff, ptr->uri);
    char* subString = strtok(buff,"/"); // find the first /
    char* subString2 = strtok(NULL,"/");       // find the second /
    if (subString2 == NULL)
        return -1;
    else{
        //printf("Got LED uri value %s", subString2);
        return atoi(subString2);
    }
}

void returnLED(NameFunction* ptr, char* buffer, int count){
    //printf("Getting LED value");
    char buf[64];
    int led = cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN);
    if (buffer){
        // Output JSON very simply
        int len = sprintf(buf, "{\"led\": %d}", led);
        printJSONHeaders(buffer, len);
        strcat(buffer, buf);
    }
}

void setLED(NameFunction* ptr, char* buffer, int count){
    int value = getSetValue(ptr);
    printf("Setting LED value %d", value);
    char buf[64];
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, value);
    if (buffer){    
        int len = sprintf(buf, "%s", "{\"success\" : true}");
        printJSONHeaders(buffer, len); // print out headers with no body
        strcat(buffer, buf);
    }
}

void setGPIO(NameFunction* ptr, char* buffer, int count, int gpio_pin, int gpio_value){

    char buf[64];

    gpio_init(gpio_pin);
    gpio_set_dir(gpio_pin, GPIO_OUT);
    gpio_put(gpio_pin, gpio_value);
    if (buffer){       
        int len = sprintf(buf, "%s", "{\"success\" : true}");
        printJSONHeaders(buffer, len); // print out headers with no body
        strcat(buffer, buf);
    }
    printf("exiting setGPIO\n");
}

int getGPIO(NameFunction* ptr){

    char buff[64]; 
    strcpy(buff, ptr->uri);
    char* subString = strtok(buff,"/"); // find the first /
    char* subString2 = strtok(NULL,"/");       // find the second /
    if (subString2 == NULL)
        return -1;
    else{
        //printf("Got GPIO number %s\n", subString2);
        return atoi(subString2);
    }
}

int getGPIOValue(NameFunction* ptr){

    char buff[64]; 
    strcpy(buff, ptr->uri);
    char* subString = strtok(buff,"/"); // find the first /
    char* subString2 = strtok(NULL,"/");       // find the second /
    if (subString2 == NULL)
        return -1;
    char* subString3 = strtok(NULL,"/");       // find the second /
    if (subString3 == NULL)
        return -1;
    else{    
        //printf("Got GPIO value %s\n", subString3);
        return atoi(subString3);
    }
}

void returnGPIO(NameFunction* ptr, char* buffer, int count, int gpio_pin){
    
    char buf[64];
    int gpio_value = (int)gpio_get(gpio_pin);

    if (buffer){
        // Output JSON very simply
        //printf("Returning GPIO status %d", gpio_pin);
        int len = sprintf(buf, "{\"%i\" : %d}", gpio_pin, gpio_value);
        printJSONHeaders(buffer, len);
        strcat(buffer, buf);
    }
}

void gpio(NameFunction* ptr, char* buffer, int count){
    int GPIOpin = getGPIO(ptr);
    int value = getGPIOValue(ptr);
    if(value != -1){
        //printf("Setting GPIO value %d, %d\n", GPIOpin, value);
        setGPIO(ptr, buffer, count, GPIOpin, value);
    }
    else{
        //printf("Returning GPIO value %d\n", GPIOpin);
        returnGPIO(ptr, buffer, count, GPIOpin);
    }
}

// each one more than required for terminating null
#define DATETIMELEN 23
#define TEMPLEN 6
#define MAXREADINGS 10
#define MAXLEN 54

typedef struct Measurement{
    char date_time[DATETIMELEN];
    char temperature[TEMPLEN];
} Measurement;

void getData(char *buff, Measurement* reading){  
    char* ptr = strtok(buff, ",");
    strncpy(reading->date_time, ptr, DATETIMELEN);
    ptr = strtok(NULL, "\n");
    strncpy(reading->temperature, ptr, TEMPLEN);
}

void getLogDate(NameFunction* ptr, char* buffer){

    char buff[64]; 
    strcpy(buff, ptr->uri);

    char* subString = strtok(buff,"/"); // find the first /
    subString = strtok(NULL,"/");   // find the second /
    //subString = strtok(NULL,"/");   // find the third /

    strcpy(buffer, subString);
}

FF_FILE* openLogFile(char* name){

    char buffer[128];

    FF_Disk_t *pxDisk = NULL;

    if (!mount(&pxDisk, DEVICENAME, MOUNTPOINT)){
        printf("Failed to mount %s as %s", DEVICENAME, MOUNTPOINT);
    }

    int n = snprintf(buffer, sizeof buffer, "%s/data/%s", MOUNTPOINT, name);                    
    configASSERT(0 < n && n < (int)sizeof buffer);
    if (-1 == mkdirhier(buffer) &&
        stdioGET_ERRNO() != pdFREERTOS_ERRNO_EEXIST) {
        printf("failed to set directory");
        return NULL;
    }

    printf("Reading log\n");

    snprintf(buffer + n, sizeof buffer - n, "/log_data.csv");
    printf("Opening log file: %s\n", buffer);  
    return ff_fopen(buffer, "r");
}

void readLog(char* name, char* jsonBuffer, int count){

    Measurement readings[MAXREADINGS];
    Measurement reading;
    char buffer[128];

    printf("Buffer length: %d, name: %s\n", count, name);

    FF_FILE *pxFile = openLogFile(name);

    // need to check if mounted?
    if (pxFile){

        printf("log file open\n");
        int i = 0;

        sprintf(jsonBuffer, "[");
        int len = 1;
        int bufferPos = len;

        // seek to the end of the file
        ff_fseek(pxFile, 0, FF_SEEK_END);
        int pos = ff_ftell(pxFile);
        /* Don't write each char on output.txt, just search for '\n' */

        int recCount = 0;
        // seek backwards for max number of records that will fit in the buffer
        while (pos) {
            ff_fseek(pxFile, --pos, FF_SEEK_SET);
            if (ff_fgetc(pxFile) == '\n') {
                if (recCount++ == count/MAXLEN) 
                    break;
            }
        }

        printf("Reading log file records\n");
        // now positioned at last n records
        while(ff_fgets(buffer, sizeof(buffer), pxFile)){
            if (i >= MAXREADINGS)
                break;
            getData(buffer, &reading);
            memcpy(&readings[i], &reading, sizeof(reading));
            //printf("Reading [%d] : Date: %s, Temperature: %s\n", i, readings[i].date_time, readings[i].temperature );  
            if (bufferPos < count){          
                len = snprintf(jsonBuffer+bufferPos, count-bufferPos, "{\"date\": %s, \"temperature\": %s, \"temperature units\": \"%c\"}, ", 
                                                readings[i].date_time, readings[i].temperature, 'F');
                bufferPos += len;
                i++;
            }
            else{
                break;
            }
        }
        sprintf(jsonBuffer+bufferPos-2, "]");
        ff_fclose(pxFile);  
    }

}

void readLogWithDate(NameFunction* ptr, char* buffer, int count){
    #define MINBUFSIZE 64
    #define JSONBUFFSIZE 1024
    
    char* logDate = pvPortMalloc(MINBUFSIZE);
     if (logDate){
        char* buf = pvPortMalloc(JSONBUFFSIZE);  
        if (buf){
            getLogDate(ptr, logDate);
            printf("log date: %s\n", logDate);
            readLog(logDate, buf, JSONBUFFSIZE);
            printf("Response from readLog: %s\n", buf);
            int len = strlen(buf);
            int hdrLen = printJSONHeaders(buffer, len);
            strcat(buffer+hdrLen, buf);
            vPortFree(buf);
        }
        else{
            printf("Failed to allocate log buffer\n");
        }
        vPortFree(logDate);
     }
     else{
        printf("Failed to allocate logDate\n");
     }
}

// this is pseudo-REST as it is all via GET - POST is not implemented.
// : values are simply indicators of position, they are not used in parsing

void success(NameFunction* ptr, char* buffer, int count){
    printf("Success\n");
    char buf[64];
    int len = sprintf(buf, "%s", "{\"success\" : true}");
    printJSONHeaders(buffer, len); // print out headers with no body
    strcat(buffer, buf);
}

void failure(NameFunction* ptr, char* buffer, int count){
    printf("Failure\n");
    char buf[64];
    int len = sprintf(buf, "%s", "{\"success\" : false}");
    printJSONHeaders(buffer, len); // print out headers with no body
    strcat(buffer, buf);
}


NameFunction routes[] =
{ 
    { "/temp", (void*) *returnTemperature, HTTP_GET },
    { "/temperature", (void*) *returnTemperature, HTTP_GET },
    { "/led", (void*) *returnLED, HTTP_GET }, 
    { "/led/:value", (void*) *setLED, HTTP_POST }, 
    { "/gpio/:gpio", (void*) *gpio, HTTP_GET },  
    { "/gpio/:gpio/:value", (void*) *gpio, HTTP_POST }, 
    { "/readlog/:date", (void*) *readLogWithDate, HTTP_GET },
    { "/failure", (void*) *failure, HTTP_GET || HTTP_POST},
    { "/success", (void*) *success, HTTP_GET || HTTP_POST},
};

NameFunction* parseExact(const char* name, int routeType){
    //printf("Parsing exact route : %s\n", name);
    for (NameFunction* ptr = routes;ptr != routes + sizeof(routes) / sizeof(routes[0]); ptr++)
    {
        if (ptr->routeType != routeType)
            continue;
        if(!strcmp(ptr->routeName, name)) {
            return ptr;
        }
    }
    return 0;
}

NameFunction* parsePartialMatch(const char* name, int routeType){

    char buf[64];
    
    //printf("Parsing partial route : %s\n", name);
    for (NameFunction* ptr = routes;ptr != routes + sizeof(routes) / sizeof(routes[0]); ptr++)
    {
        if (ptr->routeType != routeType)
            continue;
        if(!strstr(ptr->routeName, ":")) // can't partial match if no variable
            continue;
        strcpy(buf, ptr->routeName);
        char* tok = strtok(buf, "/"); // start of route
        if(!strncmp(tok, name+1, strlen(tok))) { // first token must match
            //printf("Token: %s\n", tok);
            return ptr;
        }
    }
    return 0;
}

NameFunction* isRoute(const char* name, int routeType){

    // if (routeType != HTTP_GET && routeType != HTTP_POST)
    //     routeType = HTTP_GET; // default to GET
    //printf("Checking route : %s, Type: routeType\n", name);
    NameFunction* ptr = parseExact(name, routeType);

    if (!ptr){ // always return an exact match if found otherwise look for partial
        ptr = parsePartialMatch(name, routeType);
    }
    if (ptr){
        printf("Confirmed route %s for uri %s\n", ptr->routeName, ptr->uri);
        ptr->uri = pvPortMalloc(strlen(name));
        strcpy(ptr->uri, name);
    }
    if (ptr)
        printf("In route table\n");
    else
        printf("Not in route table\n");    
    return ptr;  
}

void route(NameFunction* ptr, char* buffer, int count){
    if (buffer){       
        //printf("Routing %\n", buffer);
        ptr->routeFunction(ptr, buffer, count);
        //printf("routed uri: %s\n", ptr->uri);
        if (ptr->uri)
            vPortFree(ptr->uri);
        //printf("after routeFunction\n");
    }
    else{
        printf("no buffer\n");
    }
    //printf("returned from route\n");
}




