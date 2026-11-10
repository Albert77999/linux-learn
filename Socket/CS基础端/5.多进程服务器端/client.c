#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void write_routine(int clnt_sock, char *message)
{
    while (1)
    {
        printf("please enter q/Q to exit");
        if (fgets(message, 1024, stdin) == NULL)
        {
            error_handling("fgets error()");
        }

        if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
        {
            shutdown(clnt_sock, SHUT_WR);
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
    }
}
void read_routine(int clnt_sock, char *message)
{
    while (1)
    {
        int received = 0;

        int read_sz = read(clnt_sock,
                           message + received,
                           1024);
        if (read_sz <= 0)
        {
            error_handling(read_sz == 0 ? "server closed connection" : "read error()");
        }
        received += read_sz;

        message[received] = '\0';
        printf("message from server: %s\n", message);
    }
}

int main(int argc, char *argv[])
{
    struct sockaddr_in serv_adr;
    char message[1024];

    if (argc != 3)
    {
        printf("usage error: %s\n", argv[0]);
        exit(2);
    }

    int clnt_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (clnt_sock == -1)
    {
        error_handling("socket error()");
    }
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(clnt_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("connect error()");
    }

    // 客户端操作

    pid_t pid = fork();
    if (pid == 0)
    {
        write_routine(clnt_sock, message);
    }
    else if (pid == -1)
    {
        return 0;
    }
    else
    {
        read_routine(clnt_sock, message);
    }

    close(clnt_sock);
    return 0;
}
