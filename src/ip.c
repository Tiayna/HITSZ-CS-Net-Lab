#include "net.h"
#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 * @param src_mac 源mac地址
 */
void ip_in(buf_t *buf, uint8_t *src_mac)
{
    // TO-DO
    if(buf->len<sizeof(ip_hdr_t)) return;    //数据包长度小于报头长度，丢弃
    ip_hdr_t* ip_hdr=(ip_hdr_t*)buf->data;   //捕获报头
    if(ip_hdr->version!=IP_VERSION_4) return;  //版本号检查
    if(ip_hdr->total_len16>swap16(buf->len)) return;  //总长度字段检查(总长度应该小于等于接收到的数据包长度)
    if(ip_hdr->hdr_len!=5) return;           //报头长度检查
    uint16_t hdr_checksum16_temp=ip_hdr->hdr_checksum16;   //备份
    ip_hdr->hdr_checksum16=0;
    if(hdr_checksum16_temp != checksum16((uint16_t*)buf->data,sizeof(ip_hdr_t))) return;  //前后校验和不一致，则丢弃
    ip_hdr->hdr_checksum16=hdr_checksum16_temp;   //否则恢复原来的
    if(memcmp(net_if_ip,ip_hdr->dst_ip,NET_IP_LEN)!=0) return;   //检查目的IP地址是否为本机IP
    if(buf->len>swap16(ip_hdr->total_len16))   //数据包长度大于总长度，说明数据包有填充字段
        buf_remove_padding(buf,buf->len - swap16(ip_hdr->total_len16));
    buf_remove_header(buf,sizeof(ip_hdr_t));    //移除IP报头
    if(net_in(buf,ip_hdr->protocol,ip_hdr->src_ip)==-1)   //协议类型无法识别，则调用返回ICMP协议不可达信息
    {
        buf_add_header(buf,sizeof(ip_hdr_t));    //报头加回去，交由ICMP处理
        icmp_unreachable(buf,ip_hdr->src_ip,ICMP_CODE_PROTOCOL_UNREACH);
    }
}

/**
 * @brief 处理一个要发送的ip分片
 * 
 * @param buf 要发送的分片
 * @param ip 目标ip地址
 * @param protocol 上层协议
 * @param id 数据包id
 * @param offset 分片offset，必须被8整除
 * @param mf 分片mf标志，是否有下一个分片
 */
void ip_fragment_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol, int id, uint16_t offset, int mf)
{
    // TO-DO
    buf_add_header(buf,sizeof(ip_hdr_t));   //添加IP报头
    ip_hdr_t* ip_hdr=(ip_hdr_t*) buf->data;
    ip_hdr->version=IP_VERSION_4;  //IP协议版本
    ip_hdr->id16=swap16(id);   //标识id号
    ip_hdr->hdr_len=5; //首部长度
    ip_hdr->tos=0;     //区分服务
    ip_hdr->total_len16=swap16(buf->len);  //总长度
    ip_hdr->flags_fragment16=swap16((mf<<13) | offset);   //标志mf与分段
    ip_hdr->ttl=IP_DEFALUT_TTL;   //生存周期
    ip_hdr->protocol=protocol;    //协议
    memcpy(ip_hdr->dst_ip,ip,NET_IP_LEN);
    memcpy(ip_hdr->src_ip,net_if_ip,NET_IP_LEN);
    ip_hdr->hdr_checksum16=0;
    ip_hdr->hdr_checksum16=checksum16((uint16_t*)ip_hdr,sizeof(ip_hdr_t));
    arp_out(buf,ip);
}

/**
 * @brief 处理一个要发送的ip数据包
 * 
 * @param buf 要处理的包
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
void ip_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol)
{
    // TO-DO
    static uint16_t id=0;   //数据包id  （数据包全局id）
    uint16_t len_sum=0;    //记录已分片数据总长
    static buf_t temp;
    buf_t* ip_buf=&temp;   //用于分片发包
    if(buf->len<=1480)   //上层传来的数据报长度小于MTU-报头，直接发送
    {
        ip_fragment_out(buf,ip,protocol,id,0,0);   //无分片无偏移
        id++;
        return;
    }
    else   //否则需要分片
    {
        while(buf->len>1480)   //截取分片，直至最后一个分片小于或等于MTU-ip报头
        {
            buf_init(ip_buf,1480);
            memcpy(ip_buf->data,buf->data,1480);
            ip_fragment_out(ip_buf,ip,protocol,id,len_sum/IP_HDR_OFFSET_PER_BYTE,1);   //设置IP分片数据包id，片偏移，mf=1表示不是最后一个
            buf_remove_header(buf,1480);
            len_sum+=1480;
        }
        if(buf->len>0)   //处理最后一个分片
        {
            buf_init(ip_buf,buf->len);   //大小等于该分片大小
            memcpy(ip_buf->data,buf->data,buf->len);
            ip_fragment_out(ip_buf,ip,protocol,id,len_sum/IP_HDR_OFFSET_PER_BYTE,0);
            id++;
        }
    }
}

/**
 * @brief 初始化ip协议
 * 
 */
void ip_init()
{
    net_add_protocol(NET_PROTOCOL_IP, ip_in);
}
