cmake_minimum_required(VERSION 3.27.0)
set(CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)


project(libmissioncontrol)

include_directories(include)

# Turn on all warning.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")