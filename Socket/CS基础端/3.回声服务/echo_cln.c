#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handling(const char *message)
{
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}

int main(int argc,char *argv[])
{
    struct sockaddr_in serv_adr;
    char message[1024];

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
    memset(&serv_adr,0,sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if(connect(clnt_sock,(struct sockaddr*)&serv_adr,sizeof(serv_adr)) == -1)
    {
        error_handling("connect error()");
    }

    //客户端操作
    while(1)
    {
        printf("please enter q/Q to exit");
        if(fgets(message,sizeof(message),stdin) == NULL)
        {
            error_handling("fgets error()");
        }

        if(!strcmp(message,"q\n") || !strcmp(message,"Q\n"))
        {
            break;
        }

        int message_sz = strlen(message);
        int sent = 0;
        while (sent < message_sz)
        {
            int write_sz = write(clnt_sock, message + sent, message_sz - sent);
            if (write_sz <= 0)
            {
                error_handling("write error()");
            }
            sent += write_sz;
        }

        int received = 0;
        while (received < message_sz)
        {
            int read_sz = read(clnt_sock,
                               message + received,
                               message_sz - received);
            if (read_sz <= 0)
            {
                error_handling(read_sz == 0 ?
                               "server closed connection" :
                               "read error()");
            }
            received += read_sz;
        }
        message[received] = '\0';
        printf("message from server: %s\n",message);
    }
    close(clnt_sock);
    return 0;
}
