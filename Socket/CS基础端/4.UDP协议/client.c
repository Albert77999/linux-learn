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

    if (argc != 3)
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
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("connect error()");
    }

    while (1)
    {
        fputs("please enter q/Q to exit\n", stdout);
        if (fgets(message, sizeof(message), stdin) == NULL)
        {
            error_handling("fgets error()");
        }
        if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
        {
            break;
        }
        //ssize_t send_sz = sendto(serv_sock, message, strlen(message), 0, (struct sockaddr *)&serv_adr, serv_adr_sz);
        int send_sz = write(serv_sock,message,strlen(message));
        if (send_sz == -1)
        {
            error_handling("sendto error()");
        }
       // ssize_t recv_sz = recvfrom(serv_sock,message,sizeof(message) - 1,0,(struct sockaddr*)&serv_adr,&serv_adr_sz);
        int recv_sz = read(serv_sock,message,sizeof(message) - 1);
        if(recv_sz == -1)
        {
            error_handling("recvfrom error()");
        }
        message[recv_sz] = '\0';
        printf("message from server: %s\n",message);
    }

    close(serv_sock);
    return 0;
}