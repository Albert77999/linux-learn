#pragma once
#include "InetAddr.hpp"
#include "Common.hpp"
#include "Log.hpp"

#include <memory>

class Socket;
using SockPtr = std::shared_ptr<Socket>;

class Socket
{
public:
    virtual ~Socket() = default;
    virtual void SocketOrDie() = 0;
    virtual void SetSocketOpt() = 0;
    virtual bool BindOrDie(int port) = 0;
    virtual bool ListenOrDie() = 0;
    virtual void Close() = 0;
    virtual int Fd() = 0;
    virtual int Recv(std::string *out) = 0;
    virtual int Send(const std::string &in) = 0;
    virtual SockPtr Accepter(InetAddr *client,int *out_errno) = 0;
    void BuildTcpSocketMethod(int port)
    {
        SocketOrDie();
        SetSocketOpt();
        BindOrDie(port);
        ListenOrDie();
    }
};

class TcpSocket : public Socket
{
public:
    explicit TcpSocket(int sockfd = gdefaultsockfd)
        :_sockfd(sockfd)
    {

    }
    virtual ~TcpSocket() = default;
    virtual void SocketOrDie() override
    {
        _sockfd = socket(AF_INET,SOCK_STREAM,0);
        if(_sockfd < 0)
        {
            LOG(LogLevel::ERROR) << "Socket error";
            exit(SOCKET_ERR);
        }
        SetNonBlock(_sockfd);//设置非阻塞
        LOG(LogLevel::DEBUG) << "socket create success!";
    }
    virtual void SetSocketOpt() override
    {
        int opt = 1;
        if(setsockopt(_sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))!= 0)
        {
            LOG(LogLevel::ERROR) << "setsockopt error";
            exit(SET_SOCK_ERR);
        }
    }
    virtual bool BindOrDie(int port) override
    {
        if(_sockfd == gdefaultsockfd)
            return false;
        InetAddr addr(port);
        if(bind(_sockfd,addr.NetAddr(),addr.NetAddrLen()) != 0)
        {
            LOG(LogLevel::ERROR) << "bind error";
            exit(BIND_ERR);
        }
        LOG(LogLevel::DEBUG) << "bind success: " << _sockfd;
        return true;
    }
    virtual bool ListenOrDie() override
    {
        if(_sockfd == gdefaultsockfd)
            return false;
        if(listen(_sockfd,gbacklog) != 0)
        {
            LOG(LogLevel::ERROR) << "listen error";
            exit(LISTEN_ERR);
        }
        LOG(LogLevel::DEBUG) << "listen sucesss: " << _sockfd;
        return true;
    }
    virtual void Close() override
    {
        if(_sockfd == gdefaultsockfd)
            return;
        close(_sockfd);
    }
    virtual int Fd() override
    {
        return _sockfd;
    }
    virtual int Recv(std::string *out) override
    {
        char buffer[1024 * 8];
        ssize_t size = recv(_sockfd,buffer,sizeof(buffer) - 1,0);
        if(size > 0)
        {
            buffer[size] = 0;
            *out = buffer;
        }
        return size;
    }
    virtual int Send(const std::string &in) override
    {
        ssize_t size = send(_sockfd,in.c_str(),in.size(),0);
        return size;
    }
    virtual SockPtr Accepter(InetAddr *client,int *out_errno) override
    {
        if(!client)
            return nullptr;
        struct sockaddr_in temp;
        memset(&temp,0,sizeof(temp));
        socklen_t len = sizeof(temp);
        int newsockfd = accept(_sockfd,(struct sockaddr*)&temp,&len);
        *out_errno = errno;
        if(newsockfd < 0)
        {
            return nullptr;
        }
        client->SetAddr(temp,len);
        return std::make_shared<TcpSocket>(newsockfd);
    }
private:
    int _sockfd;
};