#pragma once
#include "Common.hpp"
#include "InetAddr.hpp"
#include "Log.hpp"

#define gdefaultport 8080
#define gdefaultip "127.0.0.1"

class UdpClient
{
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
        if(connect(_sockfd,_server_addr.NetAddr(),_server_addr.NetAddrLen()) != 0)
        {
            LOG(LogLevel::ERROR) << "connect error";
            Die(CONT_ERR);
        }
    }
    void Start()
    {
        while(true)
        {
            std::string line;
            std::cout << "please enter(q/Q to quit):# ";
            std::getline(std::cin,line);
            if(line == "q" || line == "Q")
            {
                LOG(LogLevel::INFO) << "client quit success!";
                break;
            }
            ssize_t n = ::send(_sockfd,line.data(),line.size(),0);
            if(n < 0)
            {
                //出错了
                LOG(LogLevel::ERROR) << "send error()";
                Die(SEND_ERR);
            }

            char buffer[1024];
            ssize_t recv_n = ::recv(_sockfd,buffer,sizeof(buffer) - 1,0);
            if(recv_n < 0)
            {
                LOG(LogLevel::ERROR) << "recv error()";
                Die(RECV_ERR);
            }
            buffer[recv_n] = '\0';
            std::cout << "Server: " << buffer << std::endl;
        }
    }

private:
    int _sockfd;
    InetAddr _server_addr;
};
