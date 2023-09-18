#ifndef MISSION_CONTROL_H
#define MISSION_CONTROL_H

#include <string>
#include <functional>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>

// template<typename T>
// struct readable {
//     T _value;

//     operator T&() { return _value; }
//     operator T() const { return _value; }
// };

struct mission_control {
    /** 
     * This struct will manage the sending and recieving of data.
     */

    enum data_type {
        INT, DOUBLE, STRING, BOOL, NONE,
    };


    struct readable {
        std::string name;
        data_type type;
        union {
            std::string * s;
            int * i;
            double * d;
            bool * b;
        } value;
    };

    union update_handler {
        std::function<double(double, double)> d;
        std::function<int(int, int)> i;
        std::function<std::string(std::string, std::string)> s;
        std::function<bool(bool, bool)> b;

        update_handler() {}
        update_handler(const update_handler& handler) {

        }
        ~update_handler() {};
    };

    struct writable {
        std::string name;
        data_type type;
        update_handler update = update_handler();
    };

    std::vector<readable> bound_readables;
    std::string update_changes;
    std::unordered_map<std::string, writable> bound_writables;

    int socket_fd = -1;

    mission_control(const char * path); 

    void add_writable(std::string name, double& value, std::function<double(double, double)> update=[](double d, double old) -> double { return d; });
    void bind_readable(std::string name, double& value);

    void change(std::string name, double value);

    void tick();

    void _write(std::string s);
    std::string build_msg();
};



void mission_control::add_writable(std::string name, double& value, std::function<double(double, double)> update) {
    bound_writables.emplace(name);
    writable& w = bound_writables[name];
    w.name = name;
    w.type = DOUBLE;
    w.update.d = update;
}

void mission_control::bind_readable(std::string name, double& value) {
    readable r;
    r.name = name;
    r.type = DOUBLE;
    r.value.d = &value;
    bound_readables.push_back(r);
}


void mission_control::change(std::string name, double value) {
    update_changes += name;
    update_changes += '=';
    update_changes += std::to_string(value);
    update_changes += ';';
}

mission_control::mission_control(const char * path) {
    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path));

    socket_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if(socket_fd < 0) {
        throw std::runtime_error("Couldn't create a socket");
    }
    
    int success = ::connect(socket_fd, (sockaddr *) &addr, sizeof(addr));
    if(success < 0) {
        throw std::runtime_error("Couldn't connect");
    }
}

std::string mission_control::build_msg() {
    std::string out = "";
    for(auto& i : bound_readables) {
        out += i.name;
        out += "=";
        switch(i.type) {
        case INT: 
            out += std::to_string(*(i.value.i));
            break;
        case DOUBLE:
            out += std::to_string(*(i.value.d));
            break;
        case STRING:
            out += '"';
            out += *i.value.s;
            out += '"';
            break;
        case BOOL:
            out += (*(i.value.b)? "true":"false");
            break;
        default:
            continue;
        }
        out += ";";
    }
    // out += "\n";
    out += update_changes;

    return out;
}

void mission_control::tick() { 
    std::string msg = build_msg();

    // Reset the update_changes;
    update_changes = "";

    _write(msg);
}

void mission_control::_write(std::string s) {
    size_t n = ::write(socket_fd, &s[0], s.length());
    if(n != s.length()) {
        perror("fadsfdsF");
        throw std::runtime_error("bruh");
    }
}

#endif