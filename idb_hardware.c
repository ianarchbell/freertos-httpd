#include <string.h>

#include "idb_config.h"
#include <pico/types.h>
#include <hardware/adc.h>
#include <hardware/pwm.h>

#include "idb_websocket.h"
#include "idb_state.h"
#include "idb_hardware.h"

float getCoreTemperature(char units){

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

int16_t readADC(char* descriptor){
    adc_init();

    int gpio = getGPIOFromDescriptor(descriptor);
    int adc = getADCFromDescriptor(descriptor);

    adc_gpio_init(gpio);
    adc_select_input(adc);
    return adc_read();
}

float readADCraw(char* ai){     
    const float conversionFactor = 3.3f / (1 << 12);
    return (float)readADC(ai) * conversionFactor;
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

void analogOutput(char* descriptor, float value){

    stateMessage stateMsg;

    strncpy(stateMsg.descriptor, descriptor, sizeof stateMsg.descriptor); 
    stateMsg.val.float_value = value;
    stateMsg.ulMessageType = STATE_ANALOG_OUTPUT;
    sendStateMessage(stateMsg);
    TRACE_PRINTF("Sent state message for %s, state: %d, value: %.08f", stateMsg.descriptor, stateMsg.ulMessageType, stateMsg.val.float_value);
    TRACE_PRINTF("Port %s, analog value: %.08f\n", descriptor, value);
    
    int gpio = getGPIOFromDescriptor(descriptor); 

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
    TRACE_PRINTF("Event for GPIO: %d Event: event %d\n", gpio, event);
    if ((event & GPIO_IRQ_EDGE_FALL) && (event & GPIO_IRQ_EDGE_RISE)){
        TRACE_PRINTF("GPIO: rise and fall events both detected\n");
        event &= ~GPIO_IRQ_EDGE_RISE; // if we get both events just pass the fall (if base state = high)
    }
    char* descriptor = getDescriptorFromGPIO(gpio);

    int n = snprintf(messageBuff, MAX_BUFF, "\"descriptor\" : \"%s\", \"event\" : %d", descriptor, event);

    // need to check we haven't already got something on queue - as there is only one buffer TODO ***

    wsMessage wsMsg = { 0, GPIO_EVENT, messageBuff};
    if (sendWSMessageFromISR(wsMsg)){
        TRACE_PRINTF("Queueing wsMessage: %d, %d for descriptor %s, event %d\n", wsMsg.ulMessageID, wsMsg.ulEventId, descriptor, event);
    }
    else{
        TRACE_PRINTF("Failed to queue wsMessage (websocket blocking?): %d, %d for descriptor %s, event %d\n", wsMsg.ulMessageID, wsMsg.ulEventId, descriptor, event);
    }
}

void configure_digital_input(char* descriptor, bool high_low){  
    int gpio = getGPIOFromDescriptor(descriptor);  
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

    configure_digital_input("DI01", true);
    configure_digital_input("DI02", true);
    configure_digital_input("DI03", true);
}

void hardware_init(){
    digital_input_init();
}

void reflect_state(){
    int count = getStatesCount();
    stateItem* states = getStates();
    for(int i=0 ; i < count; i++){
        if(states[i].flags == STATE_ANALOG_OUTPUT){
            analogOutput(states[i].descriptor, states[i].state_value.float_value);
        }
        if(states[i].flags == STATE_DIGITAL_OUTPUT){
            digitalOutput(states[i].descriptor, states[i].state_value.int_value);           
        }
    }
}

void digitalOutput(char* descriptor, int gpio_value){
    int gpio_pin = getGPIOFromDescriptor(descriptor);
    gpio_init(gpio_pin);
    gpio_set_dir(gpio_pin, GPIO_OUT);
    gpio_put(gpio_pin, gpio_value);

    stateMessage stateMsg;

    strncpy(stateMsg.descriptor, descriptor, sizeof stateMsg.descriptor); 
    stateMsg.val.int_value = gpio_value;
    stateMsg.ulMessageType = STATE_DIGITAL_OUTPUT;
    sendStateMessage(stateMsg);
    TRACE_PRINTF("Sent state message for %s, state: %d, value: %d", stateMsg.descriptor, stateMsg.ulMessageType, stateMsg.val.int_value);
    TRACE_PRINTF("Port %s, value: %d\n", descriptor, gpio_value);
}
