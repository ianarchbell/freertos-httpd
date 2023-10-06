#ifndef ROUTER_H
#define ROUTER_H

typedef struct NameFunction_struct
{
    const char* routeName;
    void (*routeFunction)(void*, char*, int);
}
NameFunction;

/* Choose 'C' for Celsius or 'F' for Fahrenheit. */
#define TEMPERATURE_UNITS 'F'

NameFunction* isRoute(const char* name);
void route(NameFunction* ptr, char* buffer, int count);

#endif /* ROUTER_H */



