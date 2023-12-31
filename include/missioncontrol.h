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
 * @brief A readable wrapper. Shorthand to name variables easily. Use *readable in order to retrieve a reference to the underlying data.
 * 
 * @tparam T 
 */
template<typename T>
struct readable{
    T data;
    std::string name;
    /**
     * @brief Construct a new readable object with a name
     * 
     * @param _name 
     */
    readable(std::string _name) {
        name = _name;
    }
    template<typename... Ts> readable(std::string _name, Ts... args) {
        name = _name;
        data = T(args...);
    }
    /**
     * @brief Copy an existing object
     * 
     * @param _name 
     * @param copied 
     */
    readable(std::string _name, const T& copied) {
        name = _name;
        data = copied;
    }
    /**
     * @brief Copy and destroy an existing object
     * 
     * @param _name 
     * @param destroyed 
     */
    readable(std::string _name, T&& destroyed) {
        name = _name;
        data = destroyed;
    }

    /**
     * @brief Serialize readable
     * 
     * @return std::string 
     */
    std::string serialize() const {
        return '\"' + name + "\":" + serialize::serialize(data);
    }

    /**
     * @brief Implicit conversions
     * 
     * @return T& 
     */
    operator T&() { return data; }
    operator T() const { return data; }

    /**
     * @brief Be able to set the readable
     * 
     * @param copied 
     */
    void operator = (const T& copied) {
        data = copied;
    }

    /**
     * @brief Implement *readable
     * 
     * @return T& 
     */
    T& operator* () {
        return data;
    }
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
     * @brief Bind a readable. Ensure the variable has enough lifetime. Ensure that the type has a serialize() implemented. 
     * 
     * @param t 
     */
    void bind_readable(const readable<T>& t);

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
void mission_control::bind_readable(const readable<T>& t) {
    bound_readables_advertisement.push_back(std::make_pair(t.name, ""));
    bound_readables.push_back([&]() -> std::string {
        return t.serialize();
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

void mission_control::connect(const char * path) {
    server = net::create_server(path);
    server->start_listening();
}

void mission_control::connect(unsigned short port) {
    server = net::create_server(port);
    server->start_listening();
}

mission_control::mission_control() {

}

mission_control::mission_control(const char * path) {
    connect(path);
}

void mission_control::advertise() {
    printf("Advertising...\n");
    std::string msg = "{\"type\": \"advertise\"";

    {
        msg += ",\"readables\":{";
        bool first = true;
        for(size_t i = 0; i < bound_readables_advertisement.size(); i ++) {
            if(first) first = false;
            else msg += ",";
            msg += "\"" + bound_readables_advertisement[i].first + "\":\"" + bound_readables_advertisement[i].second + "\"";
        }
        msg += "}";
    }

    {

        msg += ",\"commands\":[";
        bool first = true;
        for(auto i = commands.begin(); i != commands.end(); i ++) {
            if(first) first = false;
            else msg += ",";
            msg += "\"" + (*i).first + "\"";
        }
        msg += "]";
    }

    msg += "}\x1f";

    _write(msg);

    printf("Finished advertising\n");
}

/**
 *      Mission Control Response format::
 *          {
 *              "type": "update"|"advertise", // "update" on "tick()" calls. "advertise" on "advertise()" calls.
 *              "readables": { // Only sent during "advertise()" calls
 *              
 *              },
 *              "commands": [ // Only sent during "advertise()" calls
 *              
 *              ],
 *              "data": { // Only added during "tick()" calls
 *                  // Bound readables every tick
 *                  // Set readables when they are set
 *              },
 * 
 *              "out": [ // Only added when there is new console output in "tick()" calls
 *                  { "msg": "message", "type": "error" | "info" }
 *              ]
 *          }
*/
std::string mission_control::build_msg() {
    std::string out = "{\"type\": \"update\",";
    {
        out += "\"data\":{";
        bool first = true;
        for(size_t i = 0; i < bound_readables.size(); i ++) {
            if(first) {
                first = false;
            }else {
                out += ',';
            }
            out += bound_readables[i]();
        }
        for(size_t i = 0; i < set_readables.size(); i ++) {
            if(first) {
                first = false;
            }else {
                out += ',';
            }
            out += set_readables[i];
        }
        out += "}";
    }
    if(!output_log.empty()) {
        out += ",\"out\":[";
        bool first = true;
        
        for(size_t i = 0; i < output_log.size(); i ++) {
            if(first) {
                first = false;
            }else {
                out += ',';
            }
            out += "{\"msg\":\"" + output_log[i].msg + "\",\"type\":\"" + output_log[i].type + "\",\"time\":"+std::to_string(output_log[i].time)+"}";
        }
        out += "]";
    }
    out += "}\x1f";
    return out;
}

std::string::iterator mission_control::_parse_next_command(std::string::iterator i, std::string::const_iterator end, command_call& call) {
    call.command = "";
    call.args.clear();

    bool quotes = false;
    auto j = i;

    while(!((*j == ';' || *j == ' ') && !quotes)) {
        if(j == end) return j;
        if(*i == '"') quotes = !quotes;
        call.command += *j;
        j ++;
    }

    while(!(*j == ';' && !quotes)) {
        if(j == end) return j;
        if(*i == '"') quotes = !quotes;
        j++;
        std::string arg = "";
        while(!((*j == ' ' || *j == ';') && !quotes)) {
            if(j == end) return j;
            if(*i == '"') quotes = !quotes;
            arg += *j;
            j ++;
        }
        if(arg.length() != 0) call.args.push_back(arg);
    }

    return j;
}

void mission_control::run_command(const command_call& call) {
    ::printf("\"%s\" command recieved\n", call.command.c_str());
    if(call.command == "set") { // "set" command is built-in
        if(call.args.size() != 2) return;

        std::string name = call.args[0];
        std::string value = call.args[1];

        if(bound_writables.count(name) > 0) bound_writables[name](value);
        else log_error("\\\""+name+"\\\" is not a writable parameter.");
    }else if(call.command == "advertise"){
        advertise();
    }else if(commands.count(call.command) > 0) {
        printf("Command found\n");
        try {
            commands[call.command](call.args);
        }catch(std::exception e) {
            // Should only log errors. Fatal errors here could brick.
        };
    }else {
        // No command.
    }
}


void mission_control::_handle_commands() {
    // ::printf("Getting messages\n");
    std::vector<std::string> messages = server->process_incoming();
    // ::printf("Messages:\n");
    // int i = 1;
    // for(auto& message : messages) {
    //     ::printf("%3d : %s\n", i++, message.c_str());
    // }
    for(auto& message : messages) {
        std::string incoming_msg = message;
        ::printf("cmd: %s\n", incoming_msg.c_str());

        auto i = incoming_msg.begin();
        auto end = incoming_msg.end();
        command_call call;
        bool parsing = true;
        while(parsing) {
            auto j = _parse_next_command(i, end, call);
            // Advance iterator and run command, unless there is no more message, then add the rest of the message to remaining message.
            if(j == end) {
                // remaining_message = std::string(i, end);
                // ::printf("rem: \"%s\"\n", remaining_message.c_str());
                parsing = false;
            }else {
                
                // std::cout << call.command << std::endl;
                run_command(call);

                i = j+1;
            }
        }
    }
}

void mission_control::log(std::string message) {
    // printf("Logging %s\n", message.c_str());
    output_log.emplace_back(message, "info");
}
void mission_control::log_error(std::string message) {    
    output_log.emplace_back(message, "error");
}

void mission_control::tick() { 
    
    // Check incoming commands.
    _handle_commands();

    // Reset the update_changes;
    set_readables.clear();
    output_log.clear();

    
    // Construct output string.
    _write(build_msg());
}

void mission_control::_write(std::string s) {
    server->broadcast(s);
}

std::string serialize::serialize(const double& d) {
    return std::to_string(d);
}

std::string serialize::serialize(const int& d) {
    return std::to_string(d);
}

std::string serialize::serialize(const std::string& d) {
    return "\"" + d + "\"";
}

template<>
double serialize::deserialize<double>(const std::string& s) {
    return std::stod(s);
}

template<>
int serialize::deserialize<int>(const std::string& s) {
    return std::stoi(s);
}

template<>
std::string serialize::deserialize<std::string>(const std::string& s) {
    size_t i = 0;
    size_t length = s.length();
    if(length < 3) return "";
    if(s[i] != '"') return "";
    i++;
    size_t j = i;
    while(s[i] != '"' && i < length) {
        i++;
    }

    return s.substr(j, i - j);
}


template<>
std::vector<double> serialize::deserialize<std::vector<double>>(const std::string& s) {
    std::vector<double> out;
    
    size_t i = 0;
    size_t length = s.length();

    if(s[i] != '[') return out;

    while(s[i] != ']' && i < length) {
        i++;
        size_t j = i;
        while(s[i] != ']' && s[i] != ',' && i < length) { i++; }
        out.push_back(std::stod(s.substr(j, i - j)));
    }

    return out;
}

template<>
std::vector<int> serialize::deserialize<std::vector<int>>(const std::string& s) {
    std::vector<int> out;
    
    size_t i = 0;
    size_t length = s.length();

    if(s[i] != '[') return out;

    while(s[i] != ']' && i < length) {
        i++;
        size_t j = i;
        while(s[i] != ']' && s[i] != ',' && i < length) { i++; }
        out.push_back(std::stoi(s.substr(j, i - j)));
    }

    return out;
}

template<>
std::vector<std::string> serialize::deserialize<std::vector<std::string>>(const std::string& s) {
    std::vector<std::string> out;
    
    size_t i = 0;
    size_t length = s.length();

    if(s[i] != '[') return out;

    while(s[i] != ']' && i < length) {
        i++;
        size_t j = i;
        while(s[i] != ']' && s[i] != ',' && i < length) { i++; }
        out.push_back(serialize::deserialize<std::string>(s.substr(j, i - j)));
    }

    return out;
}

mission_control::log_message::log_message(std::string _msg, std::string _type) {
    msg = _msg;
    type = _type;
    time = std::time(nullptr);
}

void mission_control::add_command(std::string name, mission_control::command _command) {
    commands[name] = _command;
}

#endif