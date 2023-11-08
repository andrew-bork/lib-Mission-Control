#ifndef MISSION_CONTROL_H
#define MISSION_CONTROL_H
/**
 * @file missioncontrol.h
 * @brief This is a header only file for libmissioncontrol
 * @version 0.1
 * @date 2023-09-30
 * 
 * 
 */
#include <string>
#include <unordered_map>
#include <poll.h>
#include <vector>
#include <functional>
#include <net.hpp>

/**
 * @brief The serialize namespace contains all the serialize() methods in order to serialize readable objects.
 * In order to serialize custom objects, make sure to implement your own serialize() method before including missioncontrol.h
 * 
 */
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

/**
 * @brief Underlying structure that communicates with mission control.
 * 
 */
struct mission_control {
    /**
     * @brief Mission control executable command
     * 
     */
    typedef std::function<void(std::vector<std::string>)> command;

    struct log_message {
        std::string msg;
        std::string type;
        time_t time;

        log_message(std::string _msg, std::string _type);
    };
    // std::vector<readable> bound_readables;
    /**
     * @brief Construct a new mission control object without a connection
     * 
     */
    mission_control(); 
    /**
     * @brief Construct a new mission control object with a unix connection
     * 
     * @param path 
     */
    mission_control(const char * path); 
    // mission_control(unsigned int port); 

    template<typename T>
    /**
     * @brief Add a writable parameter. update(newvalue) will get run when set on this writable is called.
     * Return the new value of the writable.
     * 
     * @param name 
     * @param value 
     * @param update 
     */
    void add_writable(std::string name, T& value, std::function<T(T&)> update);

    template<typename T>
    /**
     * @brief Bind a readable variable. Ensure the variable has enough lifetime. Ensure that the type has a serialize() implemented.
     * 
     * @param name 
     * @param t 
     */
    void bind_readable(std::string name, const T& t);

    template<typename T>
    /**
     * @brief A one time set. Variable lifetime does not matter.
     * 
     * @param name 
     * @param value 
     */
    void set(std::string name, T value);

    /**
     * @brief Add a command 
     * 
     * @param name 
     * @param _command 
     */
    void add_command(std::string name, command _command);

    // template<typename... T> void printf(T&...);
    // template<typename... T> void printf_error(T&...);

    /**
     * @brief Log "info"
     * 
     * @param s 
     */
    void log(std::string s);
    /**
     * @brief Log "error"
     * 
     * @param s 
     */
    void log_error(std::string s);

    /**
     * @brief Advertise all readables and commands.
     * 
     */
    void advertise();
    /**
     * @brief Process incoming commands, then update mission control on the bound variables
     * 
     */
    void tick();

    /**
     * @brief Connect (unix socket)
     * 
     * @param path 
     */
    void connect(const char * path);
    /**
     * @brief Connect (tcp socket)
     * 
     * @param port 
     */
    void connect(unsigned short port);


    /**
     * @brief Contains the command name and the arguments that will be sent to the command.
     * 
     */
    struct command_call {
        std::string command;
        std::vector<std::string> args;
    };

    /**
     * @brief Runs the command based on the command call struct
     * 
     * @param command 
     */
    void run_command(const command_call& command);


private:
    std::vector<std::pair<std::string, std::string>> bound_readables_advertisement;
    std::vector<std::pair<std::string, std::string>> bound_writables_advertisement;

    std::vector<std::function<std::string(void)>> bound_readables;
    std::vector<std::string> set_readables;
    std::unordered_map<std::string, std::function<void(std::string)>> bound_writables;
    std::unordered_map<std::string, command> commands;

    std::vector<log_message> output_log;

    std::unique_ptr<net::server> server;

    void _handle_commands();
    std::string::iterator _parse_next_command(std::string::iterator i, std::string::const_iterator end, command_call& call);
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
    bound_readables_advertisement.push_back(std::make_pair(name, ""));
    bound_readables.push_back([&]() -> std::string {
        return name + ':' + serialize::serialize(t);
    });
}


template<typename T>
void mission_control::add_writable(std::string name, T& value, std::function<T(T&)> update) {
    bound_writables_advertisement.push_back(std::make_pair(name, ""));
    bound_writables[name] = [&](std::string s) {
        T deserialized = serialize::deserialize<T>(s);
        value = update(deserialized);
    };
}


template<typename T>
void mission_control::set(std::string name, T value) {
    set_readables.push_back(name + ":" + serialize::serialize(value));
}


#endif