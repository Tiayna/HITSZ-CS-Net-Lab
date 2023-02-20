#ifndef DRIVER_H
#define DRIVER_H

#include "net.h"

#ifndef PCAP_BUF_SIZE
#define PCAP_BUF_SIZE 1024
#endif
int driver_open();    //打开网卡
int driver_recv(buf_t *buf);   //试图从网卡接收数据包
int driver_send(buf_t *buf);   //使用网卡发送一个数据包
void driver_close();  //关闭网卡
#endif