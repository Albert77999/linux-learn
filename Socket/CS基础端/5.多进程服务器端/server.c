#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>

void error_handling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void read_childproc(int sig)
{
    int status;
    pid_t id = waitpid(-1, &status, WNOHANG);
    if (WIFEXITED(status))
    {
        printf("Removed proc id: %d \n", id);             // 子进程的 pid
        printf("Child send: %d \n", WEXITSTATUS(status)); // 子进程的返回值
    }
}

int main(int argc, char *argv[])
{
    char message[100];
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

    struct sigaction act;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    act.sa_handler = read_childproc;
    sigaction(SIGCHLD, &act, 0);

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        int clnt_sock = accept(serv_sock,
                               (struct sockaddr *)&clnt_adr,
                               &clnt_adr_sz);
        if (clnt_sock == -1)
        {
            continue;
        }
        else
            puts("new client connected...");

        //  进程处理
        pid_t pid = fork();

        if (pid == 0)
        {
            close(serv_sock);
            int read_sz = 0;
            while ((read_sz = read(clnt_sock, message, sizeof(message))) > 0)
            {
                printf("message from client :%.*s\n", read_sz, message);

                int sent = 0;
                while (sent < read_sz)
                {
                    int write_sz = write(clnt_sock, message + sent, read_sz - sent);
                    if (write_sz <= 0)
                    {
                        error_handling("write error()");
                    }
                    sent += write_sz;
                }
            }
            if (read_sz == -1)
            {
                error_handling("read error()");
            }
            close(clnt_sock);
            return 3;
        }
        else if (pid == -1)
        {
            close(clnt_sock);
            continue;
        }
        else
        {
            close(clnt_sock);
        }
    }

    close(serv_sock);
    return 0;
}
