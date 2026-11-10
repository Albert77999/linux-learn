#include "SelectServer.hpp"

int main(int argc,char *argv[])
{
    if(argc != 2)
    {
        std::cout << "usage error()" << std::endl;
        return 1;
    }
    uint16_t port = std::stoi(argv[1]);
    std::unique_ptr<SelectServer> supr = std::make_unique<SelectServer>(port);
    supr->Init();
    supr->Start();
    return 0;
}