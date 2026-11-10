#pragma once
#include "Common.hpp"
#include "InetAddr.hpp"
#include "Log.hpp"
#include <pthread.h>
#include <unistd.h>

#define gdefaultport 8080
#define gdefaultip "127.0.0.1"

class UdpClient
{
private:
    static void *RecvMessage(void *args)
    {
        struct sockaddr_in temp_addr;
        UdpClient *uc = static_cast<UdpClient *>(args);
        while (true)
        {
            memset(&temp_addr, 0, sizeof(temp_addr));
            socklen_t temp_addr_sz = sizeof(temp_addr);
            char buffer[1024];
            ssize_t recv_n = ::recvfrom(uc->_sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&temp_addr, &temp_addr_sz);
            if (recv_n < 0)
            {
                LOG(LogLevel::ERROR) << "recvfrom error()";
                Die(RECV_ERR);
            }
            else if (recv_n == 0)
            {
                break;
            }
            buffer[recv_n] = '\0';
            InetAddr temp(temp_addr);
            std::string message = buffer;
            std::cout << message << std::endl;
        }
        return nullptr;
    }
    static void *SendMessage(void *args)
    {
        UdpClient *uc = static_cast<UdpClient *>(args);
        while (true)
        {
            std::string line;
            std::cout << "please enter(q/Q to quit):# ";
            if (!std::getline(std::cin, line) || line == "q" || line == "Q")
            {
                ssize_t n = ::sendto(uc->_sockfd, "QUIT", sizeof("QUIT") - 1, 0,
                                     uc->_server_addr.NetAddr(), uc->_server_addr.NetAddrLen());
                if (n < 0)
                {
                    LOG(LogLevel::ERROR) << "send QUIT failed";
                }
                break;
            }
            ssize_t n = ::sendto(uc->_sockfd, line.c_str(), line.size(), 0, uc->_server_addr.NetAddr(), uc->_server_addr.NetAddrLen());
            if (n < 0)
            {
                LOG(LogLevel::ERROR) << "sendto error()";
                break;
            }
        }
        return nullptr;
    }

public:
    explicit UdpClient(const std::string &ip = gdefaultip, uint16_t port = gdefaultport)
        : _sockfd(-1), _server_addr(ip, port)
    {
    }
    ~UdpClient()
    {
        if (_sockfd >= 0)
            close(_sockfd);
    }
    void Init()
    {
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_sockfd < 0)
        {
            LOG(LogLevel::ERROR) << "sockfd: " << _sockfd;
            Die(SOCKET_ERR);
        }
        if (connect(_sockfd, _server_addr.NetAddr(), _server_addr.NetAddrLen()) != 0)
        {
            LOG(LogLevel::ERROR) << "connect error";
            Die(CONT_ERR);
        }
    }
    void Start()
    {
        pthread_t tid1;
        pthread_t tid2;
        int ret1 = pthread_create(&tid1, nullptr, RecvMessage, (void *)this);
        if(ret1 != 0)
        {
            LOG(LogLevel::ERROR) << "pthread_create recv error,errno:" << ret1;
            Die(ERROR);
        }
        int ret2 = pthread_create(&tid2, nullptr, SendMessage, (void *)this);
        if(ret2 != 0)
        {
            LOG(LogLevel::ERROR) << "pthread_create send error,errno:" << ret2;
            ::shutdown(_sockfd,SHUT_RD);
            pthread_join(tid1,nullptr);
            Die(ERROR);
        }
        pthread_join(tid2, nullptr); // 等发送线程结束
        // 让接收线程从阻塞中醒来，并退出循环
        if (::shutdown(_sockfd, SHUT_RD) < 0)
        {
            LOG(LogLevel::ERROR) << "shutdown error";
            Die(RECV_ERR);
        }
        pthread_join(tid1, nullptr); // 等接收线程结束
    }

private:
    int _sockfd;
    InetAddr _server_addr;
};
