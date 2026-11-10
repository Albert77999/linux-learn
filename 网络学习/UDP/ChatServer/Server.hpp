#pragma once
#include "Common.hpp"
#include "InetAddr.hpp"
#include "Log.hpp"
#include "User.hpp"
#include "ThreadPool.hpp"

#include <functional>

using namespace ThreadPoolModule;

#define gdefaultfd -1
#define gdefaultport 8080

using func_t = std::function<void(InetAddr &id)>;
using func_tt = std::function<void(int sockfd,std::string &message)>;
using task_t = std::function<void(std::string &name)>;

class UdpServer
{
public:
    explicit UdpServer(uint16_t port = gdefaultport)
        : _sockfd(gdefaultfd)
        , _local_addr(port)
    {
    }
    void Init()
    {
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_sockfd < 0)
        {
            LOG(LogLevel::ERROR) << "sockfd: " << _sockfd;
            Die(SOCKET_ERR);
        }
        if (bind(_sockfd, _local_addr.NetAddr(), _local_addr.NetAddrLen()) != 0)
        {
            LOG(LogLevel::ERROR) << "bind error()";
            Die(BIND_ERR);
        }
    }
    void Register(::func_t adduser,::func_t deluser,func_tt router)
    {
        _adduser = std::move(adduser);
        _deluser = std::move(deluser);
        _router = std::move(router);
    }
    void Start()
    {
        //线程池启动
        ThreadPool<task_t> *pool = ThreadPool<task_t>::getInstance();
        if (!pool->Start())
        {
            LOG(LogLevel::ERROR) << "线程池启动失败，服务器停止启动";
            Die(ERROR);
        }
        //线程池分配任务
        struct sockaddr_in temp_addr;
        char buffer[1024];
        while(true)
        {
            memset(buffer,0,sizeof(buffer));
            memset(&temp_addr,0,sizeof(temp_addr));
            socklen_t temp_addr_sz = sizeof(temp_addr);
            ssize_t n = ::recvfrom(_sockfd,buffer,sizeof(buffer) - 1,0,(struct sockaddr*)&temp_addr,&temp_addr_sz);
            if(n < 0)
            {
                //出错了
                LOG(LogLevel::ERROR) << "recvfrom error()";
                Die(RECV_ERR);
            }
            buffer[n] = 0;
            //重点来了：对不同的请求执行不同的操作
            InetAddr temp(temp_addr);
            std::string received = buffer;
            if(received == "QUIT")
            {
                _deluser(temp);
            }
            else
            {
                _adduser(temp);
            }
            std::string message = temp.Addr() + "says: " + received;
            task_t task = std::bind(_router,_sockfd,message);
            pool->Equeue(std::move(task));
        }
    }
    ~UdpServer()
    {
        if (_sockfd >= 0)
            close(_sockfd);
    }

private:
    int _sockfd;
    InetAddr _local_addr;
    //User处理函数
    ::func_t _adduser;
    ::func_t _deluser;
    func_tt _router;
};
