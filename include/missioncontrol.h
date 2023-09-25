#ifndef MISSION_CONTROL_H
#define MISSION_CONTROL_H

#include <string>
#include <unordered_map>
#include <poll.h>
#include <vector>
#include <functional>

struct mission_control {

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


    struct writable {
        std::string name;
        data_type type;
        union {
            std::string * s;
            int * i;
            double * d;
            bool * b;
        } value;
        std::function<void(void)> update;
    };

    std::vector<readable> bound_readables;
    std::string update_changes;
    std::unordered_map<std::string, writable> bound_writables;
    std::unordered_map<std::string, std::function<void(std::vector<std::string>)>> commands;

    pollfd poll_struct;
    int socket_fd = -1;

    mission_control(const char * path); 

    void add_writable(std::string name, double& value, std::function<void(void)> update=[]() {});
    void bind_readable(std::string name, double& value);

    void change(std::string name, double value);

    void tick();


    std::string remaining_message = "";
    void _connect_unix(const char * path);
    void _connect_inet(const char* host, unsigned short port);
    void _init_pollfd();

    void _handle_commands();

    struct command_call {
        std::string command;
        std::vector<std::string> args;
    };

    std::string::iterator _parse_next_command(std::string::iterator i, std::string::const_iterator end, command_call& call);

    void run_command(const command_call& command);
    // void _connect_serial();

    void _write(std::string s);
    std::string build_msg();
};

#endif