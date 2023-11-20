#ifndef IDB_HARDWARE_H
#define IDB_HARDWARE_H

#include <pico/types.h>

float  getCoreTemperature();

int16_t readADC(int adc);
float readADCraw(int adc);
void analogOutput(int doPort, double value);
void hardware_init();

#endif 