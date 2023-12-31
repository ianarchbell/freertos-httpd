#ifndef IDB_WEBSOCKET_H
#define IDB_WEBSOCKET_h

#include <FreeRTOS.h>

typedef struct wsMessage
{
   uint32_t ulMessageID;
   uint32_t ulEventId;
   char*    message;
} wsMessage;

void websocket_init();
BaseType_t sendWSMessage(wsMessage wsMsg);
BaseType_t sendWSMessageFromISR(wsMessage wsMsg);

#endif /* IDB_WEBSOCKET_H */