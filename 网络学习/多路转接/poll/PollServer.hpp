#pragma once
#include "Log.hpp"
#include "Socket.hpp"

#include <memory>
#include <string>
#include <cerrno>
#include <poll.h>
#include <stdlib.h>

#define MAX 512
#define BUF_SIZE 1024

const int gdefaultfd = -1;

class PollServer
{
public:
    PollServer(uint16_t port)
        : _port(port), _listen_socket(std::make_unique<TcpSocket>())
    {
    }
    ~PollServer()
    {
    }
    void Init()
    {
        _listen_socket->BuildTcpSocketMethod(_port);
        for(int i = 0;i < MAX;i++)
        {
            _fds[i].fd = gdefaultfd;
            _fds[i].events = 0;
            _fds[i].revents = 0;
        }
        _fds[0].fd = _listen_socket->Fd();
        _fds[0].events |= POLLIN;
        _fds[0].revents = 0;
    }
    bool Start()
    {
        int timeout = 1000;
        while (true)
        {
            int n = poll(_fds,MAX,timeout);
            switch (n)
            {
            case 0:
                std::cout << "time out" << std::endl;
                break;
            case -1:
                if (errno == EINTR)
                    break; // 被信号打断后重新等待
                perror("poll");
                return false; // 避免持续出错时空转
            default:
                std::cout << "有事件就绪" << std::endl;
                if (!Dispatcher())
                    return false;
                break;
            }
        }
    }

private:
    bool Dispatcher()
    {
        for (int i = 0; i < MAX; i++)
        {
            if (_fds[i].fd == gdefaultfd)
                continue;
            else if (_fds[i].fd == _listen_socket->Fd())
            {
                if (_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
                {
                    std::cerr << "监听套接字发生异常" << std::endl;
                    return false;
                }
                // 可能出现不存在的情况，也就是监听套接字未就绪
                if (_fds[i].revents & POLLIN)
                {
                    Accepter();
                }
            }
            else
            {
                if (_fds[i].revents & POLLNVAL)
                {
                    // 描述符已经无效，只移除记录，不再对它调用 close。
                    std::cerr << "无效的客户端 fd: " << _fds[i].fd << std::endl;
                    _fds[i] = {gdefaultfd, 0, 0};
                }
                else if (_fds[i].revents & (POLLIN | POLLHUP | POLLERR))
                {
                    // HUP 与可读可能同时出现；先读取剩余数据或连接结束状态。
                    Recver(i);
                }
            }
        }
        return true;
    }
    void Accepter()
    {
        InetAddr client;
        SockPtr client_fd = _listen_socket->Accepter(&client);
        if (client_fd == nullptr)
            return;
        int newfd = client_fd->Fd();
        if (newfd < 0)
        {
            std::cout << "无效的客户端 fd: " << newfd << std::endl;
            client_fd->Close();
            return;
        }
        for (int i = 0; i < MAX; i++)
        {
            if (_fds[i].fd == gdefaultfd)
            {
                _fds[i].fd = newfd;
                _fds[i].events = POLLIN;
                _fds[i].revents = 0;
                return;
            }
        }
        std::cout << "服务器已经满了" << std::endl;
        close(newfd);
    }
    void Recver(int i)
    {
        int fd = _fds[i].fd;
        char buffer[BUF_SIZE];
        ssize_t n = recv(fd, buffer, BUF_SIZE - 1, 0);
        if (n == 0)
        {
            // 客户端退出
            std::cout << "client quit!" << std::endl;
            close(fd);
            _fds[i] = {gdefaultfd, 0, 0};
            return;
        }
        else if (n < 0)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            // recv读取出错
            perror("recv");
            close(fd);
            _fds[i] = {gdefaultfd, 0, 0};
            return;
        }
        else
        {
            buffer[n] = 0;
            std::cout << "client says: " << buffer << std::endl;
            std::string message = "Echo: ";
            message.append(buffer, static_cast<std::size_t>(n));
            std::size_t sent = 0;
            while (sent < message.size())
            {
                // 避免 SIGPIPE 直接结束进程；短写时继续发送剩余数据。
                ssize_t written = send(fd, message.data() + sent, message.size() - sent, MSG_NOSIGNAL);
                if (written < 0 && errno == EINTR)
                    continue;
                if (written <= 0)
                {
                    if (written < 0)
                        perror("send");
                    close(fd);
                    _fds[i] = {gdefaultfd, 0, 0};
                    return;
                }
                sent += static_cast<std::size_t>(written);
            }
        }
    }

private:
    uint16_t _port;
    std::unique_ptr<Socket> _listen_socket;
    struct pollfd _fds[MAX];
};
