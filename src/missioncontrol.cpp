#include <missioncontrol.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <ctime>

template<typename T>
void mission_control::set(std::string name, T value) {
    set_readables.push_back(name + ":" + serialize::serialize(value));
}

void mission_control::_connect_unix(const char * path) {
    // return;
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

void mission_control::_connect_inet(const char* host, unsigned short port) {

}

void mission_control::_init_pollfd() {
    poll_struct.fd = socket_fd;
    poll_struct.events = POLLIN;
}

mission_control::mission_control(const char * path) {
    _connect_unix(path);
    _init_pollfd();
}

void mission_control::advertise() {
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

    msg += "}";

    _write(msg);
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
    out += "}";
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
    if(call.command == "set") { // "set" command is built-in
        if(call.args.size() != 2) return;

        std::string name = call.args[0];
        std::string value = call.args[1];

        bound_writables[name](value);
    }else if(commands.count(call.command) > 0) {
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

    int n_polled = poll(&poll_struct, 1, 0);
    if(n_polled < 0) {
        throw std::runtime_error("idk lmao");
    }
    if(n_polled > 0) {
        if(poll_struct.revents & POLLIN) {
            char buf[2048];
            ssize_t n_bytes = read(socket_fd, buf, 2047);
            if(n_bytes < 0) {
                throw std::runtime_error("uh read failed");
            }

            buf[n_bytes] = '\0';
            // printf("Incoming command: %s\n", buf);
            std::string incoming_msg = remaining_message + std::string(buf);

            auto i = incoming_msg.begin();
            auto end = incoming_msg.end();
            command_call call;
            bool parsing = true;
            while(parsing) {
                auto j = _parse_next_command(i, end, call);

                // Advance iterator and run command, unless there is no more message, then add the rest of the message to remaining message.
                if(j == end) {
                    remaining_message = std::string(i, end);
                    parsing = false;
                }else {
                    
                    // std::cout << call.command << std::endl;
                    run_command(call);

                    i = j+1;
                }
            }
        }
    }
}

void mission_control::log(std::string message) {
    output_log.emplace_back(message, "info");
}
void mission_control::log_error(std::string message) {    
    output_log.emplace_back(message, "error");
}

void mission_control::tick() { 
    
    // Construct output string.
    _write(build_msg());

    // Reset the update_changes;
    set_readables.clear();
    output_log.clear();

    // Check incoming commands.
    _handle_commands();
}

void mission_control::_write(std::string s) {
    size_t n = ::write(socket_fd, &s[0], s.length());
    if(n != s.length()) {
        perror("fadsfdsF");
        throw std::runtime_error("bruh");
    }
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