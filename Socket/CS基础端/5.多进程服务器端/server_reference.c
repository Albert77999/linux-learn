#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>

static void error_handling(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

/* 子进程结束后，内核给父进程发送 SIGCHLD。
 *
 * 一次 SIGCHLD 可能对应多个已经结束的子进程，因此不能只 waitpid 一次，
 * 必须循环回收，直到暂时没有可回收的子进程为止。
 *
 * 信号处理函数中不能使用 printf、malloc 等普通函数；这里仅调用 POSIX
 * 规定可在信号处理函数中使用的 waitpid。 */
static void reap_children(int sig)
{
    int saved_errno = errno;
    (void)sig;

    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
    }

    errno = saved_errno;
}

static void echo_client(int clnt_sock)
{
    char message[1024];
    ssize_t read_sz;

    /* read 返回 0 说明客户端已正常关闭连接；返回 -1 说明发生错误。 */
    while ((read_sz = read(clnt_sock, message, sizeof(message))) > 0)
    {
        ssize_t sent = 0;

        /* TCP 是字节流。一次 write 不保证发送完 read 到的全部数据，
         * 因此要循环发送，直到 sent == read_sz。 */
        while (sent < read_sz)
        {
            ssize_t write_sz = write(clnt_sock,
                                     message + sent,
                                     (size_t)(read_sz - sent));
            if (write_sz == -1)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                perror("write");
                return;
            }
            sent += write_sz;
        }
    }

    if (read_sz == -1 && errno != EINTR)
    {
        perror("read");
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* 监听 socket 只负责接收新的连接，绝不直接和某个客户端通信。 */
    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        error_handling("socket");
    }

    /* 服务端刚退出时端口可能暂时处于 TIME_WAIT。设置该选项后，
     * 在开发阶段可以更方便地重启服务端。 */
    int opt = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        error_handling("setsockopt");
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

    /* 注册 SIGCHLD 处理函数。
     * SA_RESTART 让 accept 等可重启系统调用在被 SIGCHLD 打断后自动继续等待；
     * SA_NOCLDSTOP 表示子进程只是暂停/继续时不必通知父进程。 */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = reap_children;
    act.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&act.sa_mask);

    if (sigaction(SIGCHLD, &act, NULL) == -1)
    {
        error_handling("sigaction");
    }

    printf("server is listening on port %s\n", argv[1]);

    while (1)
    {
        struct sockaddr_in clnt_adr;
        socklen_t clnt_adr_sz = sizeof(clnt_adr);

        /* accept 成功后得到一个全新的通信 socket：clnt_sock。
         * serv_sock 仍继续用于接收下一个客户端的连接。 */
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

        /* fork 后，父子进程都暂时持有 serv_sock 和 clnt_sock 的副本。
         * 接下来必须各自关闭不需要的描述符。 */
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            close(clnt_sock);
            continue;
        }

        if (pid == 0)
        {
            /* 子进程只负责当前客户端，不需要再接收新连接。 */
            close(serv_sock);
            echo_client(clnt_sock);
            close(clnt_sock);
            _exit(EXIT_SUCCESS);
        }

        /* 父进程只负责继续 accept 下一个客户端；当前客户端由子进程处理。
         * 若父进程不关闭该副本，客户端断开时 TCP 连接可能无法及时结束。 */
        close(clnt_sock);
    }
}
