#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

void error_handling(char *message)
{
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in serv_adr;

    if(argc != 3)
    {
        printf("usage error: %s\n",argv[0]);
        exit(2);
    }

    int clnt_sock = socket(AF_INET,SOCK_STREAM,0);
    if(clnt_sock == -1)
    {
        error_handling("socket error()");
    }
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));
    
    if(connect(clnt_sock,(struct sockaddr*)&serv_adr,sizeof(serv_adr)) == -1)
    {
        error_handling("connect error()");
    }

    if (write(clnt_sock, "123", strlen("123")) == -1)
        error_handling("write error()");
    if (send(clnt_sock, "4", strlen("4"), MSG_OOB) == -1)
        error_handling("send error()");
    if (write(clnt_sock, "567", strlen("567")) == -1)
        error_handling("write error()");
    if (send(clnt_sock, "8910", strlen("8910"), MSG_OOB) == -1)
        error_handling("send error()");


    close(clnt_sock);

    return 0;
}
