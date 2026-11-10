#pragma once
#include <memory>
#include "Reactor.hpp"
#include "Log.hpp"
#include "Connection.hpp"
#include "Socket.hpp"
#include "IOService.hpp"
#include "Calculator.hpp"

class Listener : public Connection
{
public:
    Listener(int port)
        : _listensock(std::make_unique<TcpSocket>()), _port(port)
    {
        _listensock->BuildTcpSocketMethod(_port);
        SetSockfd(_listensock->Fd());
        SetEvents(EPOLLIN | EPOLLET);
    }
    ~Listener()
    {
    }
    virtual void Sender() override
    {
    }
    virtual void Recver() override
    {
        while(true)
        {
            InetAddr client;
            int out_errno;
            SockPtr client_ptr = _listensock->Accepter(&client,&out_errno);
            if(client_ptr == nullptr)
            {
                //error
                if(out_errno == EAGAIN || out_errno == EWOULDBLOCK)
                {
                    LOG(LogLevel::DEBUG) << "accepte all connection done";
                    break;
                }
                else if(out_errno == EINTR)
                {
                    LOG(LogLevel::DEBUG) << "accepter interrupted by siganl,continue";
                    continue;
                }
                else
                {
                    LOG(LogLevel::ERROR) << "accepter error,ignore it";
                    break;
                }
            }
            //success
            int sockfd = client_ptr->Fd();
            SetNonBlock(sockfd);
            auto conn = std::make_unique<IOService>(sockfd);
            conn->RegisterOnMessage(HandlerRequest);
            GetOwner()->InsertConnection(std::move(conn));
            LOG(LogLevel::INFO) << "Add Connection success,sockfd: " << sockfd;
        }
    }
    virtual void Excepter() override
    {
    }

private:
    std::unique_ptr<Socket> _listensock;
    uint16_t _port;
};