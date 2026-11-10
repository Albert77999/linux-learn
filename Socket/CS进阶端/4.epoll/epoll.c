#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define BUF_SIZE 1024
#define EPOLL_SIZE 50

void error_handling(char *message)
{
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}

int main(int argc, char* argv[])
{
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_sz = sizeof(clnt_addr);
    char buffer[BUF_SIZE];

    if(argc != 2)
    {
        printf("usage error : %s\n",argv[0]);
        exit(2);
    }

    int serv_sock = socket(AF_INET,SOCK_STREAM,0);
    if(serv_sock == - 1)
    {
        error_handling("socket error()");
    }
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1)
    {
        error_handling("bind error()");
    }

    if(listen(serv_sock,5) == -1)
    {
        error_handling("listen error()");
    }
    //处理epoll逻辑
    int epfd = epoll_create(EPOLL_SIZE);//填入的参数为操作系统参考
    struct epoll_event event;
    struct epoll_event events[EPOLL_SIZE];
    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd,EPOLL_CTL_ADD,serv_sock,&event);

    while(1)
    {
        int event_count = epoll_wait(epfd,events,EPOLL_SIZE,-1);
        if(event_count == -1)
        {
            error_handling("epoll_wait() error()");
        }

        for(int i = 0;i < event_count;i++)
        {
            int current_fd = events[i].data.fd;

            if(current_fd == serv_sock)
            {
                //accept新客户端
                clnt_addr_sz = sizeof(clnt_addr);
                int clnt_sock = accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_addr_sz);
                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd,EPOLL_CTL_ADD,clnt_sock,&event);
                printf("connected client : %d\n",clnt_sock);
            }
            else
            {
                //读取客户端数据
                int str_len = read(events[i].data.fd,buffer,BUF_SIZE);
                if(str_len < 0)
                {
                    perror("read");
                    epoll_ctl(epfd,EPOLL_CTL_DEL,current_fd,NULL);
                    close(current_fd);
                }
                else if(str_len == 0)
                {
                    epoll_ctl(epfd,EPOLL_CTL_DEL,current_fd,NULL);//从epoll中删除套接字
                    close(events[i].data.fd);
                    printf("closed client :%d\n",events[i].data.fd);
                }
                else
                {
                    write(current_fd,buffer,str_len);
                }
            }
        }
    }

    close(serv_sock);
    close(epfd);
    return 0;
}
