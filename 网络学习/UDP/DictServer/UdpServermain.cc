#include "UdpServer.hpp"
#include "Dictionary.hpp"
#include <memory>

int main(int argc,char *argv[])
{
    if(argc != 2)
    {
        LOG(LogLevel::ERROR) << "usage error : " << argv[0];
        Die(USAGE_ERR);
    }
    uint16_t port = std::stoi(argv[1]);

    std::shared_ptr<Dictionary> sptr = std::make_shared<Dictionary>();
    std::unique_ptr<UdpServer> uptr = std::make_unique<UdpServer>(
        [sptr](const std::string &word){
            return sptr->Translate(word);
        },port
    );
    //这里的sptr转换为成员变量，引用计数+1
    uptr->Init();
    uptr->Start();
    return 0;
}
