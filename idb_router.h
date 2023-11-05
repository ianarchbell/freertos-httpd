#ifndef ROUTER_H
#define ROUTER_H

#define HTTP_GET  0x01
#define HTTP_POST 0x02

typedef struct NameFunction_struct
{
    const char* routeName;
    void (*routeFunction)(void*, char*, int, char*);
    int routeType;
    char* uri;
}
NameFunction;

/* Choose 'C' for Celsius or 'F' for Fahrenheit. */
#define TEMPERATURE_UNITS 'F'

NameFunction* isRoute(const char* name, int routeType);
void route(NameFunction* ptr, char* buffer, int count, char*);

// each one more than required for terminating null
#define DATETIMELEN 23
#define TEMPLEN 6
#define MAXREADINGS 10
#define MAXLEN 54

typedef struct Measurement{
    char date_time[DATETIMELEN];
    char temperature[TEMPLEN];
} Measurement;

#endif /* ROUTER_H */



