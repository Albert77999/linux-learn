#pragma once
#include "Common.hpp"
#include "InetAddr.hpp"
#include "Log.hpp"

#define gdefaultport 8080
#define gdefaultip "127.0.0.1"

class TcpClient
{
public:
    explicit TcpClient(const std::string &ip = gdefaultip, uint16_t port = gdefaultport)
        : _sockfd(-1), _server_addr(ip, port)
    {
    }
    ~TcpClient()
    {
        if (_sockfd >= 0)
            close(_sockfd);
    }
    void Init()
    {
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_sockfd < 0)
        {
            LOG(LogLevel::ERROR) << "socket error,sockfd: " << _sockfd;
            Die(SOCKET_ERR);
        }
        if(connect(_sockfd,_server_addr.NetAddr(),_server_addr.NetAddrLen()) != 0)
        {
            LOG(LogLevel::ERROR) << "connect error";
            Die(CONT_ERR);
        }
        LOG(LogLevel::INFO) << "Init service success!";
    }
    void Start()
    {
        while(true)
        {
            std::string line;
            std::cout << "please enter(q/Q to quit):# ";
            if( !std::getline(std::cin,line)
                || line == "q" || line == "Q")
            {
                LOG(LogLevel::INFO) << "client quit success!";
                break;
            }
            ssize_t n = ::write(_sockfd,line.c_str(),line.size());
            if(n < 0)
            {
                //出错了,重来
                LOG(LogLevel::ERROR) << "write error()";
                break;
            }
            char buffer[1024];
            ssize_t read_n = read(_sockfd,buffer,sizeof(buffer) - 1);
            if(read_n < 0)
            {
                LOG(LogLevel::ERROR) << "read error()";
                break;
            }
            else if(read_n == 0)
            {
                LOG(LogLevel::INFO) << "server quit";
                break;
            }
            buffer[read_n] = '\0';
            std::cout << buffer << std::endl;
        }
    }

private:
    int _sockfd;
    InetAddr _server_addr;
};
