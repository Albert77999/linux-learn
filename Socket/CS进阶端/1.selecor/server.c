#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/select.h>
#include <signal.h>

#define BUF_SIZE 1024

void error_handling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in serv_adr, clnt_adr;
    char message[BUF_SIZE];
    if (argc != 2)
    {
        printf("usage error()");
        exit(2);
    }

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        error_handling("socket error()");
    }
    memset(&serv_adr, 0, sizeof(serv_adr));
    memset(&clnt_adr, 0, sizeof(clnt_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = INADDR_ANY;
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("bind error()");
    }
    if (listen(serv_sock, 5) == -1)
    {
        error_handling("listen error()");
    }
    //  select处理
    fd_set read_set, tmp;
    FD_ZERO(&read_set);
    struct timeval timeout;
    FD_SET(serv_sock, &read_set);
    int max_fd = serv_sock;
    // 处理重头戏
    while (1)
    {
        tmp = read_set;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        int ret_val = select(max_fd + 1, &tmp, 0, 0, &timeout);
        if (ret_val == -1)
        {
            error_handling("select error()");
        }
        else if (ret_val == 0)
        {
            puts("time out!");
            continue;
        }
        for (int i = 0; i <= max_fd; i++)
        {
            if (i == serv_sock && FD_ISSET(i,&tmp))
            {
                socklen_t clnt_adr_sz = sizeof(clnt_adr);
                int clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
                if (clnt_sock == -1)
                    error_handling("accept error()");
                FD_SET(clnt_sock, &read_set);
                max_fd = clnt_sock > max_fd ? clnt_sock : max_fd;
                printf("connected with client: %d\n", clnt_sock);
            }
            else if (i != serv_sock && FD_ISSET(i, &tmp))
            {
                ssize_t read_sz = read(i, message, BUF_SIZE - 1);
                if(read_sz == -1)   error_handling("read error()");
                if(read_sz == 0)
                {
                    close(i);
                    FD_CLR(i,&read_set);
                    continue;
                }
                message[read_sz] = '\0';
                printf("message from client :%.*s\n", (int)read_sz, message);

                int sent = 0;
                while (sent < read_sz)
                {
                    int write_sz = write(i, message + sent, read_sz - sent);
                    if (write_sz <= 0)
                    {
                        error_handling("write error()");
                    }
                    sent += write_sz;
                }
            }
        }
    }

    return 0;
}
