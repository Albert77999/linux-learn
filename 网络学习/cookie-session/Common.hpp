#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#define Die(code) do { std::exit(code); }while(0)

enum{
    USAGE_ERR = 1,
    SOCKET_ERR,
    BIND_ERR,
    CONT_ERR,
    LISTEN_ERR,
    ACCEPT_ERR,
    SET_SOCK_ERR,
    ERROR
};

const int gdefaultsockfd = -1;
const int gbacklog = 8;