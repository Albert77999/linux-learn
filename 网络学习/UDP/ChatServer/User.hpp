#pragma once
#include "Log.hpp"
#include "InetAddr.hpp"
#include "Mutex.hpp"

#include <list>
#include <memory>

using namespace LockModule;

class UserInterFace
{
public:
    virtual ~UserInterFace() = default;
    virtual bool operator==(const InetAddr &id) = 0;
    virtual std::string Id() = 0;
    virtual bool SendTo(int sockfd, const std::string &message) = 0;
};

class User : public UserInterFace
{
public:
    explicit User(const InetAddr &id) // 禁止编译器偷偷调用构造函数，把其他类型隐式转换成当前类。
        : _id(id)
    {
    }
    bool operator==(const InetAddr &id) override
    {
        return _id == id;
    }
    std::string Id() override
    {
        return _id.Addr();
    }
    bool SendTo(int sockfd, const std::string &message) override
    {
        LOG(LogLevel::DEBUG) << "send message to user: " << _id.Addr() << " info:" << message;
        return sendto(sockfd, message.c_str(), message.size(), 0, _id.NetAddr(), _id.NetAddrLen()) >= 0;
    }
    ~User() override  = default;

private:
    InetAddr _id;
};

class UserManagement
{
public:
    UserManagement()
    {
    }
    void AddUser(InetAddr &id)
    {
        LockGuard lockguard(_mutex);

        for (auto &u : _users)
        {
            if (*u == id)
                return;
        }
        std::shared_ptr<UserInterFace> user = std::make_shared<User>(id);
        _users.emplace_back(user);
    }
    void DelUser(InetAddr &id)
    {
        LockGuard lockguard(_mutex);
        _users.remove_if([&id](const std::shared_ptr<UserInterFace> &user)
                         { return *user == id; });
    }
    void Router(int sockfd, std::string &message)
    {
        LockGuard lockguard(_mutex);
        for (auto &u : _users)
        {
            u->SendTo(sockfd, message);
        }
    }
    ~UserManagement()
    {
    }

private:
    std::list<std::shared_ptr<UserInterFace>> _users;
    Mutex _mutex;
};