#pragma once
#include "Common.hpp"
#include "InetAddr.hpp"
#include "Log.hpp"

#include <sys/wait.h>
#include <signal.h>

#define gdefaultfd -1
#define gdefaultport 8080

class TcpServer
{
private:
    void HandlerRequest(int sockfd)
    {
        char buffer[1024];
        while (true)
        {
            memset(buffer, 0, sizeof(buffer));
            ssize_t n = read(sockfd, buffer, sizeof(buffer) - 1);
            if (n < 0)
            {
                //出错
                LOG(LogLevel::ERROR) << "read error()";
                close(sockfd);
                break;
            }
            else if (n == 0)
            {
                //client端停止
                LOG(LogLevel::INFO) << "client quit";
                close(sockfd);
                break;
            }
            //读取正常
            buffer[n] = 0;
            std::cout << "message from client : " << buffer << std::endl;
            std::string message = "echo #";
            message += buffer;
            n = write(sockfd,message.c_str(),message.size());
            if(n < 0)
            {
                LOG(LogLevel::ERROR) << "write error()";
                close(sockfd);
                break;
            }
        }
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
        //子进程回收：
        //signal(SIGCHLD,SIG_IGN);
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
            //多进程解决：
            pid_t pid = fork();
            if(pid == 0)
            {
                //子进程
                close(_sockfd);
                if(fork() > 0)
                    exit(0);
                //孙子进程
                HandlerRequest(newsockfd);
                exit(0);
            }
            //父进程
            close(newsockfd);
            waitpid(pid,nullptr,0);
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