#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

#define BUF_SIZE 1024

int clnt_sock;

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void urg_handler(int sig)
{
    (void)sig;
    char buf[BUF_SIZE];
    ssize_t str_len = recv(clnt_sock, buf, BUF_SIZE - 1, MSG_OOB);
    if (str_len > 0)
    {
        buf[str_len] = '\0';
        printf("Urgent message: %s\n", buf);
    }
}

int main(int argc, char *argv[])
{
    char message[BUF_SIZE];
    struct sockaddr_in serv_adr;
    struct sockaddr_in clnt_adr;
    socklen_t clnt_adr_sz = sizeof(clnt_adr);

    if (argc != 2)
    {
        printf("usage error: %s\n", argv[0]);
        exit(2);
    }

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        error_handling("socket error()");
    }
    memset(&serv_adr,0,sizeof(serv_adr));
    memset(&clnt_adr, 0, sizeof(clnt_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("bind error()");
    }
    if (listen(serv_sock, 5) == -1)
    {
        error_handling("listen error()");
    }

    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
    if (clnt_sock == -1)
    {
        error_handling("accept error()");
    }
    
    //信号处理
    struct sigaction act;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    act.sa_handler = urg_handler;
    int state = sigaction(SIGURG,&act,0);
    if(state == -1)
    {
        error_handling("sigaction error()");
    }
    if (fcntl(clnt_sock, F_SETOWN, getpid()) == -1)
    {
        error_handling("fcntl error()");
    }

    while (1)
    {
        ssize_t msg_len = recv(clnt_sock, message, BUF_SIZE - 1, 0);

        if (msg_len > 0)
        {
            message[msg_len] = '\0';
            puts(message);
        }
        else if (msg_len == 0)
        {
            // 对端正常关闭连接。
            break;
        }
        else if (errno == EINTR)
        {
            // recv 被 SIGURG 中断，不是网络错误，重新读取普通数据。
            continue;
        }
        else
        {
            error_handling("recv error()");
        }
    }

    close(serv_sock);
    close(clnt_sock);
    return 0;
}
