#pragma once
#include <string>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include "Common.hpp"
class InetAddr
{
public:
    InetAddr()
    {
        memset(&_addr, 0, sizeof(_addr));
        _ip = "";
        _port = 0;
    }
    explicit InetAddr(const struct sockaddr_in &addr)
    {
        _addr = addr;
        _port = ::ntohs(addr.sin_port);
        char ipbuffer[64];
        if (inet_ntop(AF_INET, &(addr.sin_addr), ipbuffer, sizeof(ipbuffer)))
        {
            _ip = ipbuffer;
        }
        else
        {
            _ip = "";
            perror("转换失败");
        }
    }
    explicit InetAddr(const std::string &ip, uint16_t port)
    {
        memset(&_addr, 0, sizeof(_addr));
        _addr.sin_family = AF_INET;
        _addr.sin_port = ::htons(port);
        int result = inet_pton(AF_INET, ip.c_str(), &(_addr.sin_addr));
        if (result == 0)
        {
            std::cerr << "invalid IPv4 address: " << ip << '\n';
            exit(ERROR);
        }
        else if (result < 0)
        {
            perror("inet_pton");
            exit(ERROR);
        }
        _port = port;
        _ip = ip;
    }
    explicit InetAddr(uint16_t port)
    {
        memset(&_addr, 0, sizeof(_addr));
        _addr.sin_family = AF_INET;
        _addr.sin_port = ::htons(port);
        _addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
        _port = port;
        _ip = "0.0.0.0";
    }
    const std::string &Ip() const
    {
        return _ip;
    }
    uint16_t Port() const
    {
        return _port;
    }
    sockaddr* NetAddr() 
    {
        return (struct sockaddr*)(&_addr);
    }
    const sockaddr* NetAddr() const
    {
        return (const struct sockaddr*)(&_addr);
    }
    socklen_t NetAddrLen() const
    {
        return sizeof(_addr);
    }
    ~InetAddr()
    {
    }

private:
    sockaddr_in _addr;
    uint16_t _port;
    std::string _ip;
};