#include "TcpClient.hpp"
#include <memory>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        LOG(LogLevel::ERROR) << "usage error : " << argv[0];
        Die(USAGE_ERR);
    }
    std::string ip = argv[1];
    uint16_t port = std::stoi(argv[2]);

    std::unique_ptr<TcpClient> uptr = std::make_unique<TcpClient>(ip,port);
    uptr->Init();
    uptr->Start();
    
    return 0;
}