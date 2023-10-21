# freertos-httpd

## Aim

This project was driven by the need for a simple webserver application running on FreeRTOS that supports 
SD Card file access as well as JSON support for background fetch to enable Single Page Applications. 

The aim was not to reinvent the wheel and as far as possible use the LWIP distributed HTTPD application. However, this implementation only supported "packed files", that is converted and stored in the application itself. This was believed to be overly restrictive and with the availability of FreeRTOS LabProject FAT filesystem for SD Cards and in particular the already working port of that to the Raspberry Pi Pico by CarlK.

This is not intended to be production code - there is no security. It enables a simple demonstration of serving files located on an sd card and json from a Pico. The web demo (dashboard.html) shows real-time update of the Pico core temperature with a background fetch as well as the last 10 updates rom a log file. You'll need to update the date in the route in idb_router.c (/readlog/) to pull the right date data. It also shows how to turn the led and on and off and get the state using routing/json. It does support both GET and POST, so while not supporting PUT or DELETE you can use it in a RESTfull way.

The router table is a simple function table. This is an example:

    NameFunction routes[] =
    { 
        { "/temp", (void*) *returnTemperature, HTTP_GET },
        { "/temperature", (void*) *returnTemperature, HTTP_GET },
        { "/led", (void*) *returnLED, HTTP_GET }, 
        { "/led/:value", (void*) *setLED, HTTP_POST }, 
        { "/gpio/:gpio", (void*) *gpio, HTTP_GET },  
        { "/gpio/:gpio/:value", (void*) *gpio, HTTP_POST }, 
        { "/readlog/:date", (void*) *readLogWithDate, HTTP_GET },
        { "/failure", (void*) *failure, HTTP_GET || HTTP_POST},
        { "/success", (void*) *success, HTTP_GET || HTTP_POST},
    };

Note that the routing table uses :... variables in a similar way to express, but it's a very simple implementation without wildcards or regex etc. The POST implementation expects a urlencoded body. That body is used to create a route similar to the ones above, then that route is internally dispatched. 

## Libraries

FreeRTOS FAT base library: https://github.com/FreeRTOS/Lab-Project-FreeRTOS-FAT
CarlK's SD Card on the Pico project, including SD Card drivers for the RP2040: https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico

## Implementation of the LWIP HTTPD on FreeRTOS with SD Card support

The HTTPD app is extensible and I have made use of those extensions to provide:

1. SD card file access (HTML, CSS, image etc. etc.) using a third-party library
2. Extended routing capability (see router.c)
3. Dynamic JSON support for get/set 
4. GET and POST support for SD card files
5. Support for routing

There is also an example data logger that runs as a separate task. This logs the core temperature of the RP2040.  

The extensions are in two files:

1. fs_ext.c - provides sd card support - currently SPI (based on the FreeRTOS Lab Projects wrapped and with drivers provided by CarlK) because of working with FreeRTOS
2. idb_router.c - handles routes - such as providing json where there is not necessarily file access

fs_ext.c sd card access is provided by:

1. fs_open_custom
2. fs_read_custom
3. fs_close_custom

It also calls idb_router.c to find and handle other routes (specifically for JSON but could be used for other purposes).

## Configuring LWIP

There are no changes to HTTP.c or fs.c. The extension to fs_ext.c are enabled by adding the following options
to lwipopts.h:

    #define LWIP_HTTPD_CUSTOM_FILES 1 // Needed to for fs_open_custom, fs_read_custom and fs_close_custom to be called
    #define LWIP_HTTPD_DYNAMIC_FILE_READ 1 // Required for multi-block support (fs_read_custom not colled unless this is set)
    #define LWIP_HTTPD_FILE_EXTENSION 1 // Needed to have the extra field in the file control block to hold our own pointer
    #define LWIP_HTTPD_DYNAMIC_HEADERS 0 // Ensure they are off as we provide our own headers

The latest version does not use the supplied HTTPD capabiity of providing headers based on a filetype. This meant that the routing table could not be RESTfull. Now headers are supplied with the data requested as required.   

## hw_config.h

You will need to change hw_config.h to configure your SD card to use the FreeRTOS-FAT-CLI-for-RPi-Pico library. Theis library is specific to the Raspberry Pi Pico but there are many other platforms that Lab-Project-FreeRTOS-FAT supports (but will require a low level driver for the platform). 

## Limitations

This is demo code and provided on that basis. The most useful aspect of this work is the provision of a custom file system enabling SD Cards and secondly the example of how to integrate routing. Routing is rudimentary but you could take it from here. 

1. Now supports HTTP GET and POST, but not PUT or DELETE
2. As currently designed json responses can only be one block in length - you could change this in fs_ext.c read_custom/idb_router.c

## Prerequisites

Hardware

1. Pico W
2. Board with Pico socket and SD Card. I'm using Pico Expansion Plus S1 (https://www.amazon.com/XICOOLEE-Expansion-Expander-Raspberry-External/dp/B0BBVJHQCJ) or Cytron Maker (https://www.digikey.com/en/products/detail/cytron-technologies-sdn-bhd/MAKER-PI-RP2040/14557836), but most others will work in SPI mode.

Software Installed

1. PICO-SDK (with LWIP stack and HTTP app) from Github
2. FreeRTOS-Kernel from Github
3. ARM toolchain: arm-none-eabi (see: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
4. CMake
5. make

## Usage

Using this project (will need a Pico W or other networking access):

1. git clone --recurse-submodules https://github.com/ianarchbell/freertos-httpd.git
2. cd freertos-httpd
3. mkdir build
4. cd build
5. export PICO_SDK_PATH=/{your-location}/pico-sdk
6. export FREERTOS_KERNEL_PATH=/{your-location}/FreeRTOS-Kernel
7. export WIFI_SSID={your-ssid}
8. export WIFI_PASSWORD={your-password}
9. cmake ..
10. make
11. copy index.html to an sd card
12. copy the UF2 file to the Pico W
13. http://{ip address}


