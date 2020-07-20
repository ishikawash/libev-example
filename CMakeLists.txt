cmake_minimum_required(VERSION 3.0.0)
project(libev-example VERSION 0.1.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)

include_directories(
    /usr/local/include
)
link_directories(
    /usr/local/lib
)

add_executable(echo main.cpp)
target_link_libraries(echo ev fmt)
