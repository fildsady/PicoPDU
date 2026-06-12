#pragma once
// lwipopts.h for FreeRTOS + CYW43 (sys_freertos mode, NO_SYS=0)
#define NO_SYS                          0
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0
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

// Required for NO_SYS=0 (FreeRTOS) mode — all default to 0 which panics
#define TCPIP_MBOX_SIZE                 16
#define TCPIP_THREAD_STACKSIZE          512
#define TCPIP_THREAD_PRIO               5
#define DEFAULT_TCP_RECVMBOX_SIZE       8
#define DEFAULT_UDP_RECVMBOX_SIZE       8
#define DEFAULT_RAW_RECVMBOX_SIZE       8
#define DEFAULT_ACCEPTMBOX_SIZE         8
