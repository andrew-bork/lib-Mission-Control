cmake_minimum_required(VERSION 3.27.0)
set(CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)


project(libmissioncontrol)
include_directories(include)

# add_library(missioncontrol STATIC src/missioncontrol.cpp)


# add_executable(test src/test.cpp src/missioncontrol.cpp)
# target_link_libraries(test missioncontrol)

# Turn on all warning.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")