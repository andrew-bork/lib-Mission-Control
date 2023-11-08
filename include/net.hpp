#pragma once
/**
 * @file net.hpp
 * @brief netcode. This is just a lot of netcode. Supports unix and tcp sockets (hopefully).
 * @version 0.1
 * @date 2023-09-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */


#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <arpa/inet.h>
#include <netdb.h>

#include <memory>

#include <string>

#include <poll.h>
#include <thread>
#include <unordered_map>
#include <functional>
#include <vector>
// #include <iostream>

#define KB 1024
#define MB 1024*1024

#define chk_bit(a,b) ((a&b) == b)

namespace net {
    struct socket {
        int fd = -1;
        int i = -1;

        socket(int _fd);
        ~socket();


        void close();

        size_t operator>>(std::string& string);
        size_t operator<<(const std::string& string);
    };

    struct server {
        socket _socket;

        bool listening = false;
        std::thread * thread;

        server(int fd);
        ~server();

        void start_listening();

        std::vector<std::string> process_incoming();
        void broadcast(const std::string& message);

        int& fd();

        std::vector<pollfd> pollfds;

    };

    std::unique_ptr<server> create_server(const char * path);
    std::unique_ptr<server> create_server(int port);


    std::unique_ptr<socket> connect(const char * address, const char * port);
    std::unique_ptr<socket> connect(const char * address, int port);
    std::unique_ptr<socket> connect(const char * path);

};
