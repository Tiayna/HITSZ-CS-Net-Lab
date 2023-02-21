#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_SIZE 65535

char buf[MAX_SIZE+1];

char* join(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    if (result == NULL) exit (1);

    strcpy(result, s1);
    strcat(result, s2);

    return result;
}



void recv_mail()
{
    const char* host_name = "pop.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 110; // POP3 server port
    const char* user = "*********@qq.com"; // TODO: Specify the user
    const char* pass = "****************"; // TODO: Specify the password
    char dest_ip[16];
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    int r_size;

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **) host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i-1]));

    // TODO: Create a socket,return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
    struct sockaddr_in addr_serv;
    s_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    memset(&addr_serv, 0, sizeof(addr_serv));  //每个字节都用0填充
    addr_serv.sin_family = AF_INET;  //使用IPv4地址
    addr_serv.sin_addr.s_addr = inet_addr(dest_ip);  //具体的IP地址（字符转长整型）
    addr_serv.sin_port = htons(port);  //端口，SMTP服务器与POP3服务器连接区别就在于端口号不同
    connect(s_fd, (struct sockaddr*)&addr_serv, sizeof(addr_serv));
    
    // Print welcome message
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);

    // USER <name><CR><LF>
    // PASS <name><CR><LF>
    // TODO: Send user and password and print server response
    const char * user_mesg = join(join("user ", user), "\r\n");
    printf("\n--------\n");
    printf("client : %s", user_mesg);
    printf("\n-------\n");
    send(s_fd, user_mesg, strlen(user_mesg), 0);
    
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);

    const char * Pass = join(join("pass ", pass),"\r\n");
    printf("\n--------\n");
    printf("client : %s", Pass);
    printf("\n-------\n");
    send(s_fd, Pass, strlen(Pass), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);

    // STAT<CR><LF> 返回邮箱统计信息
    // TODO: Send STAT command and print server response
    //check the number of mail and print total byte of it  邮箱邮件数和邮件占用大小
    const char * SA = join("stat", "\r\n");
    printf("\n--------\n");
    printf("client : %s", SA);
    printf("\n-------\n");
    send(s_fd, SA, strlen(SA), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);

    // LIST [<msg>]<CR><LF>   返回邮件信息，可指定参数
    // TODO: Send LIST command and print server response
    const char * List = join("list", "\r\n");   //不指定参数，返回所有邮件编号及大小
    printf("\n--------\n");
    printf("client : %s", List);
    printf("\n-------\n");
    send(s_fd, List, strlen(List), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);

    // RETR msg<CR><LF>  获取编号为msg的邮件正文
    // TODO: Retrieve the first mail and print its content
    const char * Retrieve = join("retr 1", "\r\n");   //获取编号为1的邮件正文
    printf("\n--------\n");
    printf("client : %s", Retrieve);
    printf("\n-------\n");
    send(s_fd, Retrieve, strlen(Retrieve), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);

    // QUIT<CR><LF> 结束与POP3服务器的通信
    // TODO: Send QUIT command and print server response
    const char * Quit = join("quit", "\r\n");
    printf("\n--------\n");
    printf("client : %s", Quit);
    printf("\n-------\n");
    send(s_fd, Quit, strlen(Quit), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);
    
    // if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    // {
    //     perror("recv");
    //     exit(EXIT_FAILURE);
    // }
    // buf[r_size] = '\0';// 要有空终止符
    // printf("%s", buf);
    close(s_fd);
}

int main(int argc, char* argv[])
{
    recv_mail();
    exit(0);
}