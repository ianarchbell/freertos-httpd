#ifndef IDB_HARDWARE_H
#define IDB_HARDWARE_H

#include <pico/types.h>

float getCoreTemperature(char units);

int16_t readADC(char* descriptor);
float readADCraw(char* descriptor);
void analogOutput(char* descriptor, float value);
void digitalOutput(char* descriptor, int value);
void hardware_init();
void reflect_state();

#endif 