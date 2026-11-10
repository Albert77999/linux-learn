#include "Reactor.hpp"
#include "Listener.hpp"
#include "IOService.hpp"
#include "Connection.hpp"


int main(int argc,char *argv[])
{
    if(argc != 2)
    {
        std::cout << "usage error" << std::endl;
        return 1;
    }
    uint16_t port = std::stoi(argv[1]);
    auto conn = std::make_unique<Listener>(port);
    Reactor reactor;
    reactor.InsertConnection(std::move(conn));
    reactor.Loop();
    return 0;
}