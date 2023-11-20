cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

set(PICO_LWIP_APP_INCLUDE_PATH "${PICO_SDK_PATH}/lib/lwip/src/include/lwip/apps")
set(PICO_LWIP_APP_PATH "${PICO_SDK_PATH}/lib/lwip/src/apps")
set(PICO_LWIP_CONTRIB_APP_INCLUDE_PATH "${PICO_SDK_PATH}/lib/lwip/contrib/")

set(FATPATH "${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-FAT-CLI-for-RPi-Pico")

set(MININI_PATH "${CMAKE_CURRENT_LIST_DIR}/minini")

message("LWIP app path: " ${PICO_LWIP_APP_INCLUDE_PATH})

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(idb C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_BOARD pico_w)

message("FreeRTOS kernel path: " ${FREERTOS_KERNEL_PATH})

pico_sdk_init()

set(FAT_PATH FreeRTOS-FAT-CLI-for-RPi-Pico)
message("FAT path: " ${FAT_PATH})

add_subdirectory(${FAT_PATH}/FreeRTOS+FAT+CLI ./build)

add_compile_options( # -Wimplicit #check we have headers
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        -O0
        )        

if (NOT DEFINED ENV{WIFI_SSID})
	message(FATAL_ERROR "WIFI_SSID Environment variable not defined")
endif()
if (NOT DEFINED ENV{WIFI_PASSWORD})
	message(FATAL_ERROR "WIFI_PASSWORD Environment variable not defined")
endif()

add_executable(idb
        idb_main.c fs_ext.c hw_config.c idb_router.c idb_ntp_client.c idb_data_logger.c
                idb_hardware.c idb_post.c idb_ini.c idb_network.c idb_websocket.c idb_http.c
                idb_runtime_stats.c 
                idb_ping.c
                #idb_http_client.c 
                #${PICO_LWIP_CONTRIB_PATH}/apps/ping/ping.c
                ${PICO_LWIP_APP_PATH}/http/strcasestr.c
                ${CMAKE_CURRENT_LIST_DIR}/tiny-json/tiny-json.c
                ${MININI_PATH}/dev/minini.c
        )
        
target_compile_definitions(idb PRIVATE
        WIFI_SSID=\"$ENV{WIFI_SSID}\"
        WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        LWIP_SOCKET=1       # we need the socket API (generally this would be in your lwipopts.h)
        PING_USE_SOCKETS=1
        )
target_include_directories(idb PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/tiny-json
        ${PICO_SDK_PATH}/include    
        ${FAT_PATH}/FreeRTOS+FAT+CLI/Lab-Project-FreeRTOS-FAT/include
        ${FAT_PATH}/FreeRTOS+FAT+CLI/portable/RP2040
        ${FAT_PATH}/FreeRTOS+FAT+CLI/include
        ${PICO_LWIP_APP_INCLUDE_PATH}
        ${FREERTOS_KERNEL_PATH}/include
        ${FREERTOS_PATH}/portable/GCC/ARM_CM0
        ${CMAKE_CURRENT_LIST_DIR} 
        ${PICO_LWIP_CONTRIB_PATH}/apps/ping
        ${MININI_PATH}/dev
        )       
target_link_libraries(idb        
        pico_stdlib
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_http
        hardware_adc
        hardware_pwm
        FreeRTOS+FAT+CLI
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap   
        pico_lwip_iperf
        pico_mbedtls
        #pico_http_client
        ) 
          
# Ignore warnings from lwip code
set_source_files_properties(
        ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls.c
        PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-result"
        )                

pico_add_extra_outputs(idb)

pico_enable_stdio_usb(idb 0)
pico_enable_stdio_uart(idb 1)

