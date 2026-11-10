#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void error_handling(char *message)
{
    fputs(message,stderr);
    fputc('\ns',stderr);
    exit(1);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in serv_adr;
    char message[100];

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
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));
    
    if(connect(clnt_sock,(struct sockaddr*)&serv_adr,sizeof(serv_adr)) == -1)
    {
        error_handling("connect error()");
    }

    int idx = 0;
    int cur_sz = 0;
    while((cur_sz = read(clnt_sock,message + idx,1)) != 0)
    {
        if(cur_sz == -1)
        {
            error_handling("read error()");
        }
        idx += cur_sz;
    }
    message[idx] = '\0';

    printf("message from server:%s\n",message);

    close(clnt_sock);

    return 0;
}