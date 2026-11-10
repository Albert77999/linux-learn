#pragma once
#include "TcpServer.hpp"
#include "HttpProtocol.hpp"
#include <charconv>

using http_handler_t = std::function<void(HttpRequest &, HttpResponse &)>;

class HttpServer
{
private:
    bool SendError(SockPtr client, int code, const std::string &message)
    {
        HttpResponse resp;
        resp.SetCode(code);
        resp.SetHeader("Content-Type", "text/plain; charset=UTF-8");
        resp.SetHeader("Connection", "close");
        resp.SetBody(message + "\n");
        std::string response;
        resp.Serialize(&response);
        client->Send(response);
        return false;
    }

    bool HandlerHttpRequest(SockPtr client_fd, InetAddr client)
    {
        LOG(LogLevel::DEBUG) << "HttpServer get a new client: " << client.Addr();
        const std::size_t header_limit = 64 * 1024;
        const std::size_t body_limit = 1024 * 1024;
        std::string http_request;
        std::size_t header_end;

        // 仍然保持你的约定：先接收完整请求，再交给 HttpRequest 解析。
        while ((header_end = http_request.find("\r\n\r\n")) == std::string::npos)
        {
            std::string chunk;
            if (client_fd->Recv(&chunk) <= 0)
                return false;
            http_request += chunk;
            if (http_request.find("\r\n\r\n") == std::string::npos &&
                http_request.size() > header_limit)
                return SendError(client_fd, 431, "Header too large");
        }
        if (header_end > header_limit)
            return SendError(client_fd, 431, "Header too large");

        const std::size_t body_begin = header_end + 4;
        std::istringstream headers(http_request.substr(0, header_end));
        std::string line;
        std::getline(headers, line); // 跳过请求行。
        std::size_t body_length = 0;
        bool has_length = false;
        while (std::getline(headers, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            std::string key, value;
            if (!SplitString(line, &key, &value))
                return SendError(client_fd, 400, "Invalid header");
            key = LowerHeaderName(key);
            // 本学习版使用 Content-Length，不处理 chunked 请求体。
            if (key == "transfer-encoding")
                return SendError(client_fd, 400, "Transfer-Encoding is not supported");
            if (key != "content-length")
                continue;
            if (has_length || value.empty())
                return SendError(client_fd, 400, "Invalid Content-Length");
            has_length = true;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), body_length);
            if (result.ec != std::errc() || result.ptr != value.data() + value.size())
                return SendError(client_fd, 400, "Invalid Content-Length");
            if (body_length > body_limit)
                return SendError(client_fd, 413, "Body too large");
        }

        while (http_request.size() - body_begin < body_length)
        {
            std::string chunk;
            if (client_fd->Recv(&chunk) <= 0)
                return false;
            http_request += chunk;
        }
        http_request.resize(body_begin + body_length);

        HttpRequest req;
        if (!req.Deserialize(http_request))
            return SendError(client_fd, 400, "Invalid request line");
        HttpResponse resp;
        // Cookie 查看、清除不一定有参数，因此直接按 Path 查路由。
        const auto route = _route.find(req.Path());
        if (route != _route.end())
            route->second(req, resp);
        else if (req.Method() == "GET")
            resp.Build(req);
        else
        {
            resp.SetCode(405);
            resp.SetHeader("Allow", "GET");
            resp.SetHeader("Content-Type", "text/plain; charset=UTF-8");
            resp.SetBody("Method Not Allowed\n");
        }
        resp.SetHeader("Connection", "close");
        resp.SetHeader("Cache-Control", "no-store");
        std::string response;
        resp.Serialize(&response);
        return client_fd->Send(response) >= 0;
    }

public:
    HttpServer(int port)
        : _tupr(std::make_unique<TcpServer>(port))
    {
    }
    ~HttpServer()
    {
    }
    void Register(std::string func_name, http_handler_t func)
    {
        _route[func_name] = func;
    }
    void Start()
    {
        _tupr->InitServer([this](SockPtr client_fd, InetAddr client)
                          { return this->HandlerHttpRequest(client_fd, client); });
        _tupr->Loop();
    }

private:
    std::unique_ptr<TcpServer> _tupr;
    std::unordered_map<std::string, http_handler_t> _route;
};