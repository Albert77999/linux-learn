#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/times.h>

#define BUF_SIZE 1024

void error_handling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void read_childproc(int sig)
{
    (void)sig;
    int status;
    pid_t id = waitpid(-1, &status, WNOHANG);
    if (id > 0 && WIFEXITED(status))
    {
        printf("Removed proc id :%d\n", id);
        printf("child send :%d\n", WEXITSTATUS(status));
    }
}

void read_routine(int clnt_sock, char *message)
{
    while(1)
    {
        ssize_t read_sz = read(clnt_sock,message,BUF_SIZE - 1);
        if(read_sz == -1)
        {
            error_handling("read error()");
        }
        if(read_sz == 0)
        {
            return;
        }
        message[read_sz] = '\0';
        printf("message from server :%s\n",message);
    }
}

void write_routine(int clnt_sock, char *message)
{
    while(1)
    {
        printf("please enter q/Q to exit\n");
        if (fgets(message, BUF_SIZE, stdin) == NULL)
        {
            error_handling("fgets error()");
        }
        if(!strcmp(message,"q\n") || !strcmp(message,"Q\n"))
        {
            puts("exit success!");
            if (shutdown(clnt_sock,SHUT_WR) == -1)
            {
                error_handling("shutdown error()");
            }
            break;
        }
        size_t message_sz = strlen(message);
        size_t sent = 0;
        while(sent < message_sz)
        {
            ssize_t write_sz = write(clnt_sock,message + sent,message_sz - sent);
            if(write_sz <= 0)    error_handling("write error()");
            sent += (size_t)write_sz;
        }
    }
}

int main(int argc, char *argv[])
{
    struct sockaddr_in serv_adr;
    char message[BUF_SIZE];
    if (argc != 3)
    {
        printf("usage error :%s\n", argv[0]);
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

    if ((connect(clnt_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr))) == -1)
    {
        error_handling("connect error()");
    }

    struct sigaction act;
    act.sa_handler = read_childproc;
    sigemptyset(&act.sa_mask);
    // SIGCHLD 可能打断父进程的 read，SA_RESTART 让 read 自动继续。
    act.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &act, NULL) == -1)
    {
        error_handling("sigaction error()");
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        error_handling("fork error()");
    }
    if (pid == 0)
    {
        // 子进程读取键盘并向服务端发送消息。
        write_routine(clnt_sock, message);
        close(clnt_sock);
        return 0;
    }

    // 父进程接收服务端回声；子进程结束时由 SIGCHLD 处理函数回收。
    read_routine(clnt_sock, message);
    close(clnt_sock);
    return 0;
}
