#include "net.h"
#include "icmp.h"
#include "ip.h"

/**
 * @brief 发送icmp响应
 * 
 * @param req_buf 收到的icmp请求包
 * @param src_ip 源ip地址
 */
static void icmp_resp(buf_t *req_buf, uint8_t *src_ip)
{
    // TO-DO
    buf_t* buf=&txbuf;   //捕获txbuf
    buf_init(buf,req_buf->len);  //初始化长度来发送icmp回显报文
    icmp_hdr_t* req_hdr=(icmp_hdr_t*)req_buf->data;
    icmp_hdr_t* icmp_hdr=(icmp_hdr_t*)buf->data;
    icmp_hdr->type=ICMP_TYPE_ECHO_REPLY;      //回显应答type=0
    icmp_hdr->code=0;    //回显应答code=0
    icmp_hdr->checksum16=0;
    icmp_hdr->id16=req_hdr->id16;   //标识符,应答报文拷贝请求报文的标识符字段
    icmp_hdr->seq16=req_hdr->seq16; //序列号，同上
    //拷贝接收的回显请求报文中的数据
    memcpy(buf->data+sizeof(icmp_hdr_t),req_buf->data+sizeof(icmp_hdr_t),req_buf->len-sizeof(icmp_hdr_t));
    icmp_hdr->checksum16=checksum16((uint16_t*)icmp_hdr,buf->len);   //校验和写入
    ip_out(buf,src_ip,NET_PROTOCOL_ICMP);
}

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 * @param src_ip 源ip地址
 */
void icmp_in(buf_t *buf, uint8_t *src_ip)
{
    // TO-DO
    if(buf->len<sizeof(icmp_hdr_t)) return;    //接收包长度小于icmp报头长度，丢弃
    icmp_hdr_t* icmp_hdr=(icmp_hdr_t*)buf->data;   //icmp报头捕获
    if(icmp_hdr->type==ICMP_TYPE_ECHO_REQUEST && icmp_hdr->code==0)    //如果报文icmp类型为回显请求
    {
        icmp_resp(buf,src_ip);    //回送一个回显应答
    }
}

/**
 * @brief 发送icmp不可达
 * 
 * @param recv_buf 收到的ip数据包
 * @param src_ip 源ip地址
 * @param code icmp code，协议不可达或端口不可达
 */
void icmp_unreachable(buf_t *recv_buf, uint8_t *src_ip, icmp_code_t code)
{
    // TO-DO
    //差错报文结构：icmp报头首部+IP数据报首部+前8字节数据字段
    buf_t* buf=&txbuf;
    buf_init(buf,sizeof(icmp_hdr_t)*2+sizeof(ip_hdr_t));   //初始化txbuf来发送icmp报文，长度为8+20+8
    icmp_hdr_t* icmp_hdr=(icmp_hdr_t*)buf->data;
    icmp_hdr_t* recv_hdr=(icmp_hdr_t*)recv_buf->data;
    //前八字节的ICMP报头首部填写
    icmp_hdr->type=ICMP_TYPE_UNREACH;  //icmp报文类型
    icmp_hdr->code=code;   //报文代码
    icmp_hdr->checksum16=0;
    //icmp不可达为差错报文，标识符和序列号字段未用，必须为0
    icmp_hdr->id16=0;    //标识符
    icmp_hdr->seq16=0;  //序列号
    //IP数据报首部与前八字节数据字段拷贝
    memcpy(buf->data+sizeof(icmp_hdr_t),recv_hdr,sizeof(ip_hdr_t)+sizeof(icmp_hdr_t));
    icmp_hdr->checksum16=checksum16((uint16_t*)icmp_hdr,buf->len);  //校验和字段
    ip_out(buf,src_ip,NET_PROTOCOL_ICMP);
}

/**
 * @brief 初始化icmp协议
 * 
 */
void icmp_init(){
    net_add_protocol(NET_PROTOCOL_ICMP, icmp_in);
}