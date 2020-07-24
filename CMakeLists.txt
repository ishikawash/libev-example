cmake_minimum_required(VERSION 3.16.0)
project(libev-example VERSION 0.1.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)

add_executable(echo main.cpp)
target_include_directories(echo PUBLIC /usr/local/include)
target_link_directories(echo PUBLIC /usr/local/lib)
target_link_libraries(echo ev fmt)
