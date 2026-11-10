#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
    char message[1024];
    struct sockaddr_in serv_adr;
    struct sockaddr_in clnt_adr;
    socklen_t serv_adr_sz = sizeof(serv_adr);
    socklen_t clnt_adr_sz = sizeof(clnt_adr);

    if (argc != 2)
    {
        printf("usage error : %s\n", argv[0]);
        exit(2);
    }

    int serv_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
    {
        error_handling("socket error()");
    }
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = INADDR_ANY;
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, serv_adr_sz))
    {
        error_handling("bind error()");
    }

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        ssize_t recv_sz = recvfrom(serv_sock, message, sizeof(message) - 1, 0, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        if (recv_sz == -1)
        {
            error_handling("recvfrom error()");
        }
        message[recv_sz] = '\0';

        char clnt_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clnt_adr.sin_addr, clnt_ip, sizeof(clnt_ip));

        printf("from %s:%u: %s",
               clnt_ip,
               ntohs(clnt_adr.sin_port),
               message);
        ssize_t send_sz = sendto(serv_sock, message, recv_sz, 0, (struct sockaddr *)&clnt_adr, clnt_adr_sz);
        if (send_sz == -1)
        {
            error_handling("sendto error()");
        }
    }

    close(serv_sock);
    return 0;
}