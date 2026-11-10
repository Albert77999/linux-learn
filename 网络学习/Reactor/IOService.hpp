#pragma once
#include <functional>
#include "Log.hpp"
#include "Connection.hpp"
#include "Socket.hpp"
#include "Reactor.hpp"

using func_t = std::function<std::string(std::string &)>;

class IOService : public Connection
{
    static const int size = 1024;

public:
    IOService(int sockfd)
    {
        SetSockfd(sockfd);
        SetEvents(EPOLLIN | EPOLLET);
    }
    ~IOService()
    {
    }
    virtual void Sender() override
    {
        while (true)
        {
            ssize_t n = send(SockFd(), OutBuffer().c_str(), OutBuffer().size(), 0);
            if (n > 0)
            {
                DiscardOutBuffer(n); // 传输成功，删除前面的
            }
            else if (n == 0)
            {
                break; // 传输完了
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break; // 缓冲区写满了，下次再来
                }
                else if (errno == EINTR)
                {
                    continue; // 被信号打断了，重来
                }
                else
                {
                    return Excepter();
                }
            }
        }
        // 放在循环外，确保 EAGAIN 的 break 也会更新写关注。
        if (!IsOutBufferEmpty())
        {
            GetOwner()->EnableReadWrite(SockFd(), true, true);
        }
        else
        {
            GetOwner()->EnableReadWrite(SockFd(), true, false);
        }
    }
    virtual void Recver() override
    {
        while (true)
        {
            char buffer[size];
            ssize_t n = recv(SockFd(), buffer, sizeof(buffer) - 1, 0);
            if (n > 0)
            {
                buffer[n] = 0;
                AppendToIn(buffer);
            }
            else if (n == 0)
            {
                // 暂时理解为对方关闭理解（不考虑shutdown）
                return Excepter();
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break; // 缓冲区读不到更多数据了，下次再来
                }
                else if (errno == EINTR)
                {
                    continue; // 被信号打断了，重来
                }
                else
                {
                    return Excepter();
                }
            }
        }
        // 本轮事务做完了,要把buffer的数据导入到输入缓冲区中(out),同时需要确定应用层协议
        std::string result;
        // TODO(应用层协议): 后续复核/完善回调的完整报文判断、已处理输入消费及半包保留。
        // 不足一条时不产生新响应；输出缓冲非空只表示有数据待发送。
        if (_message_handler)
        {
            result = _message_handler(InBuffer());
        }
        AppendToOut(result); // 这里不一定就是完整的信息，所以后面要进一步判断
        if (!IsOutBufferEmpty())
        {
            // 只要有数据那就发送
            Sender();
        }
    }
    virtual void Excepter() override
    {
        LOG(LogLevel::ERROR) << "client may shutdown,goto error_handling" << SockFd();
        GetOwner()->DelConnection(SockFd());
    }
    void RegisterOnMessage(func_t message_handler)
    {
        _message_handler = message_handler;
    }

private:
    func_t _message_handler;
};
