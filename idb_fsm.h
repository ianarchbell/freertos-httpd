typedef struct EventMessage
{
   uint32_t         ulMessageID;
   uint32_t         ulMessageType;
   char             descriptor[5];
   union val {
        uint32_t    int_value;
        float       float_value;
   } val;
} EventMessage;

void set_initial_state();
void fsm_init();
void fsm_deinit();
void print_current_state();