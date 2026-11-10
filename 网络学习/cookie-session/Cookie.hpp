#pragma once

#include <cstring>
#include <ctime>
#include <stdexcept>
#include <string>
#include <unordered_map>

// Cookie 的文本格式与业务逻辑分开，方便对照 HTTP 请求头、响应头学习。
namespace CookieModule
{
struct Options
{
    // 默认策略统一放在这里；main() 和普通业务回调都不需要逐项设置。
    std::string path = "/";
    int max_age = 300;      // 默认 5 分钟；-1：会话 Cookie；0：删除。
    bool http_only = true;
    bool secure = false;   // 当前演示运行 HTTP，使用 HTTPS 时可设为 true。
    std::string same_site = "Lax";
    std::string domain;     // 默认不写 Domain，浏览器按当前主机保存。
};

inline std::string ExpireDate(int max_age)
{
    const std::time_t timestamp = std::time(nullptr) + (max_age == 0 ? -1 : max_age);
    std::tm utc{};
    gmtime_r(&timestamp, &utc);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &utc);
    return buffer;
}

inline std::string Trim(const std::string &text)
{
    const auto begin = text.find_first_not_of(" \t");
    if (begin == std::string::npos)
        return "";
    const auto end = text.find_last_not_of(" \t");
    return text.substr(begin, end - begin + 1);
}

inline int HexValue(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return -1;
}

// 中文、空格、分号等按字节编码，避免破坏 Cookie 的分隔格式；这不是加密。
inline std::string Encode(const std::string &value)
{
    const char *hex = "0123456789ABCDEF";
    std::string result;
    for (unsigned char ch : value)
    {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' || ch == '~')
            result += static_cast<char>(ch);
        else
        {
            result += '%';
            result += hex[ch >> 4];
            result += hex[ch & 15];
        }
    }
    return result;
}

inline std::string Decode(const std::string &value)
{
    std::string result;
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '%' && i + 2 < value.size() &&
            HexValue(value[i + 1]) >= 0 && HexValue(value[i + 2]) >= 0)
        {
            result += static_cast<char>(HexValue(value[i + 1]) * 16 + HexValue(value[i + 2]));
            i += 2;
        }
        else
            result += value[i]; // Cookie 中的 '+' 不按表单规则转换为空格。
    }
    return result;
}

// Cookie: demo_user=whb; demo_theme=blue
inline std::unordered_map<std::string, std::string> Parse(const std::string &header)
{
    std::unordered_map<std::string, std::string> cookies;
    std::size_t begin = 0;
    while (begin < header.size())
    {
        const auto end = header.find(';', begin);
        const std::string item = Trim(header.substr(begin, end - begin));
        const auto equal = item.find('='); // 仅用第一个等号拆分，值中仍可能有等号。
        if (equal != std::string::npos)
        {
            const auto name = Trim(item.substr(0, equal));
            if (!name.empty())
                cookies.emplace(name, Decode(Trim(item.substr(equal + 1))));
        }
        if (end == std::string::npos)
            break;
        begin = end + 1;
    }
    return cookies;
}

// 返回一条 Set-Cookie 的值；一条响应可以调用多次，逐条写入响应头。
inline std::string Serialize(const std::string &name, const std::string &value,
                             const Options &options)
{
    if (name.empty())
        throw std::invalid_argument("empty cookie name");
    for (unsigned char ch : name)
    {
        if (ch <= 32 || ch >= 127 || std::strchr("()<>@,;:\\\"/[]?={}", ch))
            throw std::invalid_argument("invalid cookie name");
    }
    if (options.path.empty() || options.path.front() != '/' ||
        options.path.find_first_of(";\r\n") != std::string::npos)
        throw std::invalid_argument("invalid cookie path");
    if (options.domain.find_first_of(";\r\n \t") != std::string::npos)
        throw std::invalid_argument("invalid cookie domain");
    if (options.same_site != "Lax" && options.same_site != "Strict" && options.same_site != "None")
        throw std::invalid_argument("invalid SameSite");
    if (options.same_site == "None" && !options.secure)
        throw std::invalid_argument("SameSite=None requires Secure");

    std::string result = name + "=" + Encode(value) + "; Path=" + options.path;
    if (!options.domain.empty())
        result += "; Domain=" + options.domain;
    if (options.max_age >= 0)
    {
        result += "; Max-Age=" + std::to_string(options.max_age);
        result += "; Expires=" + ExpireDate(options.max_age);
    }
    if (options.secure)
        result += "; Secure";
    if (options.http_only)
        result += "; HttpOnly";
    result += "; SameSite=" + options.same_site;
    return result;
}
}
