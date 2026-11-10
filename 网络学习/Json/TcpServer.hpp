#pragma once
#include "Common.hpp"
#include "InetAddr.hpp"
#include "Log.hpp"
#include "ThreadPool.hpp"
#include "Calculator.hpp"

#include <functional>

#define gdefaultfd -1
#define gdefaultport 8080

using namespace ThreadPoolModule;

using func_t = std::function<void(std::string)>;

class TcpServer
{
private:
    void HandlerRequest(int sockfd)
    {
        std::string message;
        Calculator cal;
        char buffer[1024];
        while (true)
        {
            ssize_t n = recv(sockfd, buffer, sizeof(buffer), 0);
            if (n < 0)
            {
                // 出错
                LOG(LogLevel::ERROR) << "recv error()";
                break;
            }
            else if (n == 0)
            {
                // client端停止
                LOG(LogLevel::INFO) << "client quit";
                break;
            }
            // 读取正常
            message.append(buffer, n);
            //处理数据
            std::string content;
            while(Decode(message,&content))
            {
                Request req;
                if (!req.Deserialize(content))
                {
                    continue;
                }
                Response res = cal.Execute(req);
                std::string send_message;
                if (!res.Serialize(send_message))
                {
                    continue;
                }
                if(!Encode(send_message))
                {
                    continue;
                }
                if(send(sockfd,send_message.c_str(),send_message.size(),0) < 0)
                {
                    LOG(LogLevel::ERROR) << "send error()";
                    close(sockfd);
                    return;
                }
            }
        }
        close(sockfd);
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
        ThreadPool<func_t> *pool = ThreadPool<func_t>::getInstance();
        if (!pool->Start())
        {
            LOG(LogLevel::ERROR) << "threadpool start failed";
            return;
        }
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
            // 获取连接成功了
            LOG(LogLevel::INFO) << "accept success,sockfd is" << newsockfd;
            InetAddr temp(temp_addr);
            LOG(LogLevel::INFO) << "client info: " << temp.Addr();
            // 多线程处理:
            ::func_t fun = std::bind(&TcpServer::HandlerRequest, this, newsockfd);
            pool->Equeue(std::move(fun));
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
