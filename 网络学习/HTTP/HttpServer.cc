#include "HttpServer.hpp"
#include "Session.hpp"

#include <string>
#include <utility>
#include <vector>

namespace
{
// 表单中的空格可能写成+，其他字符可能按字节写成%HH。
int HexValue(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

std::string DecodeForm(const std::string &value)
{
    std::string result;
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '+')
            result += ' ';
        else if (value[i] == '%' && i + 2 < value.size() &&
                 HexValue(value[i + 1]) >= 0 && HexValue(value[i + 2]) >= 0)
        {
            result += static_cast<char>(HexValue(value[i + 1]) * 16 + HexValue(value[i + 2]));
            i += 2;
        }
        else
            result += value[i];
    }
    return result;
}

// 从 username=alice&password=123 这样的表单数据中取出指定参数。
std::string GetParam(const std::string &args, const std::string &name)
{
    std::size_t start = 0;
    while (start < args.size())
    {
        const auto end = args.find('&', start);
        const std::string item = args.substr(start, end - start);
        const auto equal = item.find('=');
        if (equal != std::string::npos && DecodeForm(item.substr(0, equal)) == name)
            return DecodeForm(item.substr(equal + 1));
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    return "";
}

// 用户输入放进HTML之前转义，防止被浏览器当作标签执行。
std::string EscapeHtml(const std::string &text)
{
    std::string result;
    for (char ch : text)
    {
        switch (ch)
        {
        case '&': result += "&amp;"; break;
        case '<': result += "&lt;"; break;
        case '>': result += "&gt;"; break;
        case '"': result += "&quot;"; break;
        case '\'': result += "&#39;"; break;
        default: result += ch;
        }
    }
    return result;
}

std::string LowerAscii(std::string text)
{
    for (char &ch : text)
    {
        if (ch >= 'A' && ch <= 'Z')
            ch = static_cast<char>(ch + ('a' - 'A'));
    }
    return text;
}

// 三个业务函数共用相同的HTTP响应格式。
void SetHtmlResponse(HttpResponse &resp, int code,
                     const std::string &title, const std::string &content)
{
    const std::string body =
        "<!DOCTYPE html><html lang=\"zh-CN\"><head><meta charset=\"UTF-8\">"
        "<title>" + EscapeHtml(title) + "</title></head><body><h1>" +
        EscapeHtml(title) + "</h1>" + content +
        "<p><a href=\"/\">返回首页</a></p></body></html>";
    resp.SetCode(code);
    resp.SetHeader("Content-Type", "text/html; charset=UTF-8");
    resp.SetBody(body);
}
}

void Login(HttpRequest &req, HttpResponse &resp)
{
    // req.Path(): /login
    // 根据req，动态构建username=lisi&password=abcdefg
    LOG(LogLevel::DEBUG) << "进入登录模块" << req.Path() << ", " << req.Args();
    std::string req_args = req.Args();
    // 1. 解析参数格式，得到要的参数

    // 2. 访问数据库，验证对应的用户是否是合法的用户，其他工作....
    // TODO
    // SessionManager m;
    // session_id = m.CreateSession(XXXXX);

    // 3. 登录成功
    std::string body = req.GetContent("wwwroot/success.html");
    resp.SetCode(200);
    resp.SetHeader("Content-Length", std::to_string(body.size()));
    resp.SetHeader("Content-Type", "text/html");
    resp.SetHeader("Set-Cookie", "usename=zhangsan&passwd=11111111");
    resp.SetBody(body);

    // resp.SetCode(302);
    // resp.SetHeader("Location", "/");
}
void Register(HttpRequest &req, HttpResponse &resp)
{
    LOG(LogLevel::DEBUG) << "进入注册模块 " << req.Path();
    const std::string username = GetParam(req.Args(), "username");
    const std::string password = GetParam(req.Args(), "password");

    if (username.empty() || password.empty())
    {
        SetHtmlResponse(resp, 400, "注册信息不完整", "<p>请填写用户名和密码。</p>");
        return;
    }

    // 目前没有数据库；这是表单接收演示，不保存账号或密码。
    SetHtmlResponse(resp, 200, "注册演示", "<p>已收到用户 <strong>" +
                    EscapeHtml(username) + "</strong> 的注册信息。</p>"
                    "<p>当前为演示，账号尚未保存。</p>");
}

void Search(HttpRequest &req, HttpResponse &resp)
{
    LOG(LogLevel::DEBUG) << "进入搜索模块 " << req.Path();
    const std::string keyword = GetParam(req.Args(), "keyword");
    if (keyword.empty())
    {
        SetHtmlResponse(resp, 400, "搜索内容为空", "<p>请输入搜索关键词。</p>");
        return;
    }

    // 先用几条固定数据演示搜索，后续可以替换成数据库查询。
    const std::vector<std::pair<std::string, std::string>> items = {
        {"HTTP", "应用层请求与响应协议"},
        {"TCP", "可靠的字节流传输协议"},
        {"UDP", "面向数据报的传输协议"},
        {"Linux", "操作系统与网络编程"},
        {"C++", "系统与网络程序开发"}};

    std::string content = "<p>搜索关键词：<strong>" + EscapeHtml(keyword) + "</strong></p><ul>";
    bool found = false;
    for (const auto &item : items)
    {
        if (LowerAscii(item.first).find(LowerAscii(keyword)) != std::string::npos ||
            item.second.find(keyword) != std::string::npos)
        {
            content += "<li>" + EscapeHtml(item.first) + "：" + EscapeHtml(item.second) + "</li>";
            found = true;
        }
    }
    content += "</ul>";
    if (!found)
        content += "<p>没有找到匹配内容。</p>";
    SetHtmlResponse(resp, 200, "搜索结果", content);
}

void Test(HttpRequest &req, HttpResponse &resp)
{
    LOG(LogLevel::DEBUG) << "进入测试模块 " << req.Path();
    const std::string message = GetParam(req.Args(), "message");
    SetHtmlResponse(resp, 200, "测试结果", "<p>收到的消息：<code>" +
                    EscapeHtml(message) + "</code></p>");
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " port" << std::endl;
        return 1;
    }
    auto httpserver = std::make_unique<HttpServer>(std::stoi(argv[1]));
    // 要让服务器具有登录功能
    httpserver->Register("/login", Login); // restful风格的网络请求接口
    httpserver->Register("/register", Register);
    httpserver->Register("/search", Search);
    httpserver->Register("/test", Test);
    httpserver->Start();
    return 0;
}
