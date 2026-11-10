#pragma once

#include <sys/epoll.h>
#include <iostream>
#include "Common.hpp"
#include "Log.hpp"

class Epoller
{
private:
    void Ctrl(int sockfd, uint32_t events, int flag)
    {
        struct epoll_event ev{};
        ev.data.fd = sockfd;
        ev.events = events;
        if (epoll_ctl(_epfd, flag, sockfd, &ev) != 0)
        {
            LOG(LogLevel::ERROR) << "epoll_ctl error";
            exit(EPOLL_CTL_ERR);
        }
    }

public:
    Epoller()
        : _epfd(-1)
    {
    }
    ~Epoller()
    {
        if (_epfd >= 0)
            close(_epfd);
    }
    void Init()
    {
        _epfd = epoll_create(1);
        if (_epfd < 0)
        {
            LOG(LogLevel::ERROR) << "epoll_create error";
            exit(EPOLL_CREATE_ERR);
        }
        LOG(LogLevel::INFO) << "epoll_create success,epfd: " << _epfd;
    }
    int Wait(struct epoll_event revs[], int maxnum, int timeout)
    {
        int n = epoll_wait(_epfd, revs, maxnum, timeout);
        if (n < 0)
        {
            LOG(LogLevel::ERROR) << "epoll_wait error";
        }
        return n;
    }
    void Add(int sockfd, uint32_t events)
    {
        Ctrl(sockfd, events, EPOLL_CTL_ADD);
    }
    void Update(int sockfd, uint32_t events)
    {
        Ctrl(sockfd, events, EPOLL_CTL_MOD);
    }
    void Del(int sockfd)
    {
        Ctrl(sockfd, 0, EPOLL_CTL_DEL);
    }
private:
    int _epfd;
};