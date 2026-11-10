#pragma once
#include <jsoncpp/json/json.h>
#include <iostream>
#include <string>
#include <sstream>
#include <memory>

#include "Log.hpp"

const std::string gsep = "\r\n";

// len\r\n{json}\r\n
bool Encode(std::string &message)
{
    int size = message.size();
    if (size <= 0)
        return false;
    message = std::to_string(size) + gsep + message + gsep;
    return true;
}
bool Decode(std::string &message, std::string *content)
{
    std::string::size_type pos = message.find(gsep);
    if (pos == std::string::npos || pos <= 0)
        return false;
    int size = std::stoi(message.substr(0, pos));
    if (size <= 0)
        return false;
    if (size + gsep.size() * 2 + pos > message.size())
        return false;
    // 检查正文后的结束分隔符
    if (message.compare(gsep.size() + pos + size, gsep.size(), gsep) != 0)
        return false;
    *content = message.substr(gsep.size() + pos, size);
    message.erase(0, size + gsep.size() * 2 + pos); // 提取后删除报文
    return true;
}

// 定义两个结构体
class Request
{
public:
    explicit Request(int x = 0, int y = 0, char oper = '+')
        : _x(x), _y(y), _oper(oper)
    {
    }
    bool Serialize(std::string &output) const
    {
        Json::Value root;
        root["X"] = _x;
        root["Y"] = _y;
        root["Oper"] = _oper;
        Json::StreamWriterBuilder builder;
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        std::stringstream ss;
        if (writer->write(root, &ss) != 0)
        {
            LOG(LogLevel::ERROR) << "write error";
            return false;
        }
        output = ss.str();
        return true;
    }
    bool Deserialize(std::string &input)
    {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(input, root))
        {
            LOG(LogLevel::ERROR) << "parsr error, " << reader.getFormattedErrorMessages();
            return false;
        }
        _x = root["X"].asInt();
        _y = root["Y"].asInt();
        _oper = root["Oper"].asInt();
        return true;
    }
    ~Request()
    {
    }
    int X() const { return _x; }
    int Y() const { return _y; }
    char Oper() const { return _oper; }

private:
    int _x;
    int _y;
    char _oper;
};

class Response
{
public:
    explicit Response(int result = 0, int code = 0)
        : _result(result), _code(code)
    {
    }
    bool Serialize(std::string &output) const
    {
        Json::Value root;
        root["Result"] = _result;
        root["Code"] = _code;
        Json::StreamWriterBuilder builder;
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        std::stringstream ss;
        if (writer->write(root, &ss) != 0)
        {
            LOG(LogLevel::ERROR) << "write error";
            return false;
        }
        output = ss.str();
        return true;
    }
    bool Deserialize(std::string &input)
    {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(input, root))
        {
            LOG(LogLevel::ERROR) << "parsr error, " << reader.getFormattedErrorMessages();
            return false;
        }
        _result = root["Result"].asInt();
        _code = root["Code"].asInt();
        return true;
    }
    ~Response()
    {
    }
    void SetCode(int code) { _code = code; }
    void SetResult(int res) { _result = res; }
    int Result() const { return _result; }
    int Code() const { return _code; }

private:
    int _result;
    int _code;
};