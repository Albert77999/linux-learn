#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "usage: " << argv[0] << " <server_ipv4> <port>\n";
        return 1;
    }
    char* end = nullptr;
    errno = 0;
    long port = std::strtol(argv[2], &end, 10);
    if (errno != 0 || end == argv[2] || *end != '\0' || port < 1 || port > 65535)
    {
        std::cerr << "invalid port\n";
        return 1;
    }
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, argv[1], &server.sin_addr) != 1)
    {
        std::cerr << "invalid server IPv4\n";
        return 1;
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return 1;
    }
    if (connect(fd, reinterpret_cast<sockaddr*>(&server), sizeof(server)) == -1)
    {
        perror("connect");
        close(fd);
        return 1;
    }
    std::cout << "connected to " << argv[1] << ':' << port
              << "\nEnter text, or q/Q to close normally.\n";

    int result = 0;
    std::string line;
    while (std::cout << "> " << std::flush, std::getline(std::cin, line))
    {
        if (line == "q" || line == "Q")
            break;
        line += '\n';
        std::size_t sent = 0;
        while (sent < line.size())
        {
            ssize_t n = send(fd, line.data() + sent, line.size() - sent, MSG_NOSIGNAL);
            if (n > 0)
                sent += static_cast<std::size_t>(n);
            else if (n < 0 && errno == EINTR)
                continue;
            else
            {
                if (n == 0)
                    errno = EIO;
                perror("send");
                close(fd);
                return 1;
            }
        }

        // 服务端原样回声，所以累计读取到本次发送的字节数才算收完。
        std::string echoed(line.size(), '\0');
        std::size_t received = 0;
        while (received < echoed.size())
        {
            ssize_t n = recv(fd, echoed.data() + received, echoed.size() - received, 0);
            if (n > 0)
                received += static_cast<std::size_t>(n);
            else if (n < 0 && errno == EINTR)
                continue;
            else
            {
                if (n < 0)
                    perror("recv");
                else
                    std::cerr << "server closed before the complete echo\n";
                close(fd);
                return 1;
            }
        }
        if (echoed != line)
        {
            std::cerr << "echo does not match the sent data\n";
            close(fd);
            return 1;
        }
        std::cout << "[echo " << received << " bytes] " << echoed;
    }

    // 半关闭发送方向后仍读取，待服务端关闭，再关闭本端 fd。
    // 抓包中的 FIN 和 ACK 可能合并，不能要求一定恰好看到四个包。
    if (shutdown(fd, SHUT_WR) == -1)
    {
        perror("shutdown");
        result = 1;
    }
    else
    {
        char buffer[4096];
        while (true)
        {
            ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
            if (n > 0)
                std::cout.write(buffer, n);
            else if (n < 0 && errno == EINTR)
                continue;
            else
            {
                if (n < 0)
                {
                    perror("recv after shutdown");
                    result = 1;
                }
                break;
            }
        }
    }
    close(fd);
    std::cout << "connection closed\n";
    return result;
}
