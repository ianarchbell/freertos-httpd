#include <FreeRTOS.h>

#define STATE_DIGITAL 0x01
#define STATE_ANALOG 0x02

typedef struct stateItem
{
    char descriptor[5];
    uint8_t flags;
    uint8_t gpio;
    uint8_t adc;
    union state_value {
        uint32_t int_value;
        float    float_value;
    } state_value;
} stateItem;

typedef struct stateMessage
{
   uint32_t         ulMessageID;
   uint32_t         ulMessageType;
   char             descriptor[5];
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
BaseType_t sendStateMessage(stateMessage stateMsg);
int getGPIOFromDescriptor(char* descriptor);
int getADCFromDescriptor(char* descriptor);
char* getDescriptorFromGPIO(int gpio);
float getStateItemFloat(char* descriptor);
int getStateItemInt(char* descriptor);
stateItem* getStates();
int getStatesCount();
void save_state_analog(char* descriptor);
void save_state_digital(char* descriptor);
