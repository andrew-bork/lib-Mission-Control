#include <net.hpp>

void net::socket::close() {
    if(fd != -1) {
        ::close(fd);
        fd = -1;
    }
}

int& net::server::fd() {
    return _socket.fd;
}


net::server::server(int _fd) : _socket(_fd) {
    pollfds.resize(1);
    pollfds[0].fd = _fd;
    pollfds[0].events = POLLIN;
}

net::socket::socket(int _fd) : fd(_fd) {

}

net::server::~server() {

}

void net::server::start_listening() {
    int success = ::listen(fd(), 16);
    if(success < 0) {
        throw std::runtime_error("Couldn't listen.");
    }
}

void net::server::broadcast(const std::string& s) {
    for(size_t i = 1; i < pollfds.size(); i ++) {
        if(!chk_bit(pollfds[i].fd, POLLHUP)){
            write(pollfds[i].fd, &s[0], s.size());
        }
    }
}

std::vector<std::string> net::server::process_incoming() {
    std::vector<std::string> out;

    int& server_fd = fd();

    int n_ready = poll(&pollfds[0], pollfds.size(), 0);
    if(n_ready < 0) {
        throw std::runtime_error("Polling failed");
    }

    if(chk_bit(pollfds[0].revents, POLLIN)) {
        int client_fd = accept(server_fd, NULL, NULL);
        if(client_fd < 0) {
            throw std::runtime_error("Accept failed");
        }

        pollfd _pollfd;
        _pollfd.fd = client_fd;
        _pollfd.events = POLLIN;
        
        pollfds.push_back(_pollfd);
        printf("client connected: %d\n", client_fd);

        // std::unique_ptr<net::socket> client = std::make_unique<net::socket>(client_fd);

        // connections[client_fd] = std::move(client);
    }

    size_t n_closed_fds = 0;
    for(size_t i = 1; i < pollfds.size(); i ++) {
        if(chk_bit(pollfds[i].revents, POLLNVAL)) n_closed_fds++;
        else if(chk_bit(pollfds[i].revents, POLLHUP)) {
            n_closed_fds++;
        }else if(chk_bit(pollfds[i].revents, POLLIN)) {
            int fd = pollfds[i].fd;
            printf("Reading from %d\n", fd);

            // std::unique_ptr<net::socket>& client = connections[fd];

            char buf[4096];
            size_t n_bytes = recv(fd, buf, 4095, 0);
            // if(n_bytes == -1) {
            //     // throw std::runtime_error("Error with recv");
            // }
            buf[n_bytes] = '\0';
            
            printf("client message: \"%s\"\n", buf);
            out.emplace_back(buf);
            // std::string data(buf);
        }
    }


    if(n_closed_fds > 0) {
        printf("Cleaning up dead sockets\n");
        for(size_t i = pollfds.size() - 1; i >= 1; i --) {
            if(chk_bit(pollfds[i].revents, POLLNVAL) || chk_bit(pollfds[i].revents, POLLHUP)) {
                // connections.erase(pollfds[i].fd);
                close(pollfds[i].fd);
                pollfds.erase(pollfds.begin() + i);
            }
        }
    }

    return out;
}

net::socket::~socket() {
    close();
}

size_t net::socket::operator<<(const std::string& string) {
    size_t n_bytes = string.size();
    size_t s = send(fd, &string[0], n_bytes, 0);
    if(s == -1) {
        // throw std::runtime_error("Something went wrong with send.");
    }
    return string.size() - n_bytes;
}

size_t net::socket::operator>>(std::string& string) {
    string = "";
    char buf[4096];
    
    pollfd _pollfd;
    _pollfd.fd = fd;
    _pollfd.events = POLLIN;

    int success = poll(&_pollfd, 1, -1);
    if(success < 0) {
        throw std::runtime_error("Poll failed");
    } 

    while(chk_bit(_pollfd.revents, POLLIN)) {
        
        size_t s = recv(fd, buf, 4095, 0);
        if(s == -1) {
            // throw std::runtime_error("Recv failed");
        }

        buf[s] = '\0';
        
        string += buf;

        success = poll(&_pollfd, 1, 0);
        if(success == -1) {
            throw std::runtime_error("Poll failed");
        }
    }

    return string.size();
}


std::unique_ptr<net::server> net::create_server(const char * path) {
    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path));

    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd < 0) {
        throw std::runtime_error("Couldn't create a socket");
    }
    
    int success = ::bind(fd, (sockaddr *) &addr, sizeof(addr));
    if(success < 0) {
        throw std::runtime_error("Couldn't connect");
    }

    return std::make_unique<net::server>(fd);
}

std::unique_ptr<net::server> net::create_server(int port) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        throw std::runtime_error("Couldn't create a socket");
    }


    int success = ::bind(fd, (sockaddr *) &addr, sizeof(addr));
    if(success < 0) {
        throw std::runtime_error("Couldn't bind");
    }
    
    return std::make_unique<net::server>(fd);
}

void* get_in_addr(sockaddr *s) {
    if(s->sa_family == AF_INET) return &((sockaddr_in *) s)->sin_addr;
    else return &((sockaddr_in6 *) s)->sin6_addr;
}

std::unique_ptr<net::socket> net::connect(const char * address, const char * port) {
    addrinfo hints;
    addrinfo *results;

    memset(&hints, 0, sizeof(hints)); 

    hints.ai_family = AF_INET;
    hints.ai_socktype= SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    int success = getaddrinfo(address, port, &hints, &results);

    int fd = -1;
    for(addrinfo * curr = results; curr != NULL; curr = curr->ai_next) {

        // char host[NI_MAXHOST];
        // char port[NI_MAXSERV];

        // if(getnameinfo(curr->ai_addr, curr->ai_addrlen, host, sizeof(host), port, sizeof(port), NI_NUMERICSERV) == 0) {
        //     char s[INET6_ADDRSTRLEN];
        //     inet_ntop(curr->ai_family, get_in_addr(curr->ai_addr), s, sizeof(s));
        //     printf("host: %s\nport: %s\nip: %s\n", host, port, s);
        // }

        fd = ::socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);

        if(fd < 0) {
            close(fd);
            fd = -1;
            continue;
        }

        success = ::connect(fd, curr->ai_addr, curr->ai_addrlen);
        if(success >= 0) {
            break;
        }
        close(fd);
        fd = -1;
    }

    freeaddrinfo(results);

    if(fd < 0) {
        perror("coc");
        throw std::runtime_error("Couldn't connect");
    }

    return std::make_unique<net::socket>(fd);
}

std::unique_ptr<net::socket> net::connect(const char * addr, int port) {
    char buf[6];
    snprintf(buf, 6, "%d", port);
    return net::connect(addr, buf);
}

std::unique_ptr<net::socket> net::connect(const char * path) {
    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path));
    

    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd < 0) {
        throw std::runtime_error("Couldn't create a socket");
    }
    
    int success = ::connect(fd, (sockaddr *) &addr, sizeof(addr));
    if(success < 0) {
        throw std::runtime_error("Couldn't connect");
    }

    return std::make_unique<net::socket>(fd);
}

// void recv_all(int fd, std::string& out) {
//     out = "";

//     pollfd _pollfd;
//     _pollfd.fd = fd;
//     _pollfd.events = POLLIN;

//     do {
//         char buf[2048];
//         size_t n = recv(fd, buf, 2047, 0);
//         buf[n] = '\0';

//         out += buf;

//         if(poll(&_pollfd, 1, 0) == -1) throw std::runtime_error("Poll failed");
//     } while(chk_bit(_pollfd.revents, POLLIN));
// }

size_t clean_up_dead_sockets(std::vector<std::shared_ptr<net::socket>>& sockets, std::vector<pollfd>& pollfds) {
    size_t i = 0;
    for(size_t j = 0; j < sockets.size(); j ++) {
        if(sockets[j]->fd != -1) {
            sockets[i] = sockets[j];
            pollfds[i] = pollfds[j];

            i++;
        }
    }
    sockets.resize(i);

    return i;
}