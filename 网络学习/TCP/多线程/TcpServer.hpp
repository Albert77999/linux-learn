#pragma once
#include "Common.hpp"
#include "InetAddr.hpp"
#include "Log.hpp"

#include <pthread.h>
#include <functional>

#define gdefaultfd -1
#define gdefaultport 8080

class TcpServer
{
private:
    static void *HandlerRequest(void *args)
    {
        int sockfd = *((int *)args);
        delete (int *)args;
        char buffer[1024];
        while (true)
        {
            memset(buffer, 0, sizeof(buffer));
            ssize_t n = read(sockfd, buffer, sizeof(buffer) - 1);
            if (n < 0)
            {
                // 出错
                LOG(LogLevel::ERROR) << "read error()";
                close(sockfd);
                break;
            }
            else if (n == 0)
            {
                // client端停止
                LOG(LogLevel::INFO) << "client quit";
                close(sockfd);
                break;
            }
            // 读取正常
            buffer[n] = 0;
            std::cout << "message from client : " << buffer << std::endl;
            std::string message = "echo #";
            message += buffer;
            n = write(sockfd, message.c_str(), message.size());
            if (n < 0)
            {
                LOG(LogLevel::ERROR) << "write error()";
                close(sockfd);
                break;
            }
        }
        return nullptr;
    }

public:
    explicit TcpServer(uint16_t port = gdefaultport)
        : _sockfd(gdefaultfd), _local_addr(port)
    {
    }
    void Init()
    {
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_sockfd < 0)
        {
            LOG(LogLevel::ERROR) << "socket error() sockfd: " << _sockfd;
            Die(SOCKET_ERR);
        }
        if (bind(_sockfd, _local_addr.NetAddr(), _local_addr.NetAddrLen()) != 0)
        {
            LOG(LogLevel::ERROR) << "bind error()";
            Die(BIND_ERR);
        }
        if (listen(_sockfd, 8) != 0)
        {
            LOG(LogLevel::ERROR) << "listen error()";
            Die(LISTEN_ERR);
        }
        LOG(LogLevel::INFO) << "Init serverice success!";
    }
    void Start()
    {
        struct sockaddr_in temp_addr;
        while (true)
        {
            memset(&temp_addr, 0, sizeof(temp_addr));
            socklen_t temp_addr_sz = sizeof(temp_addr);
            int newsockfd = accept(_sockfd, (struct sockaddr *)&temp_addr, &temp_addr_sz);
            if (newsockfd < 0)
            {
                LOG(LogLevel::ERROR) << "accept error,sockfd: " << newsockfd;
                Die(ACCEPT_ERR);
            }
            // 多线程处理
            pthread_t tid;
            int *into_sock = new int(newsockfd);
            int ret = pthread_create(&tid, nullptr, HandlerRequest, into_sock);
            if (ret != 0)
            {
                LOG(LogLevel::ERROR) << "pthread_create error: " << strerror(ret);
                delete into_sock;
                close(newsockfd);
                continue;
            }
            pthread_detach(tid);
        }
    }
    ~TcpServer()
    {
        if (_sockfd >= 0)
            close(_sockfd);
    }

private:
    int _sockfd;
    InetAddr _local_addr;
};