# libmissioncontrol

A C++ library that allows programs to be monitored from an outside REST server. The code for the rest server is [here](https://github.com/andrew-bork/Mission-Control-Restful-Server)
Allows developers to "bind" readable values to be monitored from elsewhere, and allows custom commands to be run remotely.

## Documentation

Read the header file missioncontrol.h for specifics on how the use the library.

## Message format

Sends json to the server in the following format:
```json
{
    "type": "advertise" | "tick" // advertise on advertise() calls, tick on tick() calls,
    "data": { ... } // Present on tick() calls.
    "out": [ ... ] // Logs; Present on tick() calls when not empty.

    "readables": { ... } // Present on advertise() calls.
    "commands": [ ... ] // Present of advertise() calls.
}\x1f // Seperate messages with a seperator character
```

Recieves commmands from the server in the following format:
```
command1 arg1 arg2 arg3;command2 arg1;command3
```