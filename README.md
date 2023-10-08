# freertos-httpd

## Implementation of the LWIP HTTPD on FreeRTOS with SD Card support

This project was driven by the need for a simple webserver application running on FreeRTOS that supports 
SD Card file access as well as JSON support for background fetch to enable Single Page Applications. 

The aim was not to reinvent the wheel and as far as possible use the LWIP distributed HTTPD application. However, this implementation only supported "packed files", that is converted and stored in the application itself. This was believed to be overly restrictive and with the availability of ChaN's FAT filesystem for SD Cards and in particular the already working port of that to the Raspberry Pi Pico.

The HTTPD app is extensible and I have made use of those extensions to provide:

1. SD card file access (HTML, CSS, image etc. etc.) using a third-party library
2. Extended routing capability (see router.c)
3. Dynamic JSON support for get/set 

The extensions are in two files:

1. fs_ext.c
2. router.c


fs_ext.c handles:

1. fs_open_custom
2. fs_read_custom
3. fs_close_custom

It also calls router.c to find and handle other routes (specifically for JSON but could be used for other purposes).

There are no changes to HTTP.c or fs.c. The extension to fs_ext.c are enabled by adding the following options
to lwipopts.h:

    #define LWIP_HTTPD_CUSTOM_FILES 1
    #define LWIP_HTTPD_DYNAMIC_FILE_READ 1
    #define LWIP_HTTPD_FILE_EXTENSION 1
    #define LWIP_HTTPD_DYNAMIC_HEADERS 1

You will need to change hw_config.h to configure your SD card to use the no-OS-FATFS-SD-SDIO-SPI-RP-Pico library.  Theis library is specific to the Raspberry Pi Pico but there are many other platforms that ChaN 's FatFS library supports.


Notes

1. Modifed Carl's FreeRTOSConfig.h for SMP core affinity
2. Can't locate "fs.h" header - why?
