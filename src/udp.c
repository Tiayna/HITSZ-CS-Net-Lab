#include "udp.h"
#include "ip.h"
#include "icmp.h"

/**
 * @brief udp处理程序表
 * 
 */
map_t udp_table;

/**
 * @brief udp伪校验和计算
 * 
 * @param buf 要计算的包
 * @param src_ip 源ip地址
 * @param dst_ip 目的ip地址
 * @return uint16_t 伪校验和
 */
static uint16_t udp_checksum(buf_t *buf, uint8_t *src_ip, uint8_t *dst_ip)
{
    // TO-DO
    //buf传进来时，buf->data指向的是UDP报头，但UDP报头前面有一段IP报文，需暂存
    udp_hdr_t* udp_hdr=(udp_hdr_t*)buf->data;     //udp报头
    buf_t temp;   //临时缓存区暂存IP报头
    buf_t* ip_buf=&temp;      //暂存原因：在计算校验和引入伪头部时，原本的IP报头数据会被伪头部覆盖
    buf_add_header(buf,sizeof(udp_peso_hdr_t));   //此时buf->data位置为伪头部报头
    buf_init(ip_buf,sizeof(ip_hdr_t));     //临时缓存区初始化存储IP报头
    memcpy(ip_buf->data,(uint8_t*)udp_hdr-sizeof(ip_hdr_t),sizeof(ip_hdr_t));   //将UDP报头前面的IP报头拷贝到ip_buf
    ip_hdr_t* ip_hdr=(ip_hdr_t*)ip_buf->data;    //捕获IP报头
    udp_peso_hdr_t* udp_peso_hdr=(udp_peso_hdr_t*)buf->data;   //捕获伪头部报头
    udp_peso_hdr->placeholder=0;
    udp_peso_hdr->protocol=ip_hdr->protocol;   //协议号
    memcpy(udp_peso_hdr->src_ip,ip_hdr->src_ip,NET_IP_LEN);   //源IP
    memcpy(udp_peso_hdr->dst_ip,ip_hdr->dst_ip,NET_IP_LEN);   //目的IP
    udp_peso_hdr->total_len16=udp_hdr->total_len16;   //长度直接拷贝UDP报头中的长度字段
    uint16_t checksum=checksum16((uint16_t*)buf->data,buf->len);
    memcpy((uint8_t*)udp_hdr-sizeof(ip_hdr_t),ip_buf->data,sizeof(ip_hdr_t));   //将暂存的IP头部拷贝回来（覆盖掉伪头部）
    buf_remove_header(buf,sizeof(udp_peso_hdr_t));   //去掉UDP伪头部（本质上即buf->data位置从伪头部移回udp报头）
    return checksum;
}

/**
 * @brief 处理一个收到的udp数据包
 * 
 * @param buf 要处理的包
 * @param src_ip 源ip地址
 */
void udp_in(buf_t *buf, uint8_t *src_ip)
{
    // TO-DO
    if(buf->len<sizeof(udp_hdr_t)) return;   //包长度检查
    udp_hdr_t* udp_hdr=(udp_hdr_t*)buf->data;
    if(buf->len<swap16(udp_hdr->total_len16)) return;   //包长度检查
    uint16_t checksum_temp=udp_hdr->checksum16;   //保存校验和字段
    udp_hdr->checksum16=0;    //填充为0
    if(checksum_temp!=udp_checksum(buf,src_ip,net_if_ip)) return;   //前后校验和不同，则丢弃
    udp_hdr->checksum16=checksum_temp;
    uint16_t key=swap16(udp_hdr->dst_port16);
    udp_handler_t* handler = map_get(&udp_table,&key);    //从udp_table中查找，返回处理函数指针
    if(handler==NULL)   //没有找到相应的处理函数，发送端口不可达的ICMP差错报文
    {
        buf_add_header(buf,sizeof(ip_hdr_t));
        icmp_unreachable(buf,src_ip,ICMP_CODE_PORT_UNREACH);
        return;
    }
    else    //找到，则去掉UDP报头并用处理函数进行相应处理
    {
        buf_remove_header(buf,sizeof(udp_hdr_t));
        (*handler)(buf->data,buf->len,src_ip,swap16(udp_hdr->src_port16));
    }
}

/**
 * @brief 处理一个要发送的数据包
 * 
 * @param buf 要处理的包
 * @param src_port 源端口号
 * @param dst_ip 目的ip地址
 * @param dst_port 目的端口号
 */
void udp_out(buf_t *buf, uint16_t src_port, uint8_t *dst_ip, uint16_t dst_port)
{
    // TO-DO
    //UDP报头填充
    buf_add_header(buf,sizeof(udp_hdr_t));
    udp_hdr_t* udp_hdr=(udp_hdr_t*)buf->data;   
    //网络流中是大端序，x86中是小端序
    udp_hdr->src_port16=swap16(src_port);
    udp_hdr->dst_port16=swap16(dst_port);
    udp_hdr->total_len16=swap16(buf->len);
    udp_hdr->checksum16=0;  //校验和字段先填充为0
    //IP报头填充
    buf_add_header(buf,sizeof(ip_hdr_t));
    ip_hdr_t* ip_hdr=(ip_hdr_t*)buf->data;   
    ip_hdr->protocol=NET_PROTOCOL_UDP;
    memcpy(ip_hdr->src_ip,net_if_ip,NET_IP_LEN);
    memcpy(ip_hdr->dst_ip,dst_ip,NET_IP_LEN);
    buf_remove_header(buf,sizeof(ip_hdr_t));
    udp_hdr->checksum16=udp_checksum(buf,net_if_ip,dst_ip);   //调用UDP校验和，进行伪头部处理
    ip_out(buf,dst_ip,NET_PROTOCOL_UDP);   //发送
}

/**
 * @brief 初始化udp协议
 * 
 */
void udp_init()
{
    map_init(&udp_table, sizeof(uint16_t), sizeof(udp_handler_t), 0, 0, NULL);
    net_add_protocol(NET_PROTOCOL_UDP, udp_in);
}

/**
 * @brief 打开一个udp端口并注册处理程序
 * 
 * @param port 端口号
 * @param handler 处理程序
 * @return int 成功为0，失败为-1
 */
int udp_open(uint16_t port, udp_handler_t handler)
{
    return map_set(&udp_table, &port, &handler);
}

/**
 * @brief 关闭一个udp端口
 * 
 * @param port 端口号
 */
void udp_close(uint16_t port)
{
    map_delete(&udp_table, &port);
}

/**
 * @brief 发送一个udp包
 * 
 * @param data 要发送的数据
 * @param len 数据长度
 * @param src_port 源端口号
 * @param dst_ip 目的ip地址
 * @param dst_port 目的端口号
 */
void udp_send(uint8_t *data, uint16_t len, uint16_t src_port, uint8_t *dst_ip, uint16_t dst_port)
{
    buf_init(&txbuf, len);
    memcpy(txbuf.data, data, len);
    udp_out(&txbuf, src_port, dst_ip, dst_port);
}