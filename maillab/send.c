#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include "base64_utils.h"

#define MAX_SIZE 4095
#define swap16(x) ((((x)&0xFF) << 8) | (((x) >> 8) & 0xFF)) //为16位数据交换大小端
char buf[MAX_SIZE+1];


char* join(const char *s1, const char *s2)   //字符串拼接
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    if (result == NULL) exit (1);

    strcpy(result, s1);
    strcat(result, s2);

    return result;   //string s1+s2
}


// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char* receiver, const char* subject, const char* msg, const char* att_path)
{
    const char* end_msg = "\r\n.\r\n";
    const char* host_name = "smtp.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 25; // SMTP server port
    const char* user = "*********@qq.com"; // TODO: Specify the user
    const char* pass = "****************"; // TODO: Specify the password
    const char* from = "*********"; // TODO: Specify the mail address of the sender  发送端邮箱地址
    char dest_ip[16]; // Mail server IP address
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    int r_size;
    const char * boundary = "--xiewen1xiewen1xiewen1\r\n";
    //const char * MIME = "Content-Type: multipart/mixed; boundary=xiewen1xiewen1xiewen1\n";

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)    //根据DNS域名得到char类型的dest_ip
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **) host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i-1]));

    // TODO: Create a socket,return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
    s_fd = socket(AF_INET, SOCK_STREAM, 0);
    // struct sockaddr_in{     //客户端连接服务器需要服务器的IP地址与端口号
    //     short in sin_family; //地址族
    //     unsigned short int sin_port;  //端口号
    //     struct in_addr sin_addr;   //IP地址
    //     unsigned char sin_zero[8]; //填充
    // }
    struct sockaddr_in s_in;
    s_in.sin_family = AF_INET; //使用IPv4地址
    s_in.sin_port = swap16(port); //端口
    bzero(s_in.sin_zero, 8);
    // struct in_addr{
    //     unsigned long s_addr;    //无符号长整型
    // }
    struct in_addr ip_d;
    ip_d.s_addr = inet_addr(dest_ip); //具体的IP地址：字符串表示的dest_ip->32位二进制
    s_in.sin_addr = ip_d;

    if (connect(s_fd,(struct sockaddr *)&s_in,sizeof(struct sockaddr_in))==-1){
        perror("ERROR");
        exit(EXIT_FAILURE);
    }

    // Print welcome message
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)  //网络数据接收
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符，r_size为接收数据大小，在数据结尾处终止
    printf("%s", buf);

    // EHLO <domain><CR><LF>
    // Send EHLO command and print server response   建立连接后第一条命令，附上发送方主机名
    const char* Ehlo = "ehlo qq.com\r\n"; // TODO: Enter EHLO command here
    printf("client : %s", Ehlo);
    send(s_fd, Ehlo, strlen(Ehlo), 0);     //网络TCP连接建立后，send实现网络数据的发送  

    // TODO: Print server response to EHLO command   服务器对EHLO命令响应，数据接收
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);
    printf("~~~~~~~\n");

    // AUTH <para><CR><LF>
    // TODO: Authentication. Server response should be printed out.   登录认证
    const char* Authentication = "auth login\r\n";
    printf("client : %s", Authentication);
    send(s_fd, Authentication, strlen(Authentication), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)  //接收服务器响应
    {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);
    printf("~~~~~~~\n");

    char * tp = encode_str(user);   //xxxxxxxxxx@qq.com经过base64编码后的邮箱用户名
    const char * user_name = join(tp, "\r\n");
    printf("client : %s", user_name);
    send(s_fd, user_name, strlen(user_name), 0);
    free(tp);

    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);
    printf("~~~~~~~\n");

    tp = encode_str(pass);    //经过base64编码后的邮箱密码
    const char * vo_code = join(tp, "\r\n");
    printf("client : %s", vo_code);
    send(s_fd, vo_code, strlen(vo_code), 0);
    free(tp);

    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);
    printf("~~~~~~~\n");

    // MAIL FROM: <reverse-path><CR><LF>
    // TODO: Send MAIL FROM command and print server response   MAIL FROM指令：指定发件人邮箱地址
    const char* mail_mesage = join(join(join(join("MAIL FROM:","<"),from),">"), "\r\n");
    printf("client : %s", mail_mesage);
    send(s_fd, mail_mesage, strlen(mail_mesage), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);
    printf("~~~~~~~\n");
    
    // RCPT TO: <forward-path><CR><LF>
    // TODO: Send RCPT TO command and print server response
    const char* mes_Rcpt = join(join(join(join("RCPT TO:","<"),receiver),">"), "\r\n");
    printf("client : %s", mes_Rcpt);
    send(s_fd, mes_Rcpt, strlen(mes_Rcpt), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);
    printf("~~~~~~~\n");

    // DATA<CR><LF>  输入邮件内容
    // TODO: Send DATA command and print server response
    const char * Data = join("data", "\r\n");
    printf("client : %s", Data);
    send(s_fd, Data, strlen(Data), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)    //客户端发送DATA命令后服务器做出响应(354)，然后正式发送数据
    {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);
    printf("~~~~~~~\n");
    
    //邮件数据内容主体部分：
    char Message_body[100];
    FILE* f_message = fopen(msg, "rt");
    FILE * f_application = fopen(att_path,"rt");
    FILE* Base64_file = fopen("Base64file", "w+");
    encode_file(f_application, Base64_file);
    fclose(Base64_file);
    fclose(f_application);
    Base64_file = fopen("Base64file", "r");
    fseek(Base64_file, 0, SEEK_END); 
    int fileSize;
    fileSize = ftell(Base64_file);                                 
    printf("fileSize=====%d\n", fileSize);
    char *Application_body;
    Application_body = (char*)malloc(fileSize + 1);
    rewind(Base64_file);
    fread(Application_body, sizeof(char),fileSize, Base64_file);
    Application_body[fileSize] = '\0';
    printf("strText=====%s\n", Application_body);
    fclose(Base64_file);

    fgets(Message_body, 99, f_message);
    fclose(f_message);

    //MIME相关文件头
    const char * Message_title = join(join("From: ", user),"\r\n");
    printf("client : %s", Message_title);
    send(s_fd, Message_title, strlen(Message_title), 0);

    Message_title = join(join("To: ", receiver),"\r\n");
    printf("client : %s", Message_title);
    send(s_fd, Message_title, strlen(Message_title), 0);

    Message_title = join("MIME-Version: 1.0", "\r\n");
    printf("client : %s", Message_title);
    send(s_fd, Message_title, strlen(Message_title), 0);

    Message_title = join("Content-Type: multipart/mixed; boundary=xiewen1xiewen1xiewen1", "\r\n");
    printf("client : %s", Message_title);
    send(s_fd, Message_title, strlen(Message_title), 0);

    Message_title = join(join(join("subject: ", subject), "\r\n"), "\r\n");
    printf("client : %s", Message_title);
    send(s_fd, Message_title, strlen(Message_title), 0);

    printf("client : %s", boundary);
    send(s_fd, boundary, strlen(boundary),0);

    const char * encodeLabel = "Content-Transfer-Encoding: base64\r\n";
    printf("client : %s", encodeLabel);
    send(s_fd, encodeLabel, strlen(encodeLabel), 0);

    const char * Application_head = join(join("Content-Type: application/txt; name=test.txt", "\r\n") , "\r\n");
    printf("client : %s", Application_head);
    send(s_fd, Application_head, strlen(Application_head), 0);
    
    const char * Ab = join(join(Application_body, "\r\n"), "\r\n");
    printf("client : %s", Ab);
    send(s_fd, Ab, strlen(Ab),0);
    

    printf("client : %s", boundary);
    send(s_fd, boundary, strlen(boundary),0);

    

    const char * Text_head = join(join("Content-Type: text/plain", "\r\n"), "\r\n");
    printf("client : %s", Text_head);
    send(s_fd, Text_head, strlen(Text_head), 0);
    
    
    
    const char * Mb = join(join(Message_body, "\r\n"), "\r\n");
    printf("client : %s", Mb);
    send(s_fd, Mb, strlen(Mb),0);
    
    printf("client : %s", boundary);
    send(s_fd, boundary, strlen(boundary),0);

    send(s_fd, end_msg, strlen(end_msg), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("ERROR------recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; 

    // QUIT<CR><LF>  结束通信
    // TODO: Send QUIT command and print server response
    const char * Quit = join("quit", "\r\n");
    printf("client : %s", Quit);
    send(s_fd, Quit, strlen(Quit), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("ERROR------recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // 要有空终止符
    printf("%s", buf);
    close(s_fd);   //断开socket_stream连接
    
}

int main(int argc, char* argv[])
{
    int opt;
    char* s_arg = NULL;
    char* m_arg = NULL;
    char* a_arg = NULL;
    char* recipient = NULL;
    const char* optstring = ":s:m:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_arg = optarg;
            break;
        case 'm':
            m_arg = optarg;
            break;
        case 'a':
            a_arg = optarg;
            break;
        case ':':
            fprintf(stderr, "Option %c needs an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unknown option: %c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "Recipient not specified.\n");
        exit(EXIT_FAILURE);
    }
    else if (optind < argc - 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    }
}