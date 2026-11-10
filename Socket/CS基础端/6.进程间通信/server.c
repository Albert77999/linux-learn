#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_SIZE 100

static void error_handling(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

/* 每个客户端由一个子进程处理。子进程结束时会发送 SIGCHLD。
 * SIGCHLD 可能合并，因此循环 waitpid，确保全部已结束的子进程都被回收。 */
static void read_childproc(int sig)
{
    int saved_errno = errno;
    (void)sig;

    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
    }

    errno = saved_errno;
}

/* 把 read 得到的全部字节回写给客户端。
 * TCP 的 write 不保证一次发送完，所以需要循环发送。 */
static int write_all(int fd, const char *buf, size_t len)
{
    size_t sent = 0;

    while (sent < len)
    {
        ssize_t write_sz = write(fd, buf + sent, len - sent);
        if (write_sz == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        sent += (size_t)write_sz;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        error_handling("socket");
    }

    struct sockaddr_in serv_adr;
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons((unsigned short)atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("bind");
    }

    if (listen(serv_sock, 5) == -1)
    {
        error_handling("listen");
    }

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = read_childproc;
    act.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&act.sa_mask);

    if (sigaction(SIGCHLD, &act, NULL) == -1)
    {
        error_handling("sigaction");
    }

    int fds[2];
    if (pipe(fds) == -1)
    {
        error_handling("pipe");
    }

    /* 先创建一个专门写日志的进程。
     * fds[0] 是读端，fds[1] 是写端。 */
    pid_t log_pid = fork();
    if (log_pid == -1)
    {
        error_handling("fork logger");
    }

    if (log_pid == 0)
    {
        /* 日志进程不接收 TCP 连接，也不向管道写数据。 */
        close(serv_sock);
        close(fds[1]);

        FILE *fp = fopen("echomsg.txt", "w");
        if (fp == NULL)
        {
            perror("fopen echomsg.txt");
            close(fds[0]);
            _exit(EXIT_FAILURE);
        }

        char msgbuf[BUF_SIZE];
        ssize_t len;

        /* 管道也是字节流：这里按收到的字节原样记录。
         * 所有写端关闭后，read 返回 0，日志进程就正常结束。 */
        while ((len = read(fds[0], msgbuf, sizeof(msgbuf))) > 0)
        {
            if (fwrite(msgbuf, 1, (size_t)len, fp) != (size_t)len)
            {
                perror("fwrite");
                break;
            }

            /* 让实验时能立即在文件中看到刚写入的日志。 */
            fflush(fp);
        }

        if (len == -1)
        {
            perror("pipe read");
        }

        fclose(fp);
        close(fds[0]);
        _exit(EXIT_SUCCESS);
    }

    /* 父进程以后只把 fds[1] 继承给客户端服务子进程。
     * 关闭 fds[0]，否则日志进程可能永远等不到管道 EOF。 */
    close(fds[0]);

    while (1)
    {
        struct sockaddr_in clnt_adr;
        socklen_t clnt_adr_sz = sizeof(clnt_adr);

        int clnt_sock = accept(serv_sock,
                               (struct sockaddr *)&clnt_adr,
                               &clnt_adr_sz);
        if (clnt_sock == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("accept");
            continue;
        }

        puts("new client connected...");

        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork client");
            close(clnt_sock);
            continue;
        }

        if (pid == 0)
        {
            /* 子进程只服务当前客户端：不需要继续 accept，也不读管道。 */
            close(serv_sock);

            char message[BUF_SIZE];
            ssize_t read_sz;

            while ((read_sz = read(clnt_sock, message, sizeof(message))) > 0)
            {
                /* 先回声给客户端。 */
                if (write_all(clnt_sock, message, (size_t)read_sz) == -1)
                {
                    perror("socket write");
                    break;
                }

                /* 再把同一批字节交给日志进程。
                 * 每次最多 BUF_SIZE 字节，小于 PIPE_BUF，因此单次写入不会
                 * 与其他客户端子进程的单次日志写入发生字节交叉。 */
                if (write_all(fds[1], message, (size_t)read_sz) == -1)
                {
                    perror("pipe write");
                    break;
                }
            }

            if (read_sz == -1 && errno != EINTR)
            {
                perror("socket read");
            }

            close(clnt_sock);
            close(fds[1]);
            _exit(EXIT_SUCCESS);
        }

        /* 父进程把当前客户端完全交给子进程，自己回到 accept 等待下一位。 */
        close(clnt_sock);
    }
}
