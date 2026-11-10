#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define BUF_SIZE 1024

void error_handling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void read_routine(FILE *readfp, char *message)
{
    while (fgets(message, BUF_SIZE, readfp))
    {
        // fgets会保留换行符，因此这里不再额外输出'\n'。
        printf("message from server: %s", message);
    }
    if (feof(readfp))
    {
        puts("数据读取完毕，已到达文件末尾。");
    }
    else if (ferror(readfp))
    {
        perror("fgets");
    }
}

void write_routine(FILE *writefp, char *message)
{
    while (1)
    {
        printf("please enter q/Q to exit\n");
        if (fgets(message, BUF_SIZE, stdin) == NULL)
        {
            if (ferror(stdin))
            {
                error_handling("fgets error()");
            }

            // 输入到达EOF时也关闭TCP写方向，避免父进程一直等待服务端关闭。
            if (fflush(writefp) == EOF)
            {
                error_handling("fflush error()");
            }
            if (shutdown(fileno(writefp), SHUT_WR) == -1)
            {
                error_handling("shutdown error()");
            }
            break;
        }
        if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
        {
            puts("exit success!");
            if (fflush(writefp) == EOF)
            {
                error_handling("fflush error()");
            }
            if (shutdown(fileno(writefp), SHUT_WR) == -1)
            {
                error_handling("shutdown error()");
            }
            break;
        }
        size_t message_sz = strlen(message);
        size_t write_sz = fwrite(message, 1, message_sz, writefp);
        if (write_sz != message_sz)
            error_handling("fwrite error()");
        if (fflush(writefp) == EOF)
        {
            error_handling("fflush error()");
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
    pid_t pid = fork();
    if (pid == -1)
    {
        error_handling("fork error()");
    }
    if (pid == 0)
    {
        // 子进程读取键盘并向服务端发送消息。
        FILE *writefp = fdopen(clnt_sock, "w");
        if (writefp == NULL)
        {
            error_handling("fdopen write error()");
        }
        write_routine(writefp, message);
        if (fclose(writefp) == EOF)
        {
            error_handling("fclose write error()");
        }
        return 0;
    }

    // 父进程接收服务端回声。
    FILE *readfp = fdopen(clnt_sock, "r");
    if (readfp == NULL)
    {
        error_handling("fdopen read error()");
    }
    read_routine(readfp, message);
    if (fclose(readfp) == EOF)
    {
        error_handling("fclose read error()");
    }

    // 读完服务端数据后，可靠地等待并回收唯一的子进程。
    int status;
    pid_t child_id = waitpid(pid, &status, 0);
    if (child_id == -1)
    {
        error_handling("waitpid error()");
    }
    printf("Removed proc id: %d\n", (int)child_id);
    if (WIFEXITED(status))
    {
        printf("child send: %d\n", WEXITSTATUS(status));
    }
    return 0;
}
