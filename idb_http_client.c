/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/http_client.h"

#include "lwip/netif.h"

#include "FreeRTOS.h"
#include "task.h"

// Using this url as we know the root cert won't change for a long time
#define HOST "fw-download-alias1.raspberrypi.com"
#define URL_REQUEST "/net_install/boot.sig"

// This is the PUBLIC root certificate exported from a browser
// Note that the newlines are needed
#define TLS_ROOT_CERT_OK "-----BEGIN CERTIFICATE-----\n\
MIIC+jCCAn+gAwIBAgICEAAwCgYIKoZIzj0EAwIwgbcxCzAJBgNVBAYTAkdCMRAw\n\
DgYDVQQIDAdFbmdsYW5kMRIwEAYDVQQHDAlDYW1icmlkZ2UxHTAbBgNVBAoMFFJh\n\
c3BiZXJyeSBQSSBMaW1pdGVkMRwwGgYDVQQLDBNSYXNwYmVycnkgUEkgRUNDIENB\n\
MR0wGwYDVQQDDBRSYXNwYmVycnkgUEkgUm9vdCBDQTEmMCQGCSqGSIb3DQEJARYX\n\
c3VwcG9ydEByYXNwYmVycnlwaS5jb20wIBcNMjExMjA5MTEzMjU1WhgPMjA3MTEx\n\
MjcxMTMyNTVaMIGrMQswCQYDVQQGEwJHQjEQMA4GA1UECAwHRW5nbGFuZDEdMBsG\n\
A1UECgwUUmFzcGJlcnJ5IFBJIExpbWl0ZWQxHDAaBgNVBAsME1Jhc3BiZXJyeSBQ\n\
SSBFQ0MgQ0ExJTAjBgNVBAMMHFJhc3BiZXJyeSBQSSBJbnRlcm1lZGlhdGUgQ0Ex\n\
JjAkBgkqhkiG9w0BCQEWF3N1cHBvcnRAcmFzcGJlcnJ5cGkuY29tMHYwEAYHKoZI\n\
zj0CAQYFK4EEACIDYgAEcN9K6Cpv+od3w6yKOnec4EbyHCBzF+X2ldjorc0b2Pq0\n\
N+ZvyFHkhFZSgk2qvemsVEWIoPz+K4JSCpgPstz1fEV6WzgjYKfYI71ghELl5TeC\n\
byoPY+ee3VZwF1PTy0cco2YwZDAdBgNVHQ4EFgQUJ6YzIqFh4rhQEbmCnEbWmHEo\n\
XAUwHwYDVR0jBBgwFoAUIIAVCSiDPXut23NK39LGIyAA7NAwEgYDVR0TAQH/BAgw\n\
BgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwCgYIKoZIzj0EAwIDaQAwZgIxAJYM+wIM\n\
PC3wSPqJ1byJKA6D+ZyjKR1aORbiDQVEpDNWRKiQ5QapLg8wbcED0MrRKQIxAKUT\n\
v8TJkb/8jC/oBVTmczKlPMkciN+uiaZSXahgYKyYhvKTatCTZb+geSIhc0w/2w==\n\
-----END CERTIFICATE-----\n"

void* (*_callback)(void*);

// Print body to stdout
int http_client_cb(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err) {
    //printf("\ncontent err %d\n", err);
    _callback(p->payload);
    return ERR_OK;
}

int getHTTPClientResponseSecure(char* host, char* url_request, void* callback){

    static const uint8_t cert_ok[] = TLS_ROOT_CERT_OK; // watch out for stack usage!
    //void *state = http_client_init_secure(HOST, http_client_header_print_cb, http_client_cb, NULL, cert_ok, sizeof(cert_ok));
    void *state = http_client_init_secure(HOST, NULL, http_client_cb, NULL, cert_ok, sizeof(cert_ok));
    int pass = http_client_run_sync(state, cyw43_arch_async_context(), URL_REQUEST);
    http_client_deinit(state);

    // if (pass != 0) {
    //     panic("test failed");
    // }
    // printf("Test passed");
}

int getHTTPClientResponse(char* host, char* url_request, void* callback){

    _callback = callback;

    //static const uint8_t cert_ok[] = TLS_ROOT_CERT_OK; // watch out for stack usage!
    //void *state = http_client_init_basic(host, http_client_header_print_cb, http_client_body_print_cb, NULL); // http;
    void *state = http_client_init_basic(host, NULL, http_client_cb, NULL);
    int pass = http_client_run_sync(state, cyw43_arch_async_context(), url_request);
    http_client_deinit(state);

    // if (pass != 0) {
    //     panic("test failed");
    //}
    //printf("Test passed");
}


