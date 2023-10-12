# freertos-httpd

## Aim

This project was driven by the need for a simple webserver application running on FreeRTOS that supports 
SD Card file access as well as JSON support for background fetch to enable Single Page Applications. 

The aim was not to reinvent the wheel and as far as possible use the LWIP distributed HTTPD application. However, this implementation only supported "packed files", that is converted and stored in the application itself. This was believed to be overly restrictive and with the availability of ChaN's FAT filesystem for SD Cards and in particular the already working port of that to the Raspberry Pi Pico.

## Libraries

FreeRTOS FAT base library: https://github.com/FreeRTOS/Lab-Project-FreeRTOS-FAT
CarlK's SD Card on the Pico project, including SD Card drivers for the RP2040: https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico

## Implementation of the LWIP HTTPD on FreeRTOS with SD Card support

The HTTPD app is extensible and I have made use of those extensions to provide:

1. SD card file access (HTML, CSS, image etc. etc.) using a third-party library
2. Extended routing capability (see router.c)
3. Dynamic JSON support for get/set 

The extensions are in two files:

1. fs_ext.c - provides sd card support - currently SPI (based on the FreeRTOS Lab Projects wrapped and with drivers provided by CarlK) because of working with FreeRTOS
2. router.c - handles routes - such as providing json where there is not necessarily file access

fs_ext.c sd card access is provided by:

1. fs_open_custom
2. fs_read_custom
3. fs_close_custom

It also calls router.c to find and handle other routes (specifically for JSON but could be used for other purposes).

## Configuring LWIP

There are no changes to HTTP.c or fs.c. The extension to fs_ext.c are enabled by adding the following options
to lwipopts.h:

    #define LWIP_HTTPD_CUSTOM_FILES 1
    #define LWIP_HTTPD_DYNAMIC_FILE_READ 1
    #define LWIP_HTTPD_FILE_EXTENSION 1
    #define LWIP_HTTPD_DYNAMIC_HEADERS 1

## hw_config.h

You will need to change hw_config.h to configure your SD card to use the FreeRTOS-FAT-CLI-for-RPi-Pico library. Theis library is specific to the Raspberry Pi Pico but there are many other platforms that Lab-Project-FreeRTOS-FAT supports (but will require a low level driver for the platform).

## Limitations

This is demo code and provided on that basis. The most useful aspect of this work is the provision of a custom file system enabling SD Cards and secondly the example of how to integrate routing. Routing is rudimentary but you could take it from here. If you need HTTP POST etc you will need another HTTPD.

A port of the ESP32 web server (both HTTP and HTTPS) to the Pico may be the way to go...

1. Only supports HTTP get
2. As currently designed json responses can only be one block in length - easily changeable in fs_ext.c read_custom
3. Routes do not allow variables in them

## Usage

Using this project (will need a Pico W or other networking access):

1. git clone: --submodules --recursive https:
2. mkdir build
3. cd build
4. cmake ..
5. make
6. copy dashboard.html to an sd card
6. copy the UF2 file to the Pico W
7. http://{ip address}/dashboard.html


