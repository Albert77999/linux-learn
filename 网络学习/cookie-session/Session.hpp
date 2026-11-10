#pragma once

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <sys/random.h>
#include <unordered_map>

// Cookie 里只保存随机编号；用户名、访问次数、失效时间保存在服务器。
struct Session
{
    using Clock = std::chrono::steady_clock;

    std::string username;
    Clock::time_point expires_at;
    std::uint64_t visits = 0;

    int RemainingSeconds() const
    {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            expires_at - Clock::now()).count();
        return ms <= 0 ? 0 : static_cast<int>((ms + 999) / 1000);
    }
};

class SessionManager
{
public:
    inline static constexpr const char *CookieName = "cpp_session_id";
    inline static constexpr int DefaultLifetime = 300;
    inline static constexpr int ShortLifetime = 5;

    // 登录总是生成新的编号，不接受客户端指定编号；旧会话在新会话建立后撤销。
    bool CreateSession(const std::string &username, const std::string &old_id,
                       std::string *new_id, int lifetime = DefaultLifetime)
    {
        if (username.empty() || new_id == nullptr || lifetime <= 0)
            return false;

        while (true)
        {
            std::string id;
            if (!GenerateId(&id))
                return false;

            std::lock_guard<std::mutex> guard(_mutex);
            RemoveExpired();
            const auto expires = Session::Clock::now() + std::chrono::seconds(lifetime);
            if (!_sessions.emplace(id, Session{username, expires, 0}).second)
                continue; // 极小概率的碰撞也不覆盖已有会话。

            if (old_id != id)
                _sessions.erase(old_id);
            *new_id = id;
            return true;
        }
    }

    bool SearchSession(const std::string &id, Session *result)
    {
        if (result == nullptr || !ValidId(id))
            return false;
        std::lock_guard<std::mutex> guard(_mutex);
        const auto it = _sessions.find(id);
        if (it == _sessions.end())
            return false;
        // 即使客户端强行携带已经过期的 Cookie，服务器也拒绝它。
        if (it->second.expires_at <= Session::Clock::now())
        {
            _sessions.erase(it);
            return false;
        }
        ++it->second.visits;
        *result = it->second; // 返回副本，解锁后不会拿着可能已被删除的指针。
        return true;
    }

    bool DeleteSession(const std::string &id)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        return _sessions.erase(id) != 0;
    }

private:
    static bool ValidId(const std::string &id)
    {
        if (id.size() != 64)
            return false;
        for (char ch : id)
            if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')))
                return false;
        return true;
    }

    static bool GenerateId(std::string *id)
    {
        // Linux 提供的随机字节源；不用时间戳、PID 或 rand() 生成登录凭据。
        std::array<unsigned char, 32> bytes{};
        std::size_t filled = 0;
        while (filled < bytes.size())
        {
            const ssize_t n = getrandom(bytes.data() + filled, bytes.size() - filled, 0);
            if (n > 0)
                filled += static_cast<std::size_t>(n);
            else if (n < 0 && errno == EINTR)
                continue;
            else
                return false;
        }
        constexpr char hex[] = "0123456789abcdef";
        id->clear();
        id->reserve(bytes.size() * 2);
        for (unsigned char byte : bytes)
        {
            *id += hex[byte >> 4];
            *id += hex[byte & 15];
        }
        return true;
    }

    // 调用者已经持锁。创建时顺便清理不再访问的过期会话，不另开清理线程。
    void RemoveExpired()
    {
        const auto now = Session::Clock::now();
        for (auto it = _sessions.begin(); it != _sessions.end();)
        {
            if (it->second.expires_at <= now)
                it = _sessions.erase(it);
            else
                ++it;
        }
    }

    std::unordered_map<std::string, Session> _sessions;
    std::mutex _mutex; // 多个连接线程共用同一个 Manager，查找、修改、删除都要保护。
};
