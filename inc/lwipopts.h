#pragma once

// Minimal lwipopts.h for CYW43 + FreeRTOS on Pico 2W
#define NO_SYS                          0
#define LWIP_SOCKET                     0
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        4000
#define PBUF_POOL_SIZE                  24
#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_ICMP                       1
#define LWIP_RAW                        1
#define TCP_MSS                         1460
#define TCP_WND                         (8 * TCP_MSS)
#define TCP_SND_BUF                     (8 * TCP_MSS)
#define MEMP_NUM_TCP_SEG                TCP_SND_QUEUELEN
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

// FreeRTOS sys_arch
#define LWIP_FREERTOS_SYS_ARCH_TIMEOUT_USES_TICKS 1
#define TCPIP_THREAD_STACKSIZE          2048
#define TCPIP_THREAD_PRIO               6
#define DEFAULT_THREAD_STACKSIZE        256
#define DEFAULT_THREAD_PRIO             1
