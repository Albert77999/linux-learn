#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main(int argc, char *argv[])
{
    char message[] = "Welcome to China!";
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

    int clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
    if (clnt_sock == -1)
    {
        error_handling("accept error()");
    }
    int msg_sz = write(clnt_sock, message, strlen(message));
    if (msg_sz == -1)
    {
        error_handling("write error()");
    }

    close(serv_sock);
    close(clnt_sock);

    return 0;
}