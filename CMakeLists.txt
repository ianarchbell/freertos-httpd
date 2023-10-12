cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

set(PICO_LWIP_APP_INCLUDE_PATH "/Users/Ian/pico/pico-sdk/lib/lwip/src/include/lwip/apps")
#set(PICO_LWIP_CONTRIB_PATH        /Users/Ian/pico/pico-sdk/lib/lwip/src/)

set(FATPATH "/Users/Ian/pico/freertos-httpd/FreeRTOS-FAT-CLI-for-RPi-Pico")

message("LWIP app path: " ${PICO_LWIP_APP_INCLUDE_PATH})
#message("LWIP http app include path: " ${PICO_LWIP_CONTRIB_PATH})

project(test C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_BOARD pico_w)

message("FreeRTOS kernel path: " ${FREERTOS_KERNEL_PATH})

pico_sdk_init()

set(FAT_PATH FreeRTOS-FAT-CLI-for-RPi-Pico)
message("FAT path: " ${FAT_PATH})

add_subdirectory(${FAT_PATH}/FreeRTOS+FAT+CLI ./build)

# seems to stop the initial check of the compiler and avoids failure
add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

if (NOT DEFINED ENV{WIFI_SSID})
	message(FATAL_ERROR "WIFI_SSID Environment variable not defined")
endif()
if (NOT DEFINED ENV{WIFI_PASSWORD})
	message(FATAL_ERROR "WIFI_PASSWORD Environment variable not defined")
endif()

add_executable(picow_freertos_httpd_nosys
        picow_freertos_httpd.c fs_ext.c hw_config.c router.c picow_ntp_client.c data_logger.c adc.c
        )
target_compile_definitions(picow_freertos_httpd_nosys PRIVATE
    WIFI_SSID=\"$ENV{WIFI_SSID}\"
    WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\"
    CYW43_HOST_NAME="oddWires"        
    )
    
target_include_directories(picow_freertos_httpd_nosys PRIVATE
        ${CMAKE_CURRENT_LIST_DIR} 
        ${FAT_PATH}/FreeRTOS+FAT+CLI/Lab-Project-FreeRTOS-FAT/include
        ${FAT_PATH}/FreeRTOS+FAT+CLI/portable/RP2040
        ${FAT_PATH}/FreeRTOS+FAT+CLI/include
        ${PICO_LWIP_APP_INCLUDE_PATH}
        ${FREERTOS_KERNEL_PATH}/include
        ${FREERTOS_PATH}/portable/GCC/ARM_CM0
        ${CMAKE_CURRENT_LIST_DIR} 
         )
target_link_libraries(picow_freertos_httpd_nosys
        pico_stdlib
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_http
        hardware_adc
        FreeRTOS+FAT+CLI
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
pico_add_extra_outputs(picow_freertos_httpd_nosys)

add_executable(picow_freertos_httpd_sys
        picow_freertos_httpd.c fs_ext.c hw_config.c router.c picow_ntp_client.c data_logger.c adc.c
        )
target_compile_definitions(picow_freertos_httpd_sys PRIVATE
        WIFI_SSID=\"$ENV{WIFI_SSID}\"
        WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        LWIP_SOCKET=1       # we need the socket API (generally this would be in your lwipopts.h)
        )
target_include_directories(picow_freertos_httpd_sys PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}     
        ${FAT_PATH}/FreeRTOS+FAT+CLI/Lab-Project-FreeRTOS-FAT/include
        ${FAT_PATH}/FreeRTOS+FAT+CLI/portable/RP2040
        ${FAT_PATH}/FreeRTOS+FAT+CLI/include
        ${PICO_LWIP_APP_INCLUDE_PATH}
        ${FREERTOS_KERNEL_PATH}/include
        ${FREERTOS_PATH}/portable/GCC/ARM_CM0
        ${CMAKE_CURRENT_LIST_DIR} 
         )       
target_link_libraries(picow_freertos_httpd_sys        
        pico_stdlib
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_http
        hardware_adc
        FreeRTOS+FAT+CLI
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap   
        )           

pico_add_extra_outputs(picow_freertos_httpd_sys)

pico_enable_stdio_usb(picow_freertos_httpd_sys 1)
pico_enable_stdio_uart(picow_freertos_httpd_sys 0)
pico_enable_stdio_usb(picow_freertos_httpd_nosys 1)
pico_enable_stdio_uart(picow_freertos_httpd_nosys 0)
