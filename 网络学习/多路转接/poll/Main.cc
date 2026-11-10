#include "PollServer.hpp"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    uint16_t port = std::stoi(argv[1]);
    PollServer server(port);
    server.Init();
    return server.Start() ? 0 : 1;
}
