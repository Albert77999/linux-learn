#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

void error_handling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main(int argc, char *argv[])
{
    char message[BUF_SIZE];
    struct sockaddr_in serv_adr;
    struct sockaddr_in clnt_adr;
    socklen_t clnt_adr_sz = sizeof(clnt_adr);

    if (argc != 2)
    {
        printf("usage error:%s\n", argv[0]);
        exit(2);
    }

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        error_handling("socket error()");
    }
    memset(&serv_adr, 0, sizeof(serv_adr));
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

    for (int i = 0; i < 5; i++)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        int clnt_sock = accept(serv_sock,
                               (struct sockaddr *)&clnt_adr,
                               &clnt_adr_sz);
        if (clnt_sock == -1)
        {
            error_handling("accept error()");
        }
        printf("connection with client %d success!\n", i + 1);

        int write_fd = dup(clnt_sock);
        if (write_fd == -1)
        {
            error_handling("dup error()");
        }
        FILE *writefp = fdopen(write_fd, "w");
        if (writefp == NULL)
        {
            error_handling("fdopen write error");
        }

        FILE *readfp = fdopen(clnt_sock, "r");
        if (readfp == NULL)
        {
            error_handling("fdopen read error");
        }
        while (fgets(message, BUF_SIZE, readfp))
        {
            if (fputs(message, writefp) == EOF)
            {
                error_handling("fputs error");
            }

            if (fflush(writefp) == EOF)
            {
                error_handling("fflush error");
            }
        }

        if (ferror(readfp))
        {
            error_handling("fgets error");
        }

        if (fclose(readfp) == EOF)
        {
            error_handling("fclose read error");
        }
        if (fclose(writefp) == EOF)
        {
            error_handling("fclose write error");
        }
    }

    close(serv_sock);
    return 0;
}
