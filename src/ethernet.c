#include "ethernet.h"
#include "utils.h"
#include "driver.h"
#include "arp.h"
#include "ip.h"
/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 */
void ethernet_in(buf_t *buf)
{
    // TO-DO
    if(buf->len<sizeof(ether_hdr_t))
        return;
    uint16_t* protocol_p;
    uint16_t protocol;   //协议类型
    uint8_t mac_address[6];    //记录包来源mac地址，6字节
    //接收的数据帧包头结构：buf->data指向头部，分别为：6字节目的mac地址、6字节源mac地址、2字节协议类型
    for(int i=0;i<6;i++)
    {
        mac_address[i]=*(buf->data+6+i);
    }
    protocol_p=(uint16_t*)(buf->data+12);
    protocol=swap16(*protocol_p);
    buf_remove_header(buf,sizeof(ether_hdr_t));
    net_in(buf,protocol,mac_address);
}
/**
 * @brief 处理一个要发送的数据包
 * 
 * @param buf 要处理的数据包
 * @param mac 目标MAC地址
 * @param protocol 上层协议
 */
void ethernet_out(buf_t *buf, const uint8_t *mac, net_protocol_t protocol)
{
    // TO-DO
    if(buf->len<46)  //判断数据长度，如果不足46则显式填充0
    {
        buf_add_padding(buf,46-buf->len);
    }
    buf_add_header(buf,sizeof(ether_hdr_t));
    ether_hdr_t* hdr=(ether_hdr_t*)buf->data;
    memcpy(hdr->dst,mac,NET_MAC_LEN);   //填写目的MAC地址
    memcpy(hdr->src,net_if_mac,NET_MAC_LEN);   //填写源MAC地址
    hdr->protocol16=swap16(protocol);  //填写协议类型（需要大小端转换）
    driver_send(buf);  //调用驱动层封装发包
}
/**
 * @brief 初始化以太网协议
 * 
 */
void ethernet_init()
{
    buf_init(&rxbuf, ETHERNET_MAX_TRANSPORT_UNIT + sizeof(ether_hdr_t));
}

/**
 * @brief 一次以太网轮询
 * 
 */
void ethernet_poll()
{
    if (driver_recv(&rxbuf) > 0)
        ethernet_in(&rxbuf);
}
