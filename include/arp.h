#ifndef ARP_H
#define ARP_H

#include "net.h"

#define ARP_HW_ETHER 0x1 // 以太网
#define ARP_REQUEST 0x1  // ARP请求包
#define ARP_REPLY 0x2    // ARP响应包

#pragma pack(1)
typedef struct arp_pkt
{
    uint16_t hw_type16;              // 硬件类型，表示报文可以在哪种类型的网络传输
    uint16_t pro_type16;             // 协议类型，表示要映射的协议地址类型
    uint8_t hw_len;                  // 硬件地址长，硬件：MAC地址长度
    uint8_t pro_len;                 // 协议地址长，协议：IP地址长度
    uint16_t opcode16;               // 请求/响应，操作类型，即本次ARP报文类型
    uint8_t sender_mac[NET_MAC_LEN]; // 发送包硬件地址，源MAC地址
    uint8_t sender_ip[NET_IP_LEN];   // 发送包协议地址，源IP地址
    uint8_t target_mac[NET_MAC_LEN]; // 接收方硬件地址，目的MAC地址
    uint8_t target_ip[NET_IP_LEN];   // 接收方协议地址，目的IP地址
} arp_pkt_t;

#pragma pack()

void arp_init();  
void arp_print();
void arp_in(buf_t *buf, uint8_t *src_mac);
void arp_out(buf_t *buf, uint8_t *ip);
void arp_req(uint8_t *target_ip);
void arp_resp(uint8_t *target_ip, uint8_t *target_mac);
#endif