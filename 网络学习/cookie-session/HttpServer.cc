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

// 业务回调共用相同的 HTML 响应格式。
void SetHtmlResponse(HttpResponse &resp, int code,
                     const std::string &title, const std::string &content)
{
    const std::string body =
        "<!DOCTYPE html><html lang=\"zh-CN\"><head><meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
        "<link rel=\"stylesheet\" href=\"/style.css\"><title>" + EscapeHtml(title) +
        "</title></head><body><header class=\"top\"><div class=\"shell nav\">"
        "<a class=\"brand\" href=\"/\"><span class=\"brand-icon\">CS</span>Cookie / Session</a>"
        "<nav class=\"nav-links\"><a href=\"/\">首页</a><a href=\"/cookie\">查看 Cookie</a>"
        "<a href=\"/session\">查看 Session</a>"
        "</nav></div></header><main class=\"shell\"><section class=\"page-card\"><h1>" +
        EscapeHtml(title) + "</h1>" + content +
        "<p><a class=\"back\" href=\"/\">← 返回实验首页</a></p></section></main></body></html>";
    resp.SetCode(code);
    resp.SetHeader("Content-Type", "text/html; charset=UTF-8");
    resp.SetBody(body);
}
}

// 返回 303，让浏览器在保存 Cookie 后用 GET 请求查看页面。
void RedirectToCookies(HttpResponse &resp, const std::string &path = "/cookie")
{
    resp.SetCode(303);
    resp.SetHeader("Location", path);
    resp.SetBody("");
}

bool RequireMethod(HttpRequest &req, HttpResponse &resp, const std::string &method)
{
    if (req.Method() == method)
        return true;
    resp.SetHeader("Allow", method);
    SetHtmlResponse(resp, 405, "请求方法不匹配", "<p>这个地址需要使用 " + method + " 请求。</p>");
    return false;
}

// 普通 Cookie 演示独立保留，不把客户端给出的用户名当作登录依据。
void SetDemoCookies(HttpRequest &req, HttpResponse &resp)
{
    if (!RequireMethod(req, resp, "POST")) return;
    // 演示内容直接写在服务端；各种属性由 Cookie.hpp 的默认配置提供。
    resp.SetCookie("demo_user", "whb");
    resp.SetCookie("demo_theme", "blue");
    RedirectToCookies(resp);
}

void ViewCookies(HttpRequest &req, HttpResponse &resp)
{
    if (!RequireMethod(req, resp, "GET")) return;
    std::string raw;
    // 不把登录凭据反射进 HTML，否则 JavaScript 可能绕过 HttpOnly 读到它。
    for (const auto &cookie : CookieModule::Parse(req.Header("Cookie")))
    {
        if (!raw.empty()) raw += "; ";
        raw += cookie.first + "=" +
               (cookie.first == SessionManager::CookieName ? "[已隐藏]" :
                CookieModule::Encode(cookie.second));
    }
    std::string content = "<p>当前路径：<code>" + EscapeHtml(req.Path()) + "</code></p>";
    content += raw.empty() ? "<div class=\"notice\">本次请求没有携带 Cookie。</div>" :
                            "<div class=\"notice\">浏览器已经在这次请求中自动携带 Cookie。</div>";
    content += "<h2>这次请求的 Cookie（登录凭据已隐藏）</h2><pre>" +
               EscapeHtml(raw.empty() ? "（空）" : "Cookie: " + raw) + "</pre>";
    content += "<h2>服务端解析后的值</h2><div class=\"table-wrap\"><table>"
               "<thead><tr><th>Cookie 名称</th><th>本次收到的值</th></tr></thead><tbody>";
    for (const std::string &name : {std::string("demo_user"), std::string("demo_theme"), std::string("demo_path")})
    {
        const auto value = req.Cookie(name);
        content += "<tr><td><code>" + name + "</code></td><td>" +
                   (value.empty() ? "未收到" : EscapeHtml(value)) + "</td></tr>";
    }
    content += "</tbody></table></div><p>Cookie 值的百分号编码会在这里还原，例如中文用户名。</p>";
    content += "<div class=\"links\"><a href=\"/cookie\">查看 /cookie</a>"
               "<a href=\"/a/b\">查看 /a/b</a><a href=\"/a/b/inside\">查看 /a/b/inside</a>"
               "<a href=\"/a/bc\">查看 /a/bc</a></div>"
               "<p>demo_path 的 Path 是 /a/b：浏览器应在 /a/b 和它的子路径携带它，"
               "在 /cookie 和 /a/bc 不携带它。</p>"
               "<p>页面展示的是本次请求收到的值；用户名 Cookie 可以由客户端修改，不作为登录凭据。</p>"
               "<form action=\"/cookie/clear\" method=\"post\"><button type=\"submit\">清除演示 Cookie</button></form>";
    SetHtmlResponse(resp, 200, "服务端收到了哪些 Cookie？", content);
}

void SetPathCookie(HttpRequest &req, HttpResponse &resp)
{
    if (!RequireMethod(req, resp, "POST")) return;
    CookieModule::Options options;
    options.path = "/a/b"; // 仅覆盖本实验需要改变的路径，其余属性使用默认值。
    resp.SetCookie("demo_path", "inside-a-b", options);
    RedirectToCookies(resp, "/a/b");
}

void ClearCookies(HttpRequest &req, HttpResponse &resp)
{
    if (!RequireMethod(req, resp, "POST")) return;
    CookieModule::Options options;
    options.max_age = 0;
    resp.SetCookie("demo_user", "", options);
    resp.SetCookie("demo_theme", "", options);
    // 删除时必须使用原先的 Path；根路径收到不到 demo_path，也能发出删除指令。
    options.path = "/a/b";
    resp.SetCookie("demo_path", "", options);
    RedirectToCookies(resp);
}

// 会话业务拥有唯一的 Manager；main 的所有相关回调共享同一个业务对象。
class SessionService
{
public:
    void Login(HttpRequest &req, HttpResponse &resp,
               int lifetime = SessionManager::DefaultLifetime)
    {
        if (!RequireMethod(req, resp, "POST")) return;
        std::string id;
        // 学习演示：固定用户 whb，模拟“账号认证已通过”，不实现账号数据库。
        // 真正的项目应先验证账号/密码，成功后才执行创建会话这一步。
        if (!_manager.CreateSession("whb", req.Cookie(SessionManager::CookieName), &id, lifetime))
        {
            SetHtmlResponse(resp, 500, "无法创建会话", "<p>请稍后重试。</p>");
            return;
        }
        CookieModule::Options options;
        options.max_age = lifetime; // 其余 Cookie 属性继续使用封装默认值。
        resp.SetCookie(SessionManager::CookieName, id, options);
        RedirectToCookies(resp, "/session");
    }

    void View(HttpRequest &req, HttpResponse &resp)
    {
        if (!RequireMethod(req, resp, "GET")) return;
        Session session;
        if (!RequireSession(req, resp, &session)) return;
        const std::string content =
            "<div class=\"notice\">已登录。用户名和访问次数来自服务器的 Session 表。</div>"
            "<div class=\"table-wrap\"><table><tbody>"
            "<tr><th>用户</th><td id=\"session-user\">" + EscapeHtml(session.username) + "</td></tr>"
            "<tr><th>本会话访问次数</th><td id=\"session-visits\">" + std::to_string(session.visits) + "</td></tr>"
            "<tr><th>剩余有效期</th><td>约 " + std::to_string(session.RemainingSeconds()) + " 秒</td></tr>"
            "</tbody></table></div>"
            "<p>刷新会建立新的 TCP 连接，但 Cookie 带回同一编号，仍能找到同一份 Session。"
            "访问次数统计 /session 与 /private 的成功请求。</p>"
            "<p>这是固定期限，刷新不会续期。编号不在页面或日志中显示，"
            "可在开发者工具中观察 Cookie 的保存和回传。</p>"
            "<div class=\"links\"><a href=\"/session\">刷新会话信息</a>"
            "<a href=\"/private\">访问需要登录的页面 →</a></div>"
            "<form action=\"/logout\" method=\"post\"><button type=\"submit\">退出登录</button></form>"
            "<p><a href=\"/login.html\">重新登录 / 5 秒过期实验 →</a></p>";
        SetHtmlResponse(resp, 200, "这份 Session 保存在服务器", content);
    }

    void Private(HttpRequest &req, HttpResponse &resp)
    {
        if (!RequireMethod(req, resp, "GET")) return;
        Session session;
        if (!RequireSession(req, resp, &session)) return;
        SetHtmlResponse(resp, 200, "受保护的页面",
                        "<div class=\"notice\">欢迎，" + EscapeHtml(session.username) +
                        "。服务端查到有效 Session，允许访问。</div>"
                        "<p>仅设置 demo_user=whb 无法进入这里；必须持有服务端认可的随机会话编号。</p>"
                        "<a class=\"button\" href=\"/session\">返回会话信息</a>");
    }

    void Logout(HttpRequest &req, HttpResponse &resp)
    {
        if (!RequireMethod(req, resp, "POST")) return;
        // 先撤销服务器状态，再通知浏览器删除凭据。旧 Cookie 重放也不能恢复会话。
        _manager.DeleteSession(req.Cookie(SessionManager::CookieName));
        ClearSessionCookie(resp);
        RedirectToCookies(resp, "/session");
    }

private:
    static void ClearSessionCookie(HttpResponse &resp)
    {
        CookieModule::Options options;
        options.max_age = 0;
        resp.SetCookie(SessionManager::CookieName, "", options);
    }

    bool RequireSession(HttpRequest &req, HttpResponse &resp, Session *session)
    {
        const auto id = req.Cookie(SessionManager::CookieName);
        if (_manager.SearchSession(id, session))
            return true;
        if (!id.empty())
            ClearSessionCookie(resp);
        SetHtmlResponse(resp, 401, "尚未登录，或会话已经失效",
                        "<p>本次请求没有可用的 Session，受保护内容不会返回。</p>"
                        "<a class=\"button\" href=\"/login.html\">前往演示登录 →</a>"
                        "<p>登录固定用户后，浏览器只保存一个随机编号，用户信息保存在服务端。</p>");
        return false;
    }

    SessionManager _manager;
};

void Register(HttpRequest &req, HttpResponse &resp)
{
    if (!RequireMethod(req, resp, "POST")) return;
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
    if (!RequireMethod(req, resp, "GET")) return;
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
    if (!RequireMethod(req, resp, "GET")) return;
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
    int port = 0;
    const std::string port_text = argv[1];
    const auto parsed = std::from_chars(port_text.data(), port_text.data() + port_text.size(), port);
    if (parsed.ec != std::errc() || parsed.ptr != port_text.data() + port_text.size() ||
        port < 1 || port > 65535)
    {
        std::cerr << "Invalid port\n";
        return 1;
    }
    auto httpserver = std::make_unique<HttpServer>(port);
    // shared_ptr 按值捕获：所有会话回调访问同一个对象，回调存活时对象也存活。
    auto sessions = std::make_shared<SessionService>();
    httpserver->Register("/login", [sessions](HttpRequest &req, HttpResponse &resp)
                         { sessions->Login(req, resp); });
    httpserver->Register("/session/login-short", [sessions](HttpRequest &req, HttpResponse &resp)
                         { sessions->Login(req, resp, SessionManager::ShortLifetime); });
    httpserver->Register("/session", [sessions](HttpRequest &req, HttpResponse &resp)
                         { sessions->View(req, resp); });
    httpserver->Register("/private", [sessions](HttpRequest &req, HttpResponse &resp)
                         { sessions->Private(req, resp); });
    httpserver->Register("/logout", [sessions](HttpRequest &req, HttpResponse &resp)
                         { sessions->Logout(req, resp); });
    httpserver->Register("/register", Register);
    httpserver->Register("/search", Search);
    httpserver->Register("/test", Test);
    httpserver->Register("/cookie", ViewCookies);
    httpserver->Register("/cookie/set", SetDemoCookies);
    httpserver->Register("/cookie/clear", ClearCookies);
    httpserver->Register("/cookie/path", SetPathCookie);
    httpserver->Register("/a/b", ViewCookies);
    httpserver->Register("/a/b/inside", ViewCookies);
    httpserver->Register("/a/bc", ViewCookies);
    httpserver->Start();
    return 0;
}
