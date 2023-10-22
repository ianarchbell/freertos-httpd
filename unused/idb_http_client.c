/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "http_client.h"
#include "pico/stdio.h"
#include "pico/cyw43_arch.h"
#include "pico/async_context.h"

#define HOST "worldtimeapi.org"
#define URL_REQUEST "/api/ip"

// #define HOST "http://airspectrum.cdnstream1.com:8114"
// #define URL_REQUEST "/1648_128"

void http_client_header_print_cb(){

}

void http_client_body_print_cb(){
    
}

int getHTTPClient(char* host, char* url) {

    void *state = http_client_init_basic(HOST, http_client_header_print_cb, http_client_body_print_cb, NULL); // http
    int result1 = http_client_run_sync(state, cyw43_arch_async_context(), URL_REQUEST);
    //result1 += http_client_run_sync(state, cyw43_arch_async_context(), URL_REQUEST); // repeat
    http_client_deinit(state);

    // state = http_client_init_secure(HOST, http_client_header_print_cb, http_client_body_print_cb, NULL, NULL, 0); // https
    // int result2 = http_client_run_sync(state, cyw43_arch_async_context(), URL_REQUEST);
    // result2 += http_client_run_sync(state, cyw43_arch_async_context(), URL_REQUEST); // repeat
    // http_client_deinit(state);

    //if (result1 != 0 || result2 != 0) {
    if (result1 != 0) {
        panic("test failed");
    }
    cyw43_arch_deinit();
    printf("Test passed\n");
    sleep_ms(100);
    return 0;
}