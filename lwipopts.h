#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html)

#define LWIP_ALTCP                          0 // not using altcp
#define LWIP_ALTCP_TLS                      0 // not using altcp
#define LWIP_ALTCP_TLS_MBEDTLS              0 // not using altcp

// Note bug in lwip with LWIP_ALTCP and LWIP_DEBUG
// https://savannah.nongnu.org/bugs/index.php?62159
//#define LWIP_DEBUG 1
//#undef LWIP_DEBUG
//#define ALTCP_MBEDTLS_DEBUG  LWIP_DBG_ON

//under test
/*********************/
#define LWIP_HTTP_FILE_STATE                1 // enable state for each file
#define LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED   0 // see if makes a real difference to memory management

#define WS_TIMEOUT 32

/*********************/

#define LWIP_HTTPD_CUSTOM_FILES             1 // for fs_open_custom and fs_read_custom
#define LWIP_HTTPD_DYNAMIC_FILE_READ        1 // for fs_read_custom
#define LWIP_HTTPD_FILE_EXTENSION           1 // for file extension field to store our extension data
#define LWIP_HTTPD_DYNAMIC_HEADERS          0 // we send our own headers
#define LWIP_HTTPD_SUPPORT_POST             1 // required for POST implementation

#define HTTPD_MAX_RETRIES                   32 //default 4 - made high as the poll interval is so short
#define HTTPD_POLL_INTERVAL                 1 // 4 x 500ms is default
#define LWIP_HTTPD_SUPPORT_11_KEEPALIVE     1 // hopefully less overhead

#define LWIP_SO_RCVTIMEO                    1 // Needed for Ping

//#define HTTPD_DEBUG                         LWIP_DBG_ON
#define HTTPD_DEBUG                         LWIP_DBG_OFF

#if !NO_SYS
#define TCPIP_THREAD_STACKSIZE              4096 // defaults used in lwip examples
#define DEFAULT_THREAD_STACKSIZE            4096 // defaults used in lwip examples
#define DEFAULT_RAW_RECVMBOX_SIZE           8    // defaults used in lwip examples
#define TCPIP_MBOX_SIZE                     8    // defaults used in lwip examples
#define LWIP_TIMEVAL_PRIVATE                0    // defaults used in lwip examples
#define LWIP_TCPIP_CORE_LOCKING_INPUT       1    // not necessary, can be done either way - maybe?
#endif

// allow override 
#ifndef NO_SYS
#define NO_SYS                      0
#endif
// allow override 
#ifndef LWIP_SOCKET
#define LWIP_SOCKET                 1
#endif
#if PICO_CYW43_ARCH_POLL
#define MEM_LIBC_MALLOC             1
#else
// MEM_LIBC_MALLOC is incompatible with non polling versions
#define MEM_LIBC_MALLOC             0
#endif

// These have been explicitly set
#define MEM_SIZE                    16000
#define MEMP_NUM_TCP_SEG            64
#define PBUF_POOL_SIZE              24
#define TCP_WND                     (16 * TCP_MSS) // was 8
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (16 * TCP_MSS) // need to look at this - normally same as TCP_WND
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))

// These are defaults
#define MEM_ALIGNMENT               4
#define MEMP_NUM_ARP_QUEUE          10
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETCONN                0
#define LWIP_CHKSUM_ALGORITHM       3
#define LWIP_DHCP                   1
#define LWIP_IPV4                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DNS                    1
#define LWIP_TCP_KEEPALIVE          1
#define LWIP_NETIF_TX_SINGLE_PBUF   1
#define DHCP_DOES_ARP_CHECK         0
#define LWIP_DHCP_DOES_ACD_CHECK    0

// Potential speed advantage in setting it to 2 - we'll leave it at default no pad
// #define ETH_PAD_SIZE                2

// For statistics
#define MEM_STATS                   1
#define SYS_STATS                   1
#define MEMP_STATS                  1
#define LINK_STATS                  1

// Debugging
#ifndef NDEBUG
#define LWIP_DEBUG                  1
#define LWIP_STATS                  1
#define LWIP_STATS_DISPLAY          1
#endif

#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF

#endif /* __LWIPOPTS_H__ */