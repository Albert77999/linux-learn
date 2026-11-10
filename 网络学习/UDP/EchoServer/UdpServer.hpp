#pragma once
#include "Common.hpp"
#include "InetAddr.hpp"
#include "Log.hpp"

#define gdefaultfd -1
#define gdefaultport 8080

class UdpServer
{
public:
    explicit UdpServer(uint16_t port = gdefaultport)
        : _sockfd(gdefaultfd)
        , _local_addr(port)

    {
    }
    void Init()
    {
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_sockfd < 0)
        {
            LOG(LogLevel::ERROR) << "sockfd: " << _sockfd;
            Die(SOCKET_ERR);
        }
        if (bind(_sockfd, _local_addr.NetAddr(), _local_addr.NetAddrLen()) != 0)
        {
            LOG(LogLevel::ERROR) << "bind error()";
            Die(BIND_ERR);
        }
    }
    void Start()
    {
        struct sockaddr_in temp_addr;
        char buffer[1024];
        while(true)
        {
            memset(buffer,0,sizeof(buffer));
            memset(&temp_addr,0,sizeof(temp_addr));
            socklen_t temp_addr_sz = sizeof(temp_addr);
            ssize_t n = ::recvfrom(_sockfd,buffer,sizeof(buffer) - 1,0,(struct sockaddr*)&temp_addr,&temp_addr_sz);
            if(n < 0)
            {
                //出错了
                LOG(LogLevel::ERROR) << "recvfrom error()";
                Die(RECV_ERR);
            }
            buffer[n] = 0;
            InetAddr temp(temp_addr);
            std::string message;
            message = "Client-" + temp.Ip() + ":" + std::to_string(temp.Port()) + " says: " + buffer;
            std::cout << message << std::endl;
            ssize_t sent_n = ::sendto(_sockfd,buffer,n,0,temp.NetAddr(),temp.NetAddrLen());
            if(sent_n < 0)
            {
                //出错了
                LOG(LogLevel::ERROR) << "sendto error()";
                Die(SEND_ERR);
            }
        }
    }
    ~UdpServer()
    {
        if (_sockfd >= 0)
            close(_sockfd);
    }

private:
    int _sockfd;
    InetAddr _local_addr;
};
