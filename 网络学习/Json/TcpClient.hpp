#pragma once
#include "Common.hpp"
#include "InetAddr.hpp"
#include "Log.hpp"
#include "Protocol.hpp"

#define gdefaultport 8080
#define gdefaultip "127.0.0.1"

class TcpClient
{
public:
    explicit TcpClient(const std::string &ip = gdefaultip, uint16_t port = gdefaultport)
        : _sockfd(-1), _server_addr(ip, port)
    {
    }
    ~TcpClient()
    {
        if (_sockfd >= 0)
            close(_sockfd);
    }
    void Init()
    {
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_sockfd < 0)
        {
            LOG(LogLevel::ERROR) << "socket error,sockfd: " << _sockfd;
            Die(SOCKET_ERR);
        }
        if (connect(_sockfd, _server_addr.NetAddr(), _server_addr.NetAddrLen()) != 0)
        {
            LOG(LogLevel::ERROR) << "connect error";
            Die(CONT_ERR);
        }
        LOG(LogLevel::INFO) << "Init service success!";
    }
    void Start()
    {
        std::string recv_message;
        while (true)
        {
            int x = 0;
            int y = 0;
            char oper = 0;
            std::string line;
            std::cout << "please enter(q/Q to quit):# ";
            if (!std::getline(std::cin, line) || line == "q" || line == "Q")
            {
                LOG(LogLevel::INFO) << "client quit success!";
                break;
            }

            std::stringstream ss(line);
            if (!(ss >> x >> oper >> y))
            {
                LOG(LogLevel::ERROR) << "input format error";
                continue;
            }

            // 处理发送逻辑
            Request req(x, y, oper);
            std::string send_message;
            if (!req.Serialize(send_message))
            {
                continue;
            }
            if (!Encode(send_message))
            {
                continue;
            }
            ssize_t n = send(_sockfd, send_message.c_str(), send_message.size(), 0);
            if (n < 0)
            {
                // 出错了,重来
                LOG(LogLevel::ERROR) << "send error()";
                break;
            }
            // 处理接收逻辑
            std::string content;
            while (!Decode(recv_message, &content))
            {
                char buffer[1024];
                ssize_t recv_n = recv(_sockfd, buffer, sizeof(buffer), 0);
                if (recv_n < 0)
                {
                    LOG(LogLevel::ERROR) << "recv error()";
                    return;
                }
                else if (recv_n == 0)
                {
                    LOG(LogLevel::INFO) << "server quit";
                    return;
                }
                recv_message.append(buffer, recv_n);
            }

            Response resp;
            if (!resp.Deserialize(content))
            {
                continue;
            }
            if (resp.Code() != 0)
            {
                std::cout << "Wrong Calculation" << std::endl;
                continue;
            }
            std::cout << "Calculation result: " << resp.Result() << std::endl;
        }
    }

private:
    int _sockfd;
    InetAddr _server_addr;
};
