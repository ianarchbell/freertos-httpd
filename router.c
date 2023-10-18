#include <stdio.h>
#include <string.h>

#include "hardware/adc.h"
#include <stdlib.h>
#include "pico/cyw43_arch.h"

#include "ff_headers.h"
#include "ff_utils.h"
#include "ff_stdio.h"

#include "router.h"

#include "adc.h"

#define DEVICENAME "sd0"
#define MOUNTPOINT "/sd0"

/* Choose 'C' for Celsius or 'F' for Fahrenheit. */
#define TEMPERATURE_UNITS 'F'

/* References for this implementation:
 * raspberry-pi-pico-c-sdk.pdf, Section '4.1.1. hardware_adc'
 * pico-examples/adc/adc_console/adc_console.c */
float read_onboard_temperature(const char unit) {


    float temp = getCoreTemperature(unit);
    /* Initialize hardware AD converter, enable onboard temperature sensor and
     *   select its channel (do this once for efficiency, but beware that this
     *   is a global operation). */
    // adc_init();
    // adc_set_temp_sensor_enabled(true);
    // adc_select_input(4);
    
    // /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    // const float conversionFactor = 3.3f / (1 << 12);

    // float adc = (float)adc_read() * conversionFactor;
    // float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

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
        printf("Got LED uri value %s", subString2);
        return atoi(subString2);
    }
}

void returnLED(NameFunction* ptr, char* buffer, int count){
    printf("Getting LED value");
    char buf[64];
    int led = cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN);
    if (buffer){
        // Output JSON very simply
        int len = sprintf(buf, "{\"led\": %d}", led);
        printJSONHeaders(buffer, len);
        strcat(buffer, buf);
    }
}

void setLED(NameFunction* ptr, char* buffer, int count, int value){
    printf("Setting LED value %d", value);
    char buf[64];
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, value);
    if (buffer){    
        int len = sprintf(buf, "%s", "{\"success\" : true}");
        printJSONHeaders(buffer, len); // print out headers with no body
        strcat(buffer, buf);
    }
}

void led(NameFunction* ptr, char* buffer, int count){
    int i = getSetValue(ptr);
    if(i != -1){
        setLED(ptr, buffer, count, i);
    }
    else{
        returnLED(ptr, buffer, count);
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
        printf("Got GPIO number %s\n", subString2);
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
        printf("Got GPIO value %s\n", subString3);
        return atoi(subString3);
    }
}

void returnGPIO(NameFunction* ptr, char* buffer, int count, int gpio_pin){
    
    char buf[64];
    int gpio_value = (int)gpio_get(gpio_pin);

    if (buffer){
        // Output JSON very simply
        printf("Returning GPIO status %d", gpio_pin);
        int len = sprintf(buf, "{\"%i\" : %d}", gpio_pin, gpio_value);
        printJSONHeaders(buffer, len);
        strcat(buffer, buf);
    }
}

void gpio(NameFunction* ptr, char* buffer, int count){
    int GPIOpin = getGPIO(ptr);
    int value = getGPIOValue(ptr);
    if(value != -1){
        printf("Setting GPIO value %d, %d\n", GPIOpin, value);
        setGPIO(ptr, buffer, count, GPIOpin, value);
    }
    else{
        printf("Returning GPIO value %d\n", GPIOpin);
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

void readLog(char* name, char* jsonBuffer, int count){

    printf("Buffer length: %d, name: %s\n", count, name);

    char buffer[128];
    FF_FILE *pxFile;

    Measurement readings[MAXREADINGS];

    Measurement reading;

    FF_Disk_t *pxDisk = NULL;

    if (!mount(&pxDisk, DEVICENAME, MOUNTPOINT)){
        printf("Failed to mount %s as %s", DEVICENAME, MOUNTPOINT);
    }

    int n = snprintf(buffer, sizeof buffer, "%s/data/%s", MOUNTPOINT, name);                    
    configASSERT(0 < n && n < (int)sizeof buffer);
    if (-1 == mkdirhier(buffer) &&
        stdioGET_ERRNO() != pdFREERTOS_ERRNO_EEXIST) {
        printf("failed to set directory");
        return;
    }

    printf("Reading log\n");

    snprintf(buffer + n, sizeof buffer - n, "/log_data.csv");
    //configASSERT(nw);
    pxFile = ff_fopen(buffer, "r");
    printf("Opening log file: %s\n", buffer);

    // need to check if mounted
    if (pxFile){
        int i = 0;
        char* ptr = jsonBuffer;
        sprintf(jsonBuffer, "[");
        int len = 1;
        int bufferPos = len;
        //ff_fgets(buffer, sizeof(buffer), pxFile); // ignore header

        ff_fseek(pxFile, 0, FF_SEEK_END);
        int pos = ff_ftell(pxFile);
        /* Don't write each char on output.txt, just search for '\n' */

        int recCount = 0;

        while (pos) {
            ff_fseek(pxFile, --pos, FF_SEEK_SET); /* seek from begin */
            if (ff_fgetc(pxFile) == '\n') {
                if (recCount++ == count/MAXLEN) 
                    break;
            }
        }

        while(ff_fgets(buffer, sizeof(buffer), pxFile)){
            if (i >= MAXREADINGS)
                break;
            getData(buffer, &reading);
            memcpy(&readings[i], &reading, sizeof(reading));
            printf("Count: %d\n", count);
            printf("Reading [%d] : Date: %s, Temperature: %s\n", i, readings[i].date_time, readings[i].temperature );  
            if (bufferPos+MAXLEN < count){          
                len = snprintf(ptr+bufferPos, count-bufferPos, "{\"date\": %s, \"temperature\": %s, \"temperature units\": \"%c\"}, ", 
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
    
    char* logDate = malloc(MINBUFSIZE);
     if (logDate){
        char* buf = malloc(JSONBUFFSIZE);  
        if (buf){
            getLogDate(ptr, logDate);
            printf("log date: %s\n", logDate);
            readLog(logDate, buf, JSONBUFFSIZE);
            printf("Response from readLog: %s\n", buf);
            int len = strlen(buf);
            int hdrLen = printJSONHeaders(buffer, len);
            strcat(buffer+hdrLen, buf);
            free(buf);
        }
        else{
            printf("Failed to allocate log buffer\n");
        }
        free(logDate);
     }
     else{
        printf("Failed to allocate logDate\n");
     }
}

// this is pseudo-REST as it is all via GET - POST is not implemented.
// : values are simply indicators of position, they are not used in parsing

NameFunction routes[] =
{ 
    { "/temp", (void*) *returnTemperature },
    { "/temperature", (void*) *returnTemperature },
    { "/led/:value", (void*) *led }, 
    { "/gpio/:gpio/:value", (void*) *gpio },  
    { "/readlog/:date", (void*) *readLogWithDate },
};

NameFunction* parseExact(const char* name){
    printf("Parsing exact route : %s\n", name);
    for (NameFunction* ptr = routes;ptr != routes + sizeof(routes) / sizeof(routes[0]); ptr++)
    {
        if(!strcmp(ptr->routeName, name)) {
            return ptr;
        }
    }
    return 0;
}

NameFunction* parsePartialMatch(const char* name){
    printf("Parsing partial route : %s\n", name);
    for (NameFunction* ptr = routes;ptr != routes + sizeof(routes) / sizeof(routes[0]); ptr++)
    {
        for (int routeLen = strlen(name); routeLen > 0; routeLen --){
            if(!strncmp(ptr->routeName, name, routeLen)) {
                if (routeLen > 1){ // don't return a match on '/'
                    return ptr;
                }
            }
        }
    }
    return 0;
}

NameFunction* isRoute(const char* name){

    printf("Routing : %s\n", name);
    NameFunction* ptr = parseExact(name);

    if (!ptr){ // always return an exact match if found otherwise look for partial
        ptr = parsePartialMatch(name);
    }
    if (ptr){
        ptr->uri = malloc(strlen(name));
        strcpy(ptr->uri, name);
    }
    return ptr;  
}

void route(NameFunction* ptr, char* buffer, int count){
    if (buffer){       
        printf("Routing %\n", buffer);
        ptr->routeFunction(ptr, buffer, count);
        printf("routed uri: %s\n", ptr->uri);
        if (ptr->uri)
            free(ptr->uri);
        //printf("after routeFunction\n");
    }
    else{
        printf("no buffer\n");
    }
    //printf("returned from route\n");
}





