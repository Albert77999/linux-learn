#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void *error_handling(char *message)
{
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}

int main(int argc, char* argv[])
{
    struct sockaddr_in serv_addr;
    char message[100];
    if(argc != 3)
    {
        printf("usage error : %s\n",argv[0]);
        exit(2);
    }
    int clnt_sock = socket(AF_INET,SOCK_STREAM,0);
    if(clnt_sock == -1)
    {
        error_handling("socket error()");
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    //调用connect函数向服务器发送连接请求
    if((connect(clnt_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1))
    {
        error_handling("connect error()");
    }

    int str_len = read(clnt_sock,message,sizeof(message) - 1);
    if(str_len == -1)
    {
        error_handling("read error()");
    }
    message[str_len] = '\0';
    printf("message from server: %s\n",message);
    close(clnt_sock);
    
    return 0;
}