#pragma once
#include "Log.hpp"
#include "Socket.hpp"

#include <memory>
#include <string>
#include <cerrno>
#include <sys/epoll.h>
#include <stdlib.h>

#define BUF_SIZE 1024
#define EVENT_NUM 64

const int gdefaultfd = -1;

class EpollServer
{
public:
    EpollServer(uint16_t port)
        : _port(port), _epfd(gdefaultfd), _listen_socket(std::make_unique<TcpSocket>())
    {
    }
    ~EpollServer()
    {
        if (_epfd != gdefaultfd)
            close(_epfd);
        _listen_socket->Close();
    }
    void Init()
    {
        // 1.初始化
        _listen_socket->BuildTcpSocketMethod(_port);
        _epfd = epoll_create(1);
        if (_epfd < 0)
        {
            LOG(LogLevel::ERROR) << "epoll_create error";
            exit(EPOLL_CREATE_ERR);
        }
        LOG(LogLevel::DEBUG) << "epoll_create success! epfd: " << _epfd;
        // 2.添加listen_fd
        int listen_fd = _listen_socket->Fd();
        struct epoll_event ev{};
        ev.data.fd = listen_fd;
        ev.events = EPOLLIN;
        if (epoll_ctl(_epfd, EPOLL_CTL_ADD, listen_fd, &ev) != 0)
        {
            LOG(LogLevel::ERROR) << "epoll_ctl error";
            exit(EPOLL_CTL_ERR);
        }
        LOG(LogLevel::DEBUG) << "epoll add listen_fd success, listen_fd: " << listen_fd;
    }
    bool Start()
    {
        int timeout = -1;
        while (true)
        {
            int n = epoll_wait(_epfd, _events, EVENT_NUM, timeout);
            switch (n)
            {
            case 0:
                std::cout << "time out" << std::endl;
                break;
            case -1:
                if (errno == EINTR)
                    break; // 信号打断等待后，继续下一轮
                perror("epoll_wait");
                return false; // 其他错误退出，避免反复报错空转
            default:
                std::cout << "有事件就绪" << std::endl;
                if (!Dispatcher(n))
                    return false;
                break;
            }
        }
    }

private:
    bool Dispatcher(int num)
    {
        for (int i = 0; i < num; i++)
        {
            int events = _events[i].events;
            int fd = _events[i].data.fd;
            if (fd == _listen_socket->Fd())
            {
                if (events & (EPOLLERR | EPOLLHUP))
                {
                    std::cerr << "监听套接字发生异常" << std::endl;
                    return false;
                }
                if (events & EPOLLIN)
                {
                    Accepter();
                }
            }
            else
            {
                // HUP 可能和可读同时出现，尝试读取剩余数据或 EOF。
                if (events & (EPOLLIN | EPOLLHUP | EPOLLERR))
                {
                    Recver(fd);
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
        // 添加客户端套接字
        struct epoll_event ev{};
        ev.data.fd = newfd;
        ev.events = EPOLLIN;
        if (epoll_ctl(_epfd, EPOLL_CTL_ADD, newfd, &ev) != 0)
        {
            perror("epoll_ctl add client");
            client_fd->Close(); // 注册失败时关闭新连接，不影响其他客户端
            return;
        }
        LOG(LogLevel::DEBUG) << "epoll add newfd success,newfd: " << newfd;
    }
    void CloseClient(int fd)
    {
        if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr) != 0)
            perror("epoll_ctl del client");
        close(fd); // 即使删除失败，也要关闭连接
    }
    void Recver(int fd)
    {
        char buffer[BUF_SIZE];
        ssize_t n = recv(fd, buffer, BUF_SIZE - 1, 0);
        if (n == 0)
        {
            // 客户端退出
            std::cout << "client quit!" << std::endl;
            CloseClient(fd);
            return;
        }
        else if (n < 0)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            // 先报告 recv 的错误，再执行可能改变 errno 的清理操作。
            perror("recv");
            CloseClient(fd);
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
                ssize_t written = send(fd, message.data() + sent, message.size() - sent, MSG_NOSIGNAL);
                if (written > 0)
                {
                    sent += static_cast<std::size_t>(written);
                }
                else if (written < 0 && errno == EINTR)
                {
                    continue;
                }
                else
                {
                    if (written < 0)
                        perror("send");
                    CloseClient(fd);
                    return;
                }
            }
        }
    }

private:
    uint16_t _port;
    int _epfd;
    std::unique_ptr<Socket> _listen_socket;
    struct epoll_event _events[EVENT_NUM];
};
