#include <stdio.h>
#include <string.h>

#include "hardware/adc.h"

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

void returnTemperature(char* buffer, int count){
    float temperature = read_onboard_temperature(TEMPERATURE_UNITS);
    printf("Onboard temperature = %.02f %c\n", temperature, TEMPERATURE_UNITS);

    if (buffer){
        // Output JSON very simply
        sprintf(buffer, "{\"temperature\": %.02f,\"temperature units\": \"%c\"}", temperature, TEMPERATURE_UNITS);
    }
}

typedef struct NameFunction_struct
{
    const char* routeName;
    void (*routeFunction)(char*, int);
}
NameFunction;

NameFunction routes[] =
{ 
    { "/temp.json", *returnTemperature },
    { "/temperature.json", *returnTemperature }
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
    printf("routing with a function pointer, max length: %i\n", count);
    if (buffer){        
        ptr->routeFunction(buffer, count);
    }
    else{
        printf("no buffer\n");
    }
    printf("returned from route\n");
}





