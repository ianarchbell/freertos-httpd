#ifndef ROUTER_H
#define ROUTER_H

#define HTTP_GET  0x01
#define HTTP_POST 0x02

typedef struct NameFunction_struct
{
    const char* routeName;
    void (*routeFunction)(void*, char*, int);
    int routeType;
    char* uri;
}
NameFunction;

/* Choose 'C' for Celsius or 'F' for Fahrenheit. */
#define TEMPERATURE_UNITS 'F'

NameFunction* isRoute(const char* name, int routeType);
void route(NameFunction* ptr, char* buffer, int count);

#endif /* ROUTER_H */



