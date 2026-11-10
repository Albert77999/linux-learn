#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutex;

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void *handle_clnt(void *arg);
void send_msg(char *msg, int len);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    if (argc != 2)
    {
        printf("usage error()");
        exit(2);
    }
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket error()");
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind error()");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen error()");

    pthread_mutex_init(&mutex, NULL); // 创建互斥锁
    while (1)
    {
        socklen_t clnt_addr_sz = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_sz);
        if (clnt_sock == -1)
            error_handling("accept error()");
        // 处理逻辑
        pthread_mutex_lock(&mutex);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutex);
        // 创建业务处理
        pthread_t tid;
        int *sock_arg = (int *)malloc(sizeof(int));
        *sock_arg = clnt_sock;
        pthread_create(&tid, NULL, handle_clnt, (void *)sock_arg);
        pthread_detach(tid);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_addr.sin_addr)); // 客户端连接的ip地址
    }
    pthread_mutex_destroy(&mutex); // 销毁互斥锁
    close(serv_sock);
    return 0;
}

void *handle_clnt(void *arg)
{
    int clnt_sock = *((int *)arg);
    free(arg);
    int str_len = 0;
    char msg[BUF_SIZE];
    while ((str_len = read(clnt_sock, msg, BUF_SIZE)) > 0)
    {
        send_msg(msg, str_len);
    }
    // 处理失败的逻辑
    if (str_len == -1)
    {
        perror("read");
    }
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < clnt_cnt; i++)
    {
        if (clnt_sock == clnt_socks[i])
        {
            while (i < clnt_cnt - 1)
            {
                clnt_socks[i] = clnt_socks[i + 1];
                i++;
            }
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutex);
    close(clnt_sock);
    return NULL;
}

void send_msg(char *msg, int len)
{
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    pthread_mutex_unlock(&mutex);
}
