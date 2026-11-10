#include "TcpServer.hpp"
#include <memory>

int main(int argc,char *argv[])
{
    if(argc != 2)
    {
        LOG(LogLevel::ERROR) << "usage error : " << argv[0];
        Die(USAGE_ERR);
    }

    uint16_t port = std::stoi(argv[1]);
    std::unique_ptr<TcpServer> uptr = std::make_unique<TcpServer>(port);
    uptr->Init();
    uptr->Start();
    return 0;
}
