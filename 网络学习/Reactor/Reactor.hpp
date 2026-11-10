#pragma once

#include <memory>
#include <unordered_map>
#include "Epoller.hpp"
#include "Connection.hpp"

using connection_t = std::unique_ptr<Connection>;

class Reactor
{
    static const int maxnum = 64;
private:
    bool IsConnectionExists(int sockfd)
    {
        return _connections.find(sockfd) == _connections.end() ? false : true;
    }
    void LoopOnce(int timeout)
    {
        int n = _epoller->Wait(_revs,maxnum,timeout);
        Dispatcher(n);
    }
    void Dispatcher(int n)
    {
        for(int i = 0;i < n;i++)
        {
            int sockfd = _revs[i].data.fd;
            uint32_t revents = _revs[i].events;
            if(revents & EPOLLERR || revents & EPOLLHUP)
            {
                revents = EPOLLIN | EPOLLOUT;
            }
            if(revents & EPOLLIN && IsConnectionExists(sockfd))
            {
                _connections[sockfd]->Recver();
            }
            if(revents & EPOLLOUT && IsConnectionExists(sockfd))
            {
                _connections[sockfd]->Sender();
            }
        }
    }
public:
    Reactor()
        :_epoller(std::make_unique<Epoller>())
        ,_isrunning(false)
    {
        _epoller->Init();
    }
    ~Reactor()
    {
    }
    void InsertConnection(connection_t conn)
    {
        const int sockfd = conn->SockFd();
        if(!IsConnectionExists(sockfd))
        {
            _connections.insert({sockfd,std::move(conn)});
            // conn 已移交所有权，后续通过管理表访问连接。
            _epoller->Add(sockfd,_connections[sockfd]->GetEvents());
            _connections[sockfd]->SetOwner(this);
        }
    }
    void DelConnection(int sockfd)
    {
        if(IsConnectionExists(sockfd))
        {
            _epoller->Del(sockfd);
            //下面两个的顺序不能变
            _connections[sockfd]->Close();
            _connections.erase(sockfd);
        }
    }
    void EnableReadWrite(int sockfd,bool readable,bool writeable)
    {
        if(IsConnectionExists(sockfd))
        {
            uint32_t events = (readable ? EPOLLIN : 0) | (writeable ? EPOLLOUT : 0) | EPOLLET;
            _connections[sockfd]->SetEvents(events);
            _epoller->Update(sockfd,_connections[sockfd]->GetEvents());
        }
    }
    void Loop()
    {
        _isrunning = true;
        int timeout = -1;
        while(_isrunning)
        {
            LoopOnce(timeout);
        }
        _isrunning = false;
    }
    void Stop()
    {
        _isrunning = false;
    }
private:
    std::unordered_map<int,connection_t> _connections;
    std::unique_ptr<Epoller> _epoller;
    bool _isrunning;
    struct epoll_event _revs[maxnum];
};
