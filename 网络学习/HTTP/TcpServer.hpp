#pragma once
#include "Socket.hpp"
#include <sys/wait.h>
#include <functional>

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
            //子进程处理
            pid_t id = fork();
            if(id == 0)
            {
                //子进程
                _listensockp->Close();
                if(fork() > 0)
                    exit(0);
                //孙子进程
                _handler(client_fd,client);
                exit(0);//退出自动被init接管
            }
            //父进程
            client_fd->Close();
            waitpid(id,nullptr,0);//回收立刻退出的子进程
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