#include "UdpClient.hpp"
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

    std::unique_ptr<UdpClient> uptr = std::make_unique<UdpClient>(ip,port);
    uptr->Init();
    uptr->Start();
    
    return 0;
}