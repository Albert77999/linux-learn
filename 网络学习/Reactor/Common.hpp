#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fcntl.h>

#define Die(code) do { std::exit(code); }while(0)

enum{
    USAGE_ERR = 1,
    SOCKET_ERR,
    BIND_ERR,
    CONT_ERR,
    LISTEN_ERR,
    ACCEPT_ERR,
    SET_SOCK_ERR,
    EPOLL_CREATE_ERR,
    EPOLL_CTL_ERR,
    ERROR
};

const int gdefaultsockfd = -1;
const int gbacklog = 8;

void SetNonBlock(int sockfd)
{
    int fl = fcntl(sockfd,F_GETFL);
    if(fl < 0)
        return;
    fcntl(sockfd,F_SETFL,fl | O_NONBLOCK);
}