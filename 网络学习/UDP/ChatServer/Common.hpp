#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#define Die(code) do { std::exit(code); }while(0)

enum{
    USAGE_ERR = 1,
    SOCKET_ERR,
    BIND_ERR,
    CONT_ERR,
    RECV_ERR,
    SEND_ERR,
    ERROR
};