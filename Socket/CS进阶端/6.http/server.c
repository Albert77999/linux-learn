#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 4096
#define SMALL_BUF 256

static void error_handling(const char *message);
static void *request_handler(void *arg);
static void send_data(FILE *fp, const char *content_type, const char *file_name);
static const char *get_content_type(const char *file_name);
static void send_error(FILE *fp, int status, const char *reason);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 2;
    }

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket");

    int reuse_addr = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR,
                   &reuse_addr, sizeof(reuse_addr)) == -1)
        error_handling("setsockopt");

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons((unsigned short)atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) == -1)
        error_handling("bind");

    if (listen(serv_sock, 20) == -1)
        error_handling("listen");

    /* 客户端提前断开时，让写操作返回错误，不要让 SIGPIPE 终止整个服务器。 */
    signal(SIGPIPE, SIG_IGN);

    while (1)
    {
        struct sockaddr_in clnt_addr;
        socklen_t addr_size = sizeof(clnt_addr);
        int clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr,
                               &addr_size);
        if (clnt_sock == -1)
        {
            if (errno == EINTR)
                continue;
            perror("accept");
            continue;
        }

        int *socket_argument = malloc(sizeof(*socket_argument));
        if (socket_argument == NULL)
        {
            perror("malloc");
            close(clnt_sock);
            continue;
        }
        *socket_argument = clnt_sock;

        pthread_t thread_id;
        int state = pthread_create(&thread_id, NULL, request_handler,
                                   socket_argument);
        if (state != 0)
        {
            fprintf(stderr, "pthread_create: %s\n", strerror(state));
            free(socket_argument);
            close(clnt_sock);
            continue;
        }
        pthread_detach(thread_id);

        printf("Connection request: %s:%u\n",
               inet_ntoa(clnt_addr.sin_addr),
               (unsigned int)ntohs(clnt_addr.sin_port));
    }

    close(serv_sock);
    return 0;
}

static void *request_handler(void *arg)
{
    int clnt_sock = *(int *)arg;
    free(arg);

    FILE *clnt_read = fdopen(clnt_sock, "r");
    if (clnt_read == NULL)
    {
        perror("fdopen read");
        close(clnt_sock);
        return NULL;
    }

    int write_fd = dup(clnt_sock);
    if (write_fd == -1)
    {
        perror("dup");
        fclose(clnt_read);
        return NULL;
    }

    FILE *clnt_write = fdopen(write_fd, "w");
    if (clnt_write == NULL)
    {
        perror("fdopen write");
        close(write_fd);
        fclose(clnt_read);
        return NULL;
    }

    char request_line[BUF_SIZE];
    char header_line[BUF_SIZE];
    char method[16];
    char target[BUF_SIZE];
    char version[16];
    char file_name[BUF_SIZE];

    if (fgets(request_line, sizeof(request_line), clnt_read) == NULL)
        goto cleanup;

    if (sscanf(request_line, "%15s %4095s %15s",
               method, target, version) != 3 ||
        strncmp(version, "HTTP/", 5) != 0)
    {
        send_error(clnt_write, 400, "Bad Request");
        goto cleanup;
    }

    /* 第一版一次只处理一个请求，但仍然要把请求头读到空行。 */
    while (fgets(header_line, sizeof(header_line), clnt_read) != NULL)
    {
        if (strcmp(header_line, "\r\n") == 0 ||
            strcmp(header_line, "\n") == 0)
            break;
    }

    if (strcmp(method, "GET") != 0)
    {
        send_error(clnt_write, 405, "Method Not Allowed");
        goto cleanup;
    }

    if (target[0] != '/')
    {
        send_error(clnt_write, 400, "Bad Request");
        goto cleanup;
    }

    /* 查询参数不是文件名的一部分。 */
    char *query = strchr(target, '?');
    if (query != NULL)
        *query = '\0';

    /* 简单防止通过 URL 读取当前目录之外的文件。 */
    if (strstr(target, "..") != NULL)
    {
        send_error(clnt_write, 403, "Forbidden");
        goto cleanup;
    }

    if (strcmp(target, "/") == 0)
    {
        snprintf(file_name, sizeof(file_name), "index.html");
    }
    else
    {
        /* target + 1 跳过路径开头的 '/'。 */
        int written = snprintf(file_name, sizeof(file_name), "%s", target + 1);
        if (written < 0 || (size_t)written >= sizeof(file_name))
        {
            send_error(clnt_write, 414, "URI Too Long");
            goto cleanup;
        }
    }

    send_data(clnt_write, get_content_type(file_name), file_name);

cleanup:
    /* 套接字的读写端各有一个文件描述符，并且只在这里关闭一次。 */
    fclose(clnt_read);
    fclose(clnt_write);
    return NULL;
}

static void send_data(FILE *fp, const char *content_type,
                      const char *file_name)
{
    FILE *send_file = fopen(file_name, "rb");
    if (send_file == NULL)
    {
        send_error(fp, 404, "Not Found");
        return;
    }

    if (fseek(send_file, 0, SEEK_END) != 0)
    {
        fclose(send_file);
        send_error(fp, 500, "Internal Server Error");
        return;
    }

    long file_size = ftell(send_file);
    if (file_size < 0)
    {
        fclose(send_file);
        send_error(fp, 500, "Internal Server Error");
        return;
    }
    rewind(send_file);

    fprintf(fp,
            "HTTP/1.1 200 OK\r\n"
            "Server: Linux Web Server\r\n"
            "Content-Length: %ld\r\n"
            "Content-Type: %s\r\n"
            "Connection: close\r\n"
            "\r\n",
            file_size, content_type);

    char buffer[BUF_SIZE];
    size_t read_count;
    while ((read_count = fread(buffer, 1, sizeof(buffer), send_file)) > 0)
    {
        if (fwrite(buffer, 1, read_count, fp) != read_count)
            break;
    }

    fflush(fp);
    fclose(send_file);
}

static const char *get_content_type(const char *file_name)
{
    const char *extension = strrchr(file_name, '.');
    if (extension == NULL)
        return "application/octet-stream";

    if (strcasecmp(extension, ".html") == 0 ||
        strcasecmp(extension, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcasecmp(extension, ".css") == 0)
        return "text/css; charset=utf-8";
    if (strcasecmp(extension, ".js") == 0)
        return "application/javascript; charset=utf-8";
    if (strcasecmp(extension, ".jpg") == 0 ||
        strcasecmp(extension, ".jpeg") == 0)
        return "image/jpeg";
    if (strcasecmp(extension, ".png") == 0)
        return "image/png";
    if (strcasecmp(extension, ".txt") == 0)
        return "text/plain; charset=utf-8";

    return "application/octet-stream";
}

static void send_error(FILE *fp, int status, const char *reason)
{
    char body[SMALL_BUF];
    int body_length = snprintf(body, sizeof(body),
                               "<!doctype html>"
                               "<html><body><h1>%d %s</h1></body></html>\n",
                               status, reason);
    if (body_length < 0)
        return;

    fprintf(fp,
            "HTTP/1.1 %d %s\r\n"
            "Server: Linux Web Server\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n",
            status, reason, body_length);
    fwrite(body, 1, (size_t)body_length, fp);
    fflush(fp);
}

static void error_handling(const char *message)
{
    perror(message);
    exit(1);
}
