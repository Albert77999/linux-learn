#pragma once
#include "Socket.hpp"
#include <functional>
#include <thread>
#include <system_error>

using tcphandler_t = std::function<bool(SockPtr,InetAddr)>;

class TcpServer
{
public:
    explicit TcpServer(int port)
        :_port(port)
        ,_listensockp(std::make_unique<TcpSocket>())
    {
        
    }
    void InitServer(tcphandler_t handler)
    {
        _handler = handler;
        _listensockp->BuildTcpSocketMethod(_port);
    }
    void Loop()
    {
        while(true)
        {
            InetAddr client;
            SockPtr client_fd = _listensockp->Accepter(&client);
            if(client_fd == nullptr)
                continue;
            LOG(LogLevel::DEBUG) << "Get a new client, info is: " << client.Addr();
            // 本章改为每连接一个线程：线程共享 Session 表，fork 出的进程不会共享。
            // handler 和 socket 按值保存，局部 client_fd 离开本轮循环后仍然有效。
            try
            {
                std::thread([handler = _handler, client_fd, client]()
                {
                    try
                    {
                        handler(client_fd, client);
                    }
                    catch (const std::exception &)
                    {
                        LOG(LogLevel::ERROR) << "connection handler failed";
                    }
                    client_fd->Close(); // 线程用完后关闭；主线程不能提前关闭同一个 fd。
                }).detach();
            }
            catch (const std::system_error &)
            {
                LOG(LogLevel::ERROR) << "create connection thread failed";
                client_fd->Close();
            }
        }
    }
    ~TcpServer()
    {
        _listensockp->Close();
    }
private:
    int _port;
    std::unique_ptr<Socket> _listensockp;
    tcphandler_t _handler;
    
};
