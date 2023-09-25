#include <missioncontrol.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

void mission_control::add_writable(std::string name, double& value, std::function<void(void)> update) {
    writable w;
    w.name = name;
    w.value.d = &value;
    w.type = DOUBLE;
    w.update = update;
    bound_writables[name] = w;
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

void mission_control::_connect_unix(const char * path) {
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
std::string::iterator mission_control::_parse_next_command(std::string::iterator i, std::string::const_iterator end, command_call& call) {
    call.command = "";
    call.args.clear();

    bool quotes = false;
    auto j = i;
    while(*i == ';' && !quotes) {
        if(*i == '"') quotes = !quotes;
        i++;
        if(i == end) {
            return i;
        }
    }

    while((*j == ';' || *j == ' ') && !quotes) {
        if(*i == '"') quotes = !quotes;
        call.command += *j;
        j ++;
    }

    while(*j == ';' && !quotes) {
        if(*i == '"') quotes = !quotes;
        j++;
        std::string arg = "";
        while((*j == ' ' || *j == ';') && !quotes) {
            if(*i == '"') quotes = !quotes;
            arg += *j;
            j ++;
        }
        if(arg.length() != 0) call.args.push_back(arg);
    }

    return i;
}

void mission_control::run_command(const command_call& call) {
    if(call.command == "set") { // "set" command is built-in
        std::string out = call.args[0];
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
            printf("Incoming command: %s\n", buf);
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