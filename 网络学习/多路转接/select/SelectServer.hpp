#pragma once
#include "Log.hpp"
#include "Socket.hpp"

#include <memory>
#include <string>
#include <sys/select.h>
#include <stdlib.h>

#define NUM FD_SETSIZE
#define BUF_SIZE 1024

const int gdefaultfd = -1;

class SelectServer
{
public:
    SelectServer(uint16_t port)
        : _port(port), _listen_socket(std::make_unique<TcpSocket>())
    {
    }
    ~SelectServer()
    {
    }
    void Init()
    {
        _listen_socket->BuildTcpSocketMethod(_port);
        _fd_array[0] = _listen_socket->Fd();
        for (int i = 1; i < NUM; i++)
        {
            _fd_array[i] = gdefaultfd;
        }
    }
    void Start()
    {
        fd_set rdset;
        while (true)
        {
            FD_ZERO(&rdset);
            struct timeval timeout{10, 0};
            int maxfd = gdefaultfd;
            for (int i = 0; i < NUM; i++)
            {
                if (_fd_array[i] == gdefaultfd)
                    continue;
                maxfd = _fd_array[i] > maxfd ? _fd_array[i] : maxfd;
                FD_SET(_fd_array[i], &rdset);
            }
            int n = select(maxfd + 1, &rdset, nullptr, nullptr, &timeout);
            switch (n)
            {
            case 0:
                std::cout << "time out" << std::endl;
                break;
            case -1:
                perror("select");
                break;
            default:
                std::cout << "有事件就绪,timeout: " << timeout.tv_sec << std::endl;
                Dispatcher(rdset);
                break;
            }
        }
    }

private:
    void Dispatcher(fd_set &rdset)
    {
        for (int i = 0; i < NUM; i++)
        {
            if (_fd_array[i] == gdefaultfd)
                continue;
            else if (_fd_array[i] == _listen_socket->Fd())
            {
                // 可能出现不存在的情况，也就是监听套接字未就绪
                if (FD_ISSET(_fd_array[i], &rdset))
                {
                    Accepter();
                }
            }
            else
            {
                if (FD_ISSET(_fd_array[i], &rdset))
                {
                    Recver(i);
                }
            }
        }
    }
    void Accepter()
    {
        InetAddr client;
        SockPtr client_fd = _listen_socket->Accepter(&client);
        if (client_fd == nullptr)
            return;
        int newfd = client_fd->Fd();
        if (newfd < 0 || newfd >= FD_SETSIZE)
        {
            std::cout << "fd 超出 select 支持的范围: " << newfd << std::endl;
            client_fd->Close();
            return;
        }
        for (int i = 0; i < NUM; i++)
        {
            if (_fd_array[i] == gdefaultfd)
            {
                _fd_array[i] = newfd;
                return;
            }
        }
        std::cout << "服务器已经满了" << std::endl;
        close(newfd);
    }
    void Recver(int i)
    {
        int fd = _fd_array[i];
        char buffer[BUF_SIZE];
        ssize_t n = recv(fd, buffer, BUF_SIZE - 1, 0);
        if (n == 0)
        {
            // 客户端退出
            std::cout << "client quit!" << std::endl;
            _fd_array[i] = gdefaultfd;
            close(fd);
            return;
        }
        else if (n < 0)
        {
            // recv读取出错
            std::cout << "recv error,fd: " << fd << std::endl;
            _fd_array[i] = gdefaultfd;
            close(fd);
            return;
        }
        else
        {
            buffer[n] = 0;
            std::cout << "client says: " << buffer << std::endl;
            std::string message = "Echo: ";
            message += buffer;
            if (send(fd, message.c_str(), message.size(), 0) < 0)
            {
                std::cout << "send error,fd: " << fd << std::endl;
                return;
            }
        }
    }

private:
    uint16_t _port;
    std::unique_ptr<Socket> _listen_socket;
    int _fd_array[NUM];
};