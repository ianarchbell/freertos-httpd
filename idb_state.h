#include <FreeRTOS.h>

#define DIGITAL_VALUE 0x01
#define ANALOG_VALUE 0x02

typedef struct stateItem
{
    char descriptor[5];
    uint8_t gpio;
    union state_value {
        uint32_t int_value;
        float    float_value;
    } state_value;
} stateItem;

typedef struct stateMessage
{
   uint32_t         ulMessageID;
   uint32_t         ulMessageType;
   char             descriptor[4];
   union val {
        uint32_t    int_value;
        float       float_value;
   } val;
} stateMessage;

void set_first_state();
void state_init();
void initialize_state();
void print_state();
void save_state();
