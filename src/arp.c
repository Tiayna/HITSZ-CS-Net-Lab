#include <string.h>
#include <stdio.h>
#include "net.h"
#include "arp.h"
#include "ethernet.h"
/**
 * @brief 初始的arp包
 * 
 */
static const arp_pkt_t arp_init_pkt = {    //初始化的arp包，内容包括硬件类型与协议类型，源地址
    .hw_type16 = swap16(ARP_HW_ETHER),
    .pro_type16 = swap16(NET_PROTOCOL_IP),
    .hw_len = NET_MAC_LEN,
    .pro_len = NET_IP_LEN,
    .sender_ip = NET_IF_IP,
    .sender_mac = NET_IF_MAC,
    .target_mac = {0}};

/**
 * @brief arp地址转换表，<ip,mac>的容器
 * 
 */
map_t arp_table;

/**
 * @brief arp buffer，<ip,buf_t>的容器
 * 
 */
map_t arp_buf;

/**
 * @brief 打印一条arp表项
 * 
 * @param ip 表项的ip地址
 * @param mac 表项的mac地址
 * @param timestamp 表项的更新时间
 */
void arp_entry_print(void *ip, void *mac, time_t *timestamp)
{
    printf("%s | %s | %s\n", iptos(ip), mactos(mac), timetos(*timestamp));
}

/**
 * @brief 打印整个arp表
 * 
 */
void arp_print()
{
    printf("===ARP TABLE BEGIN===\n");
    map_foreach(&arp_table, arp_entry_print);
    printf("===ARP TABLE  END ===\n");
}

/**
 * @brief 发送一个arp请求
 * 
 * @param target_ip 想要知道的目标的ip地址
 */
void arp_req(uint8_t *target_ip)
{
    // TO-DO
    buf_init(&txbuf,sizeof(arp_pkt_t));   //不是很懂此处设置数据报长度为arp报头大小（数据报长度到底表示什么？）
    arp_pkt_t arp_pkt=arp_init_pkt;   //拷贝初始化arp包头
    arp_pkt.opcode16=swap16(ARP_REQUEST);  //操作类型设置为请求
    memcpy(arp_pkt.target_ip,target_ip,NET_IP_LEN);   //设置arp目的ip
    memcpy(txbuf.data,&arp_pkt,sizeof(arp_pkt_t));    //将arp数据报拷贝到txbuf中
    uint8_t dest_mac[6]={0xff,0xff,0xff,0xff,0xff,0xff};  //目标mac地址为广播地址，无回报ARP广播包
    ethernet_out(&txbuf,dest_mac,NET_PROTOCOL_ARP);   //发送txbuf到广播地址
}

/**
 * @brief 发送一个arp响应
 * 
 * @param target_ip 目标ip地址
 * @param target_mac 目标mac地址
 */
void arp_resp(uint8_t *target_ip, uint8_t *target_mac)
{
    // TO-DO
    buf_init(&txbuf,sizeof(arp_pkt_t));
    arp_pkt_t arp_pkt=arp_init_pkt;
    arp_pkt.opcode16=swap16(ARP_REPLY);   //操作类型设为响应
    memcpy(arp_pkt.target_ip,target_ip,NET_IP_LEN);
    memcpy(arp_pkt.target_mac,target_mac,NET_MAC_LEN);
    memcpy(txbuf.data,&arp_pkt,sizeof(arp_pkt_t));    //拷贝arp报文到txbuf中
    ethernet_out(&txbuf,target_mac,NET_PROTOCOL_ARP);    //发送ARP协议类型的数据报
}

/**
 * @brief 处理一个收到的ARP数据包
 * 
 * @param buf 要处理的数据包
 * @param src_mac 源mac地址
 */
void arp_in(buf_t *buf, uint8_t *src_mac)
{
    // TO-DO
    if(buf->len<sizeof(arp_pkt_t)) return;  //丢弃不完整的数据包
    arp_pkt_t* arp_pkt=(arp_pkt_t*)buf->data;   //捕获报头,并且进行报头检查
    if(arp_pkt->hw_type16!=swap16(ARP_HW_ETHER)) return;
    if(arp_pkt->pro_type16!=swap16(NET_PROTOCOL_IP)) return;
    if(arp_pkt->hw_len!=NET_MAC_LEN) return;
    if(arp_pkt->pro_len!=NET_IP_LEN) return;
    if(arp_pkt->opcode16!=swap16(ARP_REPLY)&&arp_pkt->opcode16!=swap16(ARP_REQUEST)) return;

    map_set(&arp_table,arp_pkt->sender_ip,src_mac);   //设置接收来源的ARP表项<ip,mac>
    buf_t* buf_p=(buf_t*)map_get(&arp_buf,arp_pkt->sender_ip);    //查找该接收的ARP报文的IP表项是否存在于map_buf
    if(buf_p!=NULL)   //如果存在，则说明之前arp_out时在ARP表中没有相应的MAC地址，进行了arp_req，现在收到了相应IP地址的主机响应
    {
        ethernet_out(buf_p,src_mac,NET_PROTOCOL_IP);  //发送到响应的IP地址对应的mac地址中（注意此时不再是ARP报文，而是IP报文）
        map_delete(&arp_buf,arp_pkt->sender_ip);      //从arp_buf中移除
    }
    else   //如果不存在的话
    {
        if(arp_pkt->opcode16==swap16(ARP_REQUEST) && !memcmp(arp_pkt->target_ip,net_if_ip,NET_IP_LEN))   //判断是否为请求本主机IP的报文
        {
            arp_resp(arp_pkt->sender_ip,arp_pkt->sender_mac);   //进行响应，告知本机IP地址对应的mac地址
        }
    }
}

/**
 * @brief 处理一个要发送的数据包
 * 
 * @param buf 要处理的数据包
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
void arp_out(buf_t *buf, uint8_t *ip)
{
    // TO-DO
    uint8_t* mac = map_get(&arp_table,ip);  //检查arp表中是否有缓存目标ip地址的mac地址
    if(mac==NULL)   //未缓存则检查arp_buf中是否有包
    {
        if(map_get(&arp_buf,ip)==NULL)   //有包则不能发送arp请求（正在等待响应），无包则设置arp_buf并广播请求
        {
            map_set(&arp_buf,ip,buf);
            arp_req(ip);   //广播目的地址的，仅目的IP的主机会响应
        }
    }
    else   //已缓存则直接发送ip报文
    {
        ethernet_out(buf,mac,NET_PROTOCOL_IP);
    }

}

/**
 * @brief 初始化arp协议
 * 
 */
void arp_init()
{
    map_init(&arp_table, NET_IP_LEN, NET_MAC_LEN, 0, ARP_TIMEOUT_SEC, NULL);  //初始化arp_table，并设置超时时间
    map_init(&arp_buf, NET_IP_LEN, sizeof(buf_t), 0, ARP_MIN_INTERVAL, buf_copy);  //初始化用于缓存来自IP层的数据包，并设置超时时间
    net_add_protocol(NET_PROTOCOL_ARP, arp_in);  //增加键值对<NET_PROTOCOL_ARP,arp_in>
    arp_req(net_if_ip);    //告知局域网中其他主机、路由器等本机的IP地址（广播自己的IP，没有任何响应）
}