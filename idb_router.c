/**
 * Handle uri routing for both GET and POST 
*/

#include <stdio.h>
#include <string.h>

#include <hardware/adc.h>
#include <stdlib.h>
#include "pico/cyw43_arch.h"

#include "ff_headers.h"
#include "ff_utils.h"
#include "ff_stdio.h"

#include "idb_router.h"
#include "idb_hardware.h"
#include "idb_config.h"

#define DEVICENAME "sd0"
#define MOUNTPOINT "/sd0"

/* Choose 'C' for Celsius or 'F' for Fahrenheit. */
#define TEMPERATURE_UNITS 'F'

/**
 * 
 * Calls getCoreTemperature in adc.c to get the core temperature
 * 
*/
float read_onboard_temperature(const char unit) {

    float temp = getCoreTemperature(unit);
    if (unit == 'C') {
        return temp;
    } else if (unit == 'F') {
        return temp * 9 / 5 + 32;
    }
    return -1.0f;
}

/**
 * 
 * Prints the HTTP headers for all the json responses
 * 
*/
int printJSONHeaders(char* buffer, int count){

    strcpy(buffer, "HTTP/1.1 200 OK\n");
    strcat(buffer, "Content-Type: application/json\n");
    sprintf(buffer+strlen(buffer), "Content-Length: %d\n\n", count);
    //TRACE_PRINTF(buffer);
    return strlen(buffer);
}

/**
 * 
 * Outputs the json for the GET temperature response
 * 
*/
void returnTemperature(NameFunction* ptr, char* buffer, int count){

    if (buffer){
        char buf[64];
        float temperature = getCoreTemperature(TEMPERATURE_UNITS);
        TRACE_PRINTF("Onboard temperature =  %.3g %c\n", (double)temperature, TEMPERATURE_UNITS);
    
        // Output JSON very simply
        int len = sprintf(buf, "{\"temperature\": %.3g,\"temperature units\": \"%c\"}", (double)temperature, TEMPERATURE_UNITS);
        if (len > count){
            panic("buffer too small for temperature\n");
        }
        printJSONHeaders(buffer, len);
        strcat(buffer, buf);
    }
}

/**
 * 
 * Getting the value to be set = 0 or 1
 * 
*/
int getSetValue(const char* uri){

    char buff[64]; 
    strncpy(buff, uri, sizeof(buff));
    buff[sizeof(buff)-1] = '\0';  // Ensure null if truncated
    strtok(buff,"/"); // find the first /
    const char* subString = strtok(NULL,"/");       // find the second "/""
    if (subString == NULL)
        return -1;
    else{
        //TRACE_PRINTF("Got LED uri value %s", subString2);
        return atoi(subString);
    }
}

/**
 * 
 * Returns the currrent value of the LED in json format
 * 
*/
void returnLED(NameFunction* ptr, char* buffer, int count){
    //TRACE_PRINTF("Getting LED value");

    int led = cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN);
    if (buffer){
        char buff[64];
        // Output JSON very simply
        int len = sprintf(buff, "{\"led\": %d}", led);
        printJSONHeaders(buffer, len);
        if(strlen(buffer) + len < count){
            strcat(buffer, buff);
        }
        else{
            panic("buffer too small for returnLED\n");
        }
    }
}

/**
 * 
 * Sets the value of the lED and returns json success
 * 
*/
void setLED(NameFunction* ptr, char* buffer, int count, const char* uri){

    int value = getSetValue(uri);
    //TRACE_PRINTF("Setting LED value %d", value);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, value);
    // if (buffer){ 
    //     char buff[64];   
    //     int len = sprintf(buff, "%s", "{\"success\" : true}");
    //     printJSONHeaders(buffer, len); // print out headers with no body
    //     if(strlen(buffer) + len < count){
    //         strcat(buffer, buff);
    //     }
    //     else{
    //         panic("buffer too small for returnLED\n");
    //     }
    // }
}

/**
 * 
 * Sets the value of a GPIO and returns json success
 * 
*/
void setGPIO(NameFunction* ptr, char* buffer, int count, int gpio_pin, int gpio_value){
    gpio_init(gpio_pin);
    gpio_set_dir(gpio_pin, GPIO_OUT);
    gpio_put(gpio_pin, gpio_value);
    // if (buffer){
    //     char buff[64];       
    //     int len = sprintf(buff, "%s", "{\"success\" : true}");
    //     printJSONHeaders(buffer, len); // print out headers 
    //     if((strlen(buffer) + len) < count){
    //         strcat(buffer, buff);
    //     }
    //     else{
    //         panic("buffer too small for setGPIO\n");
    //     }
    // }
}

/**
 * 
 * Returns the number of the GPIO to the caller
 * 
*/
int getGPIO(const char* uri){

    char buff[64]; 
    strncpy(buff, uri, sizeof(buff));
    buff[sizeof(buff)-1] = '\0';  // Ensure null if truncated
    strtok(buff,"/"); // find the first /
    const char* subString = strtok(NULL,"/");       // find the second /
    if (subString == NULL)
        return -1;
    else{
        //TRACE_PRINTF("Got GPIO number %s\n", subString2);
        return atoi(subString);
    }
}

/**
 * 
 * Returns the value of the GPIO to the caller
 * 
*/
int getGPIOValue(const char* uri){

    char buff[64]; 
    strncpy(buff, uri, sizeof(buff));
    buff[sizeof(buff)-1] = '\0';  // Ensure null if truncated
    strtok(buff,"/"); // find the first /
    const char* subString = strtok(NULL,"/");       // find the second /
    if (subString == NULL)
        return -1;
    const char* subString2 = strtok(NULL,"/");       // find the second /
    if (subString2 == NULL)
        return -1;
    else{    
        //TRACE_PRINTF("Got GPIO value %s\n", subString3);
        return atoi(subString2);
    }
}

/**
 * 
 * Returns the value of the GPIO in json
 * 
*/
void returnGPIO(NameFunction* ptr, char* buffer, int count, int gpio_pin){
    
    int gpio_value = (int)gpio_get(gpio_pin);

    if (buffer){
        char buff[64];
        // Output JSON very simply
        //TRACE_PRINTF("Returning GPIO status %d", gpio_pin);
        int len = snprintf(buff, sizeof buff, "{\"%d\" : %d}", gpio_pin, gpio_value);
        printJSONHeaders(buffer, len);
        if(strlen(buffer) + len < count){
            strcat(buffer, buff);
        }
        else{
            panic("buffer too small for returnLED\n");
        }
    }
}

/**
 * 
 * Handles gpio - either get or set TODO - this is allowing GET for a POST route 
 * 
*/
void gpio(NameFunction* ptr, char* buffer, int count, const char* uri){
    int GPIOpin = getGPIO(uri);
    int value = getGPIOValue(uri);
    if(value != -1){
        //TRACE_PRINTF("Setting GPIO value %d, %d\n", GPIOpin, value);
        setGPIO(ptr, buffer, count, GPIOpin, value);
    }
    else{
        //TRACE_PRINTF("Returning GPIO value %d\n", GPIOpin);
        returnGPIO(ptr, buffer, count, GPIOpin);
    }
}

/**
 * 
 * Returns the value of all the GPIOs in json
 * 
*/
void returnGPIOs(NameFunction* ptr, char* buffer, int count){
    if (buffer){   
        char buf[512]; 
        strcpy(buf, "[");
        int offset = 1;
        int gpio_values[27]; 
        for(int i = 0; i < 27; i ++){       
            gpio_values[i] = (int)gpio_get(i);
            int len = snprintf(buf + offset, sizeof buf, "{\"%d\" : %d}", i, gpio_values[i]);
            offset += len;
        }
        strcat(buf, "]");
        offset += 1;
        
        printJSONHeaders(buffer, offset);

        if(strlen(buffer) + offset < count){
            strcat(buffer, buf);
        }
        else{
            panic("buffer too small for returnGPIOs\n");
        }
    }
}

/**
 * 
 * Gets the next token based on a single character delimiter
 * Returns the next token in ns if found, returns empty string if not found
 * Return value is a pointer to the delimiter
*/
char* getNextValue(char* ns, char* s, char delim){   
    char* p1 = s;

    char* p2 = strchr(p1, ',');
    if (p2 != NULL){
        if(p2-p1 > 0){
            memcpy(ns, p1, p2-p1);
            ns[p2-p1] = '\0';
            TRACE_PRINTF("Found value: %s\n", ns);
        }
        else{
            ns[0] = '\0';
            TRACE_PRINTF("Empty value: %s\n", ns);
        }  
    }           
    return p2;    
}

/**
 * 
 * Returns the state of all the GPIOs in an array
 * 
*/
int extractGPIOValues(int values[27], const char* uri){

    printf("Extracting GPIO values: %s\n", uri);
    char buff[128]; 
    char value[16];

    strncpy(buff, uri, sizeof(buff));
    buff[sizeof(buff)-1] = '\0';  // Ensure null if truncated
    TRACE_PRINTF("buff: %s\n", buff);
    char* subString = strtok(buff,"/");       // find the first /
    if (subString == NULL)
        return -1;
    char* subString2 = strtok(NULL,"/");       // find the /
    TRACE_PRINTF("subString2: %s\n", subString2);
    if (subString2 == NULL)
        return -1;
    int i = 0;
    TRACE_PRINTF("subString2: %s\n", subString2);
    subString2 = getNextValue((char*)&value, subString2, ',');
    do  {  
        TRACE_PRINTF("Value from getNextValue: %s\n", value);
        if(strlen(value) > 0){
            TRACE_PRINTF("Setting values[%d]: %s\n", i, value);
            values[i] = atoi(value);
        }
        else{
            TRACE_PRINTF("Ignoring value for GPIO %d\n", i);
            values[i] = -1;    
        }
        i++;
        subString2++; 
        subString2 = getNextValue((char*)&value, subString2, ',');
    } while (strlen(subString2) > 0);
    return 0;
}

/**
 * 
 * Handles gpio array /gpioarray/0,1,1,0... from 0-26
 * Leave out the values you want unchanged
 * 
*/
void gpioArray(NameFunction* ptr, char* buffer, int count, const char* uri){
    #define GPIOMAX 27
    int values[GPIOMAX];

    for (int i = 0; i < GPIOMAX ; i++){  // default to ignore
        values[i] = -1; 
    } 

    int ret = extractGPIOValues((int*)&values, uri);
    if (ret == -1){
        TRACE_PRINTF("Failed to extract values\n");
    }

    for(int i = 0; i < 27; i ++){     
       
        if (values[i] != -1){
            TRACE_PRINTF("Setting GPIO %d to %d\n", i, values[i]);
             setGPIO(ptr, buffer, count, i, values[i]);
        }
        else
            TRACE_PRINTF("Ignoring GPIO %d\n", i);    
    }    
}

/**
 * 
 * Helper to create a reading from a supplied buffer
 * 
*/
void getData(char *buff, Measurement* reading){  
    char* ptr = strtok(buff, ",");
    strncpy(reading->date_time, ptr, DATETIMELEN);
    ptr = strtok(NULL, "\n");
    strncpy(reading->temperature, ptr, TEMPLEN);
}

/**
 * 
 * Tokenises the date from the uri
 * 
*/
void getLogDate(const char* uri, char* buffer, int count){

    char buff[64]; 
    strncpy(buff, uri, sizeof(buff));
    buff[sizeof(buff)-1] = '\0';  // Ensure null if truncated

    strtok(buff,"/"); // find the first /
    const char* subString = strtok(NULL,"/");   // find the second /
    //subString = strtok(NULL,"/");   // find the third /
    if (strlen(subString) > count){
        panic("Buffer too small for logDate\n");
    }
    strcpy(buffer, subString);
}

/**
 * 
 * Helper to open the log file
 * 
*/
FF_FILE* openLogFile(const char* name){

    char buffer[128];

    FF_Disk_t *pxDisk = NULL;
    
    if (!mount(&pxDisk, DEVICENAME, MOUNTPOINT)){
        TRACE_PRINTF("Failed to mount %s as %s", DEVICENAME, MOUNTPOINT);
    }

    int n = snprintf(buffer, sizeof buffer, "%s/data/%s", MOUNTPOINT, name);                    
    configASSERT(0 < n && n < (int)sizeof buffer);
    // TODO - not sure this is needed here as we'll already have this directory structure
    if (-1 == mkdirhier(buffer) &&
        stdioGET_ERRNO() != pdFREERTOS_ERRNO_EEXIST) {
        TRACE_PRINTF("failed to set directory");
        return NULL;
    }
    TRACE_PRINTF("Mounted disk to read log file\n");

    snprintf(buffer + n, sizeof buffer - n, "/log_data.csv");
    TRACE_PRINTF("Opening log file: %s\n", buffer);  
    FF_FILE* px = ff_fopen(buffer, "r");
    return px;
}

/**
 * 
 * Read the log file - seeks to the end and reads no more than the last 10 records 
 * or the number of records that will fit into the buffer - whichever is less.
 * Result is provided as json
 * 
*/
void readLog(const char* name, char* jsonBuffer, int count){

    Measurement readings[MAXREADINGS];
    Measurement reading;

    //TRACE_PRINTF("Buffer length: %d, name: %s\n", count, name);

    FF_FILE *pxFile = openLogFile(name);

    // need to check if mounted?
    if (pxFile){
        char buffer[128];

        TRACE_PRINTF("Log file open\n");
        int i = 0;

        sprintf(jsonBuffer, "[");
        int len = 1;
        int bufferPos = len;

        // seek to the end of the file
        ff_fseek(pxFile, 0, FF_SEEK_END);
        int pos = ff_ftell(pxFile);
        /* Don't write each char on output.txt, just search for '\n' */

        int recCount = 0;
        // seek backwards for max number of records that will fit in the buffer
        while (pos) {
            ff_fseek(pxFile, --pos, FF_SEEK_SET);
            if (ff_fgetc(pxFile) == '\n') {
                if (recCount++ == count/MAXLEN) 
                    break;
            }
        }

        TRACE_PRINTF("Reading log file records\n");
        // now positioned at last n records
        while(ff_fgets(buffer, sizeof(buffer), pxFile)){
            if (i >= MAXREADINGS)
                break;
            getData(buffer, &reading);
            memcpy(&readings[i], &reading, sizeof(reading));
            //TRACE_PRINTF("Reading [%d] : Date: %s, Temperature: %s\n", i, readings[i].date_time, readings[i].temperature );  
            if (bufferPos < count){          
                len = snprintf(jsonBuffer+bufferPos, count-bufferPos, "{\"date\": %s, \"temperature\": %s, \"temperature units\": \"%c\"}, ", 
                                                readings[i].date_time, readings[i].temperature, 'F');
                bufferPos += len;
                i++;
            }
            else{
                break;
            }
        }
        sprintf(jsonBuffer+bufferPos-2, "]");
        ff_fclose(pxFile);  
    }
}

/**
 * 
 * Gets the log date and calls readLog to read the event log.
 * Result is provided as json
 * 
*/
void readLogWithDate(NameFunction* ptr, char* buffer, int count, const char* uri){
    #define MINBUFSIZE 64
    #define JSONBUFFSIZE 1460
    
    char* logDate = pvPortMalloc(MINBUFSIZE);
    if (logDate){
        char* buf = pvPortMalloc(JSONBUFFSIZE);  
        if (buf){
            getLogDate(uri, logDate, MINBUFSIZE);
            //TRACE_PRINTF("Log date: %s\n", logDate);
            readLog(logDate, buf, JSONBUFFSIZE-MINBUFSIZE); // should be enough for the header
            //TRACE_PRINTF("Response from readLog: %s\n", buf);
            int len = strlen(buf);
            int hdrLen = printJSONHeaders(buffer, len);
            strcat(buffer+hdrLen, buf);
            vPortFree(buf);
        }
        else{
            panic("Failed to allocate log buffer\n");
        }
        vPortFree(logDate);
     }
     else{
        panic("Failed to allocate logDate\n");
     }
}

/**
 * 
 * General purpose successs route
 * 
*/
void success(NameFunction* ptr, char* buffer, int count, char* uri){
    TRACE_PRINTF("Success\n");
    char buff[64];
    int len = sprintf(buff, "%s", "{\"success\" : true}");
    printJSONHeaders(buffer, len); // print out headers with no body
    if(strlen(buffer) + len < count){
        strcat(buffer, buff);
    }
    else{
        panic("buffer too small for success\n");
    }
}

/**
 * 
 * General purpose failure route
 * 
*/
void failure(NameFunction* ptr, char* buffer, int count, char* uri){
    TRACE_PRINTF("Failure\n");
    char buff[64];
    int len = sprintf(buff, "%s", "{\"success\" : false}");
    printJSONHeaders(buffer, len); // print out headers with no body
    if(strlen(buffer) + len < count){
        strcat(buffer, buff);
    }
    else{
        panic("buffer too small for success\n");
    }
}

/**
 * 
 * Routing table containes names routes, witha function pointer
 * Routes can be confined to GET or POST
 * The NULL pointer here is used at runtime to point to a URI
 * 
*/
NameFunction routes[] =
{ 
    { "/temp", (void*) *returnTemperature, HTTP_GET, NULL },
    { "/temperature", (void*) *returnTemperature, HTTP_GET, NULL },
    { "/led", (void*) *returnLED, HTTP_GET, NULL }, 
    { "/led/:value", (void*) *setLED, HTTP_POST, NULL }, 
    { "/gpio/:gpio", (void*) *gpio, HTTP_GET, NULL },  
    { "/gpio/:gpio/:value", (void*) *gpio, HTTP_POST, NULL },
    { "/gpioarray", (void*) *returnGPIOs, HTTP_GET, NULL },  
    { "/gpioarray/:values", (void*) *gpioArray, HTTP_POST, NULL }, 
    { "/readlog/:date", (void*) *readLogWithDate, HTTP_GET, NULL },
    { "/failure", (void*) *failure, HTTP_GET || HTTP_POST, NULL},
    { "/success", (void*) *success, HTTP_GET || HTTP_POST, NULL},
};

/**
 * 
 * Checks is there is a full match on a provided route name and type
 * 
*/
NameFunction* parseExact(const char* name, int routeType){
    //TRACE_PRINTF("Parsing exact route : %s\n", name);
    for (NameFunction* ptr = routes;ptr != routes + sizeof(routes) / sizeof(routes[0]); ptr++)
    {
        if (ptr->routeType != routeType)
            continue;
        if(!strcmp(ptr->routeName, name)) {
            return ptr;
        }
    }
    return 0;
}

/**
 * 
 * Checks is there is a partial match on a provided route name and type
 * 
*/
NameFunction* parsePartialMatch(const char* name, int routeType){

    char buff[64];
    char buff2[64];

    strncpy(buff2, name, sizeof(buff2)); // copy for strtok
    buff2[sizeof(buff2)-1] = '\0';  // Ensure null if truncated
    const char* nameTok = strtok(buff2, "/"); // start of route

    
    //TRACE_PRINTF("Parsing partial route : %s\n", name);
    for (NameFunction* ptr = routes;ptr != routes + sizeof(routes) / sizeof(routes[0]); ptr++)
    {
        if (ptr->routeType != routeType)
            continue;
        if(!strstr(ptr->routeName, ":")) // can't partial match if no variable
            continue;
        strncpy(buff, ptr->routeName, sizeof(buff)); // copy for strtok
        buff[sizeof(buff)-1] = '\0';  // Ensure null if truncated
        const char* tok = strtok(buff, "/"); // start of route
        TRACE_PRINTF("Tok: %s, name: %s\n", tok,nameTok);
        if(!strcmp(tok, nameTok)) { // first token must be an exact match
            TRACE_PRINTF("Token match: %s, routeName: %s\n", tok, nameTok);
            return ptr;
        }
    }
    return 0;
}

/**
 * 
 * Checks for both an exact or partial match on a route name
 * Returns a pointer to the NameFunction (and so the function pointer)
 * 
*/
NameFunction* isRoute(const char* name, int routeType){

    NameFunction* ptr = parseExact(name, routeType);
    if (!ptr){ // always return an exact match if found otherwise look for partial
        ptr = parsePartialMatch(name, routeType);
    }
    if (ptr)
        TRACE_PRINTF("In route table\n");
    else
        TRACE_PRINTF("Not in route table\n");    
    return ptr;  
}

/**
 * 
 * Performs the routing function using the supplied NameFunction
 * buffer, count and original URI
 * 
*/
void route(NameFunction* ptr, char* buffer, int count, char* uri){
    if (buffer && count > 0 && uri){       
        TRACE_PRINTF("Routing %s\n", uri);
        ptr->routeFunction(ptr, buffer, count, uri);
        
        if (uri){
            TRACE_PRINTF("routed uri: %s\n", uri);
            // vPortFree(ptr->uri); // no need to free it is up to the caller
            // ptr->uri = NULL;
        }
        else{
            panic("no uri isRoute must be called prior to route\n");
        }
    }
    else{
        TRACE_PRINTF("no buffer, can't route\n");
    }
    //TRACE_PRINTF("returned from route\n");
}





