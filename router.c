#include <stdio.h>
#include <string.h>

#include "hardware/adc.h"

typedef struct NameFunction_struct
{
    const char* routeName;
    void (*routeFunction)(void*, char*, int);
}
NameFunction;

/* Choose 'C' for Celsius or 'F' for Fahrenheit. */
#define TEMPERATURE_UNITS 'F'

/* References for this implementation:
 * raspberry-pi-pico-c-sdk.pdf, Section '4.1.1. hardware_adc'
 * pico-examples/adc/adc_console/adc_console.c */
float read_onboard_temperature(const char unit) {

    /* Initialize hardware AD converter, enable onboard temperature sensor and
     *   select its channel (do this once for efficiency, but beware that this
     *   is a global operation). */
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);
    
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    if (unit == 'C') {
        return tempC;
    } else if (unit == 'F') {
        return tempC * 9 / 5 + 32;
    }

    return -1.0f;
}

void returnTemperature(NameFunction* ptr, char* buffer, int count){
    float temperature = read_onboard_temperature(TEMPERATURE_UNITS);
    printf("Onboard temperature = %.02f %c\n", temperature, TEMPERATURE_UNITS);

    if (buffer){
        // Output JSON very simply
        sprintf(buffer, "{\"temperature\": %.02f,\"temperature units\": \"%c\"}", temperature, TEMPERATURE_UNITS);
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
    int gpio_value = gpio_get(gpio_pin);

    if (buffer){
        // Output JSON very simply
        sprintf(buffer, "{\"%s\" : %d}", gpio_pin, gpio_value);
    }
}

void setGPIO(NameFunction* ptr, char* buffer, int count){
    printf("in setGPIO\n");
    int gpio_pin = strtol(getGPIO(ptr), NULL, 10);
    printf("gpio pin: %i\n",gpio_pin);
    char* gpio_value = strtol(getGPIOvalue(ptr), NULL, 10);
    printf("gpio value: %i\n", gpio_value);
    gpio_init(gpio_pin);
    gpio_set_dir(gpio_pin, GPIO_OUT);
    gpio_put(gpio_pin, gpio_value);
    if (buffer){       
        sprintf(buffer, ""); // nothing to send
    }
    printf("exiting setGPIO\n");
}

NameFunction routes[] =
{ 
    { "/temp.json", *returnTemperature },
    { "/temperature.json", *returnTemperature },
    { "/led.json", *returnLED },
    { "/led/1.json", *setLEDon },
    { "/led/0.json", *setLEDoff },
    { "/gpio/18/get.json", *returnGPIO },
    { "/gpio/18/1/set.json", *setGPIO },
    { "/gpio/18/0/set.json", *setGPIO },
};


NameFunction* isRoute(const char* name){
    printf("routing : %s\n", name);
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
        printf("routing with a function pointer, max length: %i\n", count);
        ptr->routeFunction(ptr, buffer, count);
        printf("after routeFunction\n");
    }
    else{
        printf("no buffer\n");
    }
    printf("returned from route\n");
}





