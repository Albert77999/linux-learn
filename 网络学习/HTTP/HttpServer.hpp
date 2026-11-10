#pragma once
#include "TcpServer.hpp"
#include "HttpProtocol.hpp"

using http_handler_t = std::function<void(HttpRequest &, HttpResponse &)>;

class HttpServer
{
private:
    // 处理http
    bool HandlerHttpRequest(SockPtr client_fd, InetAddr client)
    {
        LOG(LogLevel::DEBUG)
            << "HttpServer get a new client: "
            << client.Addr() << " Fd: " << client_fd->Fd();

        std::string http_request;
        std::size_t header_end;

        // 1. 接收数据，直到Header完整
        while (true)
        {
            std::string chunk;
            int n = client_fd->Recv(&chunk);

            if (n <= 0)
                return false;

            http_request += chunk;

            header_end = http_request.find("\r\n\r\n");
            if (header_end != std::string::npos)
                break;
        }

        // 2. 在Header中查找Content-Length
        std::size_t body_begin = header_end + 4;
        std::string header = http_request.substr(0, body_begin);

        std::string field = "\r\nContent-Length:";
        std::size_t pos = header.find(field);

        // 没有Content-Length时，先按没有Body处理
        std::size_t body_length = 0;

        if (pos != std::string::npos)
        {
            std::size_t value_begin = pos + field.size();
            std::size_t value_end = header.find("\r\n", value_begin);

            std::string value = header.substr(
                value_begin, value_end - value_begin);

            body_length = std::stoul(value);
        }

        // 3. 如果Body还没收齐，就继续接收
        while (http_request.size() - body_begin < body_length)
        {
            std::string chunk;
            int n = client_fd->Recv(&chunk);

            if (n <= 0)
                return false;

            http_request += chunk;
        }

        // 4. 截取当前这条完整请求
        http_request.resize(body_begin + body_length);

        // 5. 交给HttpRequest解析
        HttpRequest req;
        req.Deserialize(http_request);
        //开始处理resp
        HttpResponse resp;
        if(req.Args() != std::string())
        {
            std::cout << "This request has args" << std::endl;
            std::string service = req.Path();
            if(_route.find(service) != _route.end())
            {
                _route[service](req,resp);
            }
            else
            {
                resp.Build(req);
            }
        }
        else
        {
            std::cout << "This request doesn't has args" << std::endl;
            resp.Build(req);
        }
        std::string resp_str;
        resp.Serialize(&resp_str);
        client_fd->Send(resp_str);
        return true;
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