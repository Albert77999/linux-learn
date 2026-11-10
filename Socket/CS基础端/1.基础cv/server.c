#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_sz = sizeof(clnt_addr);
    char message[] = "hello world!";
    if(argc != 2)
    {
        printf("usage error : %s\n",argv[0]);
        exit(2);
    }

    int serv_sock = socket(AF_INET,SOCK_STREAM,0);
    if(serv_sock == - 1)
    {
        error_handling("socket error()");
    }
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1)
    {
        error_handling("bind error()");
    }

    if(listen(serv_sock,5) == -1)
    {
        error_handling("listen error()");
    }

    int clnt_sock = accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_addr_sz);
    if(clnt_sock == -1)
    {
        error_handling("accept error()");
    }

    //传输数据
    write(clnt_sock,message,strlen(message));

    close(clnt_sock);
    close(serv_sock);

    return 0;
}
