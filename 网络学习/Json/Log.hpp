// 一条完整的信息: [2024-08-04 12:27:03] [DEBUG] [202938] [main.cc] [16] + 日志的可变部分(<< "hello world" << 3.14 << a << b;)
#pragma once
#include <iostream>
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <ctime>
#include <memory>
#include <string>

#include "Mutex.hpp"

using namespace LockModule;

inline std::string GetCurTime()
{
    time_t time_stamp = ::time(nullptr);
    struct tm cur_time;
    localtime_r(&time_stamp, &cur_time);

    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%4d-%02d-%02d %02d:%02d:%02d",
             cur_time.tm_year + 1900,
             cur_time.tm_mon + 1,
             cur_time.tm_mday,
             cur_time.tm_hour,
             cur_time.tm_min,
             cur_time.tm_sec);
    return buffer;
}
// 日志等级的处理
enum class LogLevel
{
    INFO,
    DEBUG,
    WARNING,
    ERROR
};

inline std::string level2string(LogLevel level)
{
    switch (level)
    {
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::WARNING:
        return "WARNING";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::INFO:
        return "INFO";
    default:
        return "unknown-level";
    }
}

#define gdefaultpath "./"
#define gdefaultname "log.txt"

// 刷新策略：分为文件刷新和控制台刷新
class Strategy
{
public:
    virtual ~Strategy() = default;
    virtual void SyncLog(const std::string &message) = 0;

private:
};

class ConsoleStrategy : public Strategy
{
public:
    virtual void SyncLog(const std::string &message) override
    {
        LockGuard lockguard(_mutex);
        std::cout << message << std::endl;
    }

private:
    Mutex _mutex;
};

class FileStrategy : public Strategy
{
public:
    FileStrategy(std::string filename = gdefaultname, std::string filepath = gdefaultpath)
        : _filename(filename), _filepath(filepath)
    {
    }
    virtual void SyncLog(const std::string &message) override
    {
        LockGuard lockguard(_mutex);
        std::string file = _filepath + _filename;
        std::fstream fs;
        fs.open(file.c_str(), std::ios::app);
        if (!fs)
        {
            std::cerr << "日志文件打开失败：" << file << '\n';
            return;
        }
        fs << message << std::endl; // endl 会换行并刷新缓冲区
        if (!fs)
        {
            std::cerr << "日志文件写入或刷新失败：" << file << '\n';
            return;
        }
        fs.close();
        if (!fs)
        {
            std::cerr << "日志文件关闭失败：" << file << '\n';
        }
    }

private:
    std::string _filename;
    std::string _filepath;
    Mutex _mutex;
};

// 日志的处理
//  一条完整的信息: [2024-08-04 12:27:03] [DEBUG] [202938] [main.cc] [16] + 日志的可变部分(<< "hello world" << 3.14 << a << b;)
class Log
{
public:
    Log()
        : _pid(::getpid())
    {
        _uptr = std::make_unique<ConsoleStrategy>();
    }
    void Convert2File()
    {
        LockGuard lockguard(_ptr_mutex);
        _uptr = std::make_unique<FileStrategy>();
    }
    void Convert2Console()
    {
        LockGuard lockguard(_ptr_mutex);
        _uptr = std::make_unique<ConsoleStrategy>();
    }
    void SyncLog2(const std::string &message)
    {
        LockGuard lockguard(_ptr_mutex);
        _uptr->SyncLog(message);
    }
    //_message的处理
    class LogMessage
    {
    public:
        LogMessage(Log &log, LogLevel level, std::string file, int line)
            : _log(log), _ss(), _curtime(GetCurTime()), _file(file), _line(line)
        {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), "[%s] [%s] [%d] [%s] [%d]",
                     _curtime.c_str(),
                     level2string(level).c_str(),
                     _log._pid,
                     _file.c_str(),
                     _line);
            _message = buffer;
            _message += "-";
        }
        //  重载处理
        template <typename T>
        LogMessage &operator<<(const T &t)
        {
            _ss << t;
            return *this;
        }
        ~LogMessage()
        {
            _message += _ss.str();
            _log.SyncLog2(_message);
        }

    private:
        Log &_log;
        std::stringstream _ss;
        std::string _message;
        std::string _curtime;
        std::string _file;
        int _line;
    };
    // 最后一步，将整体化为 () 处理
    LogMessage operator()(LogLevel level, const char *file, int line)
    {
        return LogMessage(*this, level, file, line);
    }
    ~Log()
    {
    }

private:
    int _pid;
    std::unique_ptr<Strategy> _uptr;
    Mutex _ptr_mutex;
};

inline Log logger;
#define LOG(Level) logger(Level, __FILE__, __LINE__)
#define ENABLE_CONSOLE_LOG() logger.Convert2Console()
#define ENABLE_FILE_LOG() logger.Convert2File()
