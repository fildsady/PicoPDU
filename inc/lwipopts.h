#pragma once

// lwIP options for Pico W httpd + CGI + SSI
// ref: pico-sdk/lib/lwip and pico-examples/pico_w/wifi/http_server

#define NO_SYS                          0
#define LWIP_SOCKET                     0
#define MEM_LIBC_MALLOC                 0
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        4000
#define MEMP_NUM_TCP_SEG                32
#define PBUF_POOL_SIZE                  24
#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_ICMP                       1
#define LWIP_RAW                        1
#define TCP_MSS                         1460
#define TCP_WND                         (8 * TCP_MSS)
#define TCP_SND_BUF                     (8 * TCP_MSS)
#define TCP_SND_QUEUELEN                ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1
#define LWIP_NETIF_HOSTNAME             1
#define LWIP_NETCONN                    0
#define MEM_STATS                       0
#define SYS_STATS                       0
#define MEMP_STATS                      0
#define LINK_STATS                      0
#define LWIP_CHKSUM_ALGORITHM           3
#define LWIP_DHCP                       1
#define LWIP_IPV4                       1
#define LWIP_TCP                        1
#define LWIP_UDP                        1
#define LWIP_DNS                        1
#define LWIP_TCP_KEEPALIVE              1
#define LWIP_NETIF_TX_SINGLE_PBUF       1
#define DHCP_DOES_ARP_CHECK             0
#define LWIP_DHCP_DOES_ACD_CHECK        0

// httpd + CGI + SSI
#define LWIP_HTTPD_CGI                  1
#define LWIP_HTTPD_SSI                  1
#define LWIP_HTTPD_SSI_INCLUDE_TAG      0
#define HTTPD_USE_CUSTOM_FSDATA         0

// FreeRTOS sys_arch — required by pico_cyw43_arch_lwip_sys_freertos
#define LWIP_FREERTOS_SYS_ARCH_TIMEOUT_USES_TICKS 1
#define TCPIP_THREAD_STACKSIZE          1024  // stack for lwIP tcpip_thread (words)
#define TCPIP_THREAD_PRIO               8     // higher than app tasks (1-2)
#define DEFAULT_THREAD_STACKSIZE        256
#define DEFAULT_THREAD_PRIO             1
