#include "hardware/adc.h"

#define TRACE_PRINTF(fmt, args...)
//#define TRACE_PRINTF printf

float  getCoreTemperature(char units){

// The temperature sensor is on input 4:
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);
    uint16_t result = adc_read();
    // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
    const float conversion_factor = 3.3f / (1 << 12);
    float voltage = conversion_factor * result;
    TRACE_PRINTF("Raw value: 0x%03x, voltage: %f V\n", result,
                    (double)voltage);

    // Temperature sensor values can be approximated in centigrade as:
    //    T = 27 - (ADC_Voltage - 0.706)/0.001721
    float Tc = 27.0f - (voltage - 0.706f) / 0.001721f;
    TRACE_PRINTF("Temperature: %.1f °F\n", (double)Tc);
    if (units == 'F')
        return Tc * 9/5 + 32;
    else{    
        return Tc;
    }
}