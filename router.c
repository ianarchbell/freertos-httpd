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

void returnTemperature(NameFunction* ptr, char* buffer, int count){

    float temperature = getCoreTemperature(TEMPERATURE_UNITS);

    printf("Onboard temperature =  %.3g %c\n", (double)temperature, TEMPERATURE_UNITS);

    if (buffer){
        // Output JSON very simply
        sprintf(buffer, "{\"temperature\": %.3g,\"temperature units\": \"%c\"}", (double)temperature, TEMPERATURE_UNITS);
    }
}

void returnLED(NameFunction* ptr, char* buffer, int count){

    //cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    int led = cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN);
    if (buffer){
        // Output JSON very simply
        sprintf(buffer, "{\"led\": %d}", led);
    }
}

void setLEDon(NameFunction* ptr, char* buffer, int count){

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    if (buffer){       
        sprintf(buffer, ""); // nothing to send
    }
}

void setLEDoff(NameFunction* ptr, char* buffer, int count){

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    if (buffer){ 
        sprintf(buffer, ""); // nothing to send
    }
}

char* getGPIO(NameFunction* ptr){

    char buffer[1024];  // where we will put a copy of the input
    strcpy(buffer, ptr->routeName);

    char* subString = strtok(buffer,"/"); // find the first double quote
    subString = strtok(NULL,"/");   // find the second double quote

    return subString;
}

char* getGPIOvalue(NameFunction* ptr){

    char buffer[1024];  // where we will put a copy of the input
    strcpy(buffer, ptr->routeName);

    char* subString = strtok(buffer,"/"); // find the first /
    subString = strtok(NULL,"/");   // find the second /
    subString = strtok(NULL,"/");   // find the third /

    return subString;
}

void returnGPIO(NameFunction* ptr, char* buffer, int count){
    
    char* gpio_pin = getGPIO(ptr);
    int gpio_value = (int)gpio_get(atoi(gpio_pin));

    if (buffer){
        // Output JSON very simply
        sprintf(buffer, "{\"%s\" : %d}", gpio_pin, gpio_value);
    }
}

void setGPIO(NameFunction* ptr, char* buffer, int count){
    printf("in setGPIO\n");
    int gpio_pin = strtol(getGPIO(ptr), NULL, 10);
    printf("gpio pin: %i\n",gpio_pin);
    int gpio_value = strtol(getGPIOvalue(ptr), NULL, 10);
    printf("gpio value: %i\n", gpio_value);
    gpio_init(gpio_pin);
    gpio_set_dir(gpio_pin, GPIO_OUT);
    gpio_put(gpio_pin, gpio_value);
    if (buffer){       
        sprintf(buffer, ""); // nothing to send
    }
    printf("exiting setGPIO\n");
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
    strcpy(buff, ptr->routeName);

    char* subString = strtok(buff,"/"); // find the first /
    subString = strtok(NULL,"/");   // find the second /
    //subString = strtok(NULL,"/");   // find the third /

    strcpy(buffer, subString);
}

void readLog(char* name, char* jsonBuffer, int count){

    printf("Buffer length: %d, name: %s", count, name);

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
    printf("Opening log file: %s", buffer);

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
    char*  logDate = malloc(64);  
    getLogDate(ptr, logDate);
    printf("log date: %s\n", logDate);
    readLog(logDate, buffer, count);
    free(logDate);
}

NameFunction routes[] =
{ 
    { "/temp.json", (void*) *returnTemperature },
    { "/temperature.json", (void*) *returnTemperature },
    { "/led.json", (void*) *returnLED },
    { "/led/1.json", (void*) *setLEDon },
    { "/led/0.json", (void*) *setLEDoff },
    { "/gpio/18/get.json", (void*) *returnGPIO },
    { "/gpio/18/1/set.json", (void*) *setGPIO },
    { "/gpio/18/0/set.json", (void*) *setGPIO },
    { "/readlog/2023-10-12/get.json", (void*) *readLogWithDate },
};


NameFunction* isRoute(const char* name){

    printf("Routing : %s\n", name);
    for (NameFunction* ptr = routes;ptr != routes + sizeof(routes) / sizeof(routes[0]); ptr++)
    {
        if(!strcmp(ptr->routeName, name)) {
            return ptr;
        }
    }
    return 0;
}

void route(NameFunction* ptr, char* buffer, int count){
    if (buffer){       
        //printf("Routing %\n", buffer);
        ptr->routeFunction(ptr, buffer, count);
        //printf("after routeFunction\n");
    }
    else{
        printf("no buffer\n");
    }
    //printf("returned from route\n");
}





