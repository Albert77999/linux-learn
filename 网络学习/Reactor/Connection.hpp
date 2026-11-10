#pragma once

#include <string>
#include "InetAddr.hpp"

class Reactor;

class Connection
{
public:
    virtual void Sender() = 0;
    virtual void Recver() = 0;
    virtual void Excepter() = 0;
public:
    Connection()
        : _sockfd(-1)
    {
    }
    virtual ~Connection() //虚函数一定要重写析构函数
    {
        if (_sockfd >= 0)
            close(_sockfd);
    }
    void Close()
    {
        if (_sockfd >= 0)
        {
            close(_sockfd);
            _sockfd = -1;
        }
    }
    void SetSockfd(int sockfd)
    {
        _sockfd = sockfd;
    }
    void SetEvents(uint32_t events)
    {
        _events = events;
    }
    void SetPerrInfo(const InetAddr &peer_addr)
    {
        _peer_addr = peer_addr;
    }
    void SetOwner(Reactor *owner)
    {
        _owner = owner;
    }
    void AppendToIn(const std::string &in)
    {
        _inbuffer += in;
    }
    void AppendToOut(const std::string &out)
    {
        _outbuffer += out;
    }
    void DiscardOutBuffer(int n)
    {
        _outbuffer.erase(0, n);
    }
    bool IsOutBufferEmpty()
    {
        return _outbuffer.empty();
    }
    // 接口
    Reactor *GetOwner() { return _owner; }
    std::string &InBuffer() { return _inbuffer; }
    std::string &OutBuffer() { return _outbuffer; }
    uint32_t GetEvents() { return _events; }
    int SockFd() { return _sockfd; }

protected:
    int _sockfd;
    std::string _inbuffer;
    std::string _outbuffer;
    Reactor *_owner;
    InetAddr _peer_addr;
    uint32_t _events;
};