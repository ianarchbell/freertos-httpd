cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

project(freertos-httpd C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_BOARD pico_w)

message("FreeRTOS kernel path: ${FREERTOS_KERNEL_PATH}")

pico_sdk_init()

add_executable(picow_freertos_httpd_nosys
        picow_freertos_httpd.c
        )
target_compile_definitions(picow_freertos_httpd_nosys PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_freertos_httpd_nosys PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
        ${PICO_LWIP_CONTRIB_PATH}/apps/httpd
        )
target_link_libraries(picow_freertos_httpd_nosys
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_http
        pico_stdlib
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
pico_add_extra_outputs(picow_freertos_httpd_nosys)

add_executable(picow_freertos_httpd_sys
        picow_freertos_httpd.c
        )
target_compile_definitions(picow_freertos_httpd_sys PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        LWIP_SOCKET=1       # we need the socket API (generally this would be in your lwipopts.h)
        )
target_include_directories(picow_freertos_httpd_sys PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
        ${PICO_LWIP_CONTRIB_PATH}/apps/httpd
        )
target_link_libraries(picow_freertos_httpd_sys
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_http
        pico_stdlib
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
pico_add_extra_outputs(picow_freertos_httpd_sys)
