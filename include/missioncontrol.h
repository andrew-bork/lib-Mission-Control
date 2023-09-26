#ifndef MISSION_CONTROL_H
#define MISSION_CONTROL_H

#include <string>
#include <unordered_map>
#include <poll.h>
#include <vector>
#include <functional>



namespace serialize {
    std::string serialize(const double& d);

    std::string serialize(const int& d);
    std::string serialize(const std::string& d);

    template <typename T>
    std::string serialize(const std::vector<T>& s);

    template <typename T>
    std::string serialize(const T& t) {
        return t.serialize();
    }
    template<typename T>
    T deserialize(const std::string& s);

    template<>
    double deserialize<double>(const std::string& s);
    template<>
    int deserialize<int>(const std::string& s);

    template<>
    std::string deserialize<std::string>(const std::string& s);

    template<>
    std::vector<double> deserialize<std::vector<double>>(const std::string& s);

    template<>
    std::vector<int> deserialize<std::vector<int>>(const std::string& s);

    template<>
    std::vector<std::string> deserialize<std::vector<std::string>>(const std::string& s);


};

template<typename T>
struct readable{
    T data;
    std::string name;
    readable(std::string _name) {
        name = _name;
    }
    template<typename... Ts> readable(std::string _name, Ts... args) {
        name = _name;
        data = T(args...);
    }
    readable(std::string _name, const T& copied) {
        name = _name;
        data = copied;
    }
    readable(std::string _name, T&& destroyed) {
        name = _name;
        data = destroyed;
    }
    //std::enable_if<data.serialze, std::function<std::string(void)>>
    std::string serialize() const {
        return '\"' + name + "\":" + serialize::serialize(data);
    }

    operator T&() { return data; }
    operator T() const { return data; }

    void operator = (const T& copied) {
        data = copied;
    }

    T& operator* () {
        return data;
    }
};

struct mission_control {

    enum data_type {
        INT, DOUBLE, STRING, BOOL, NONE,
    };

    // std::vector<readable> bound_readables;
    std::vector<std::function<std::string(void)>> bound_readables;
    std::string update_changes;
    std::unordered_map<std::string, std::function<void(std::string)>> bound_writables;
    std::unordered_map<std::string, std::function<void(std::vector<std::string>)>> commands;

    pollfd poll_struct;
    int socket_fd = -1;

    mission_control(const char * path); 

    template<typename T>
    void add_writable(std::string name, T& value, std::function<T(T&)> update);
    // void bind_readable(std::string name, double& value);
    template<typename T>
    void bind_readable(std::string name, const T& t);
    template<typename T>
    void bind_readable(const readable<T>& t);

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

template<typename T>
std::string serialize::serialize(const std::vector<T>& s) {
    if(s.size() == 0) return "[]";

    std::string built = "[";

    auto i = s.begin();
    built += serialize(*i);

    if(s.size() == 1) {
        built += ']';
        return built;
    }

    for(; i != s.end(); i ++) {
        built += ',';
        built += serialize((*i));
    }
    built += ']';
    return built;
}

template<typename T>
void mission_control::bind_readable(std::string name, const T& t) {
    bound_readables.push_back([&]() -> std::string {
        return name + ':' + serialize::serialize(t);
    });
}

template<typename T>
void mission_control::bind_readable(const readable<T>& t) {
    bound_readables.push_back([&]() -> std::string {
        return t.serialize();
    });
}

template<typename T>
void mission_control::add_writable(std::string name, T& value, std::function<T(T&)> update) {
    bound_writables[name] = [&](std::string s) {
        T deserialized = serialize::deserialize<T>(s);
        value = update(deserialized);
    };
}

#endif