#include "idb_config.h"
#include <pico/types.h>
#include <hardware/adc.h>
#include <hardware/pwm.h>

#include "idb_websocket.h"

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

int16_t readADC(int ai){
    adc_init();
    int adc;
    int gpio;
    
    switch(ai){
        case 1:
            gpio = 26;
            adc = 0;
            break;
        case 2:
            gpio = 27;
            adc = 1;
            break;
        case 3:
            gpio = 28;
            adc = 2;
            break;
        case 4:
            gpio = 29;
            adc = 3;
            break;   
        default:
            gpio = 26;
            adc = 0;            
    }
    adc_gpio_init(gpio);
    adc_select_input(adc);
    return adc_read();
}

float readADCraw(int adc){     
    const float conversionFactor = 3.3f / (1 << 12);
    return (float)readADC(adc) * conversionFactor;
}

uint32_t pwm_set_freq_duty(uint slice_num, uint chan, uint32_t f, double d){

    uint32_t clock = 125000000;
    uint32_t divider16 = clock / f / 4096 + 
                            (clock % (f * 4096) != 0);
    if (divider16 / 16 == 0)
    divider16 = 16;
    uint32_t wrap = clock * 16 / divider16 / f - 1;
    pwm_set_clkdiv_int_frac(slice_num, divider16/16,
                                        divider16 & 0xF);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, (uint16_t)((double)wrap * d ));
    return wrap;
}

// void analogOutput(int doPort, double value){
    
//     // Tell GPIO 2 and 3 they are allocated to the PWM
//     gpio_set_function(2, GPIO_FUNC_PWM);
//     gpio_set_function(3, GPIO_FUNC_PWM);

//     // Find out which PWM slice is connected to GPIO 2 (it's slice 1)
//     uint slice_num = pwm_gpio_to_slice_num(2);

//     // Set wrap
//     pwm_set_wrap(slice_num, 64000);
//     // Set channel A output high for one cycle before dropping
//     pwm_set_chan_level(slice_num, PWM_CHAN_A, 1);
//     // Set initial B output high for three cycles before dropping
//     pwm_set_chan_level(slice_num, PWM_CHAN_B, 3);
//     // Set the PWM running
//     pwm_set_enabled(slice_num, true);
//     /// \end::setup_pwm[]
// }

void analogOutput(int doPort, double value){
    TRACE_PRINTF("Port %d, analog value: %03f\n", doPort, value);
    int gpio = -1;
     switch(doPort){
        case 2:
            gpio = 2; // 2 and 3 are on same slice (1)
            break;
        case 3:
            gpio = 3;
            break;
        case 4:
            gpio = 4; // 4 and 5 are on same slice (2)
            break;
        case 5:
        default:
            gpio = 5;            
    }  

    if (value > 3.3) // max value
        value = 3.3;
    double duty_cycle = (double)value/(double)3.3;

    // indicate we are using PWM on this GPIO
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    TRACE_PRINTF("Setting analog value on gpio: %d\n", gpio);
    // get the slice number
    uint slice_num = pwm_gpio_to_slice_num (gpio); 
    // get the channel
    uint channel = pwm_gpio_to_channel (gpio);
    // set the frequency and duty cycle
    uint32_t wrap = pwm_set_freq_duty(slice_num, channel, 1176, duty_cycle);
    // start the PWM
    pwm_set_enabled (slice_num, true); 	
    TRACE_PRINTF("Wrap: %d, duty cycle: %03f\n", wrap, duty_cycle);
}


//enum  gpio_irq_level { GPIO_IRQ_LEVEL_LOW = 0x1u , GPIO_IRQ_LEVEL_HIGH = 0x2u , GPIO_IRQ_EDGE_FALL = 0x4u , GPIO_IRQ_EDGE_RISE = 0x8u };

void digital_input_callback(uint gpio, uint32_t event) {
    printf("Event for GPIO: %d Event: event %d\n", gpio, event);
    if ((event & GPIO_IRQ_EDGE_FALL) && (event & GPIO_IRQ_EDGE_RISE)){
        printf("Rise and fall both detected\n");
        event &= ~GPIO_IRQ_EDGE_RISE; // if we get both events just pass the rise (if base state = high)
    }
    wsMessage wsMsg = { 0, GPIO_EVENT, gpio, event};
    if (sendWSMessageFromISR(wsMsg)){
        printf("Queueing wsMessage: %d, %d for GPIO %d, event %d\n", wsMsg.ulMessageID, wsMsg.ulEventId, wsMsg.ulDescriptor, event);
    }
    else{
        printf("Failed to queue wsMessage (websocket blocking?): %d, %d for GPIO %d, event %d\n", wsMsg.ulMessageID, wsMsg.ulEventId, wsMsg.ulDescriptor, event);
    }
}

void configure_digital_input(int gpio, bool high_low){    
    gpio_set_function(gpio, GPIO_FUNC_SIO);
    gpio_set_dir(gpio, GPIO_IN);
    if(high_low)
        gpio_pull_up(gpio);
    else
        gpio_pull_down(gpio);  
    gpio_set_irq_enabled_with_callback(gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &digital_input_callback); 
    gpio_set_input_enabled(gpio, true);     
}

void digital_input_init(){

    configure_digital_input(DI1, true);
    configure_digital_input(DI2, true);
    configure_digital_input(DI3, true);
    configure_digital_input(DI4, true);
}

void hardware_init(){
    digital_input_init();
}


