project(libev-echo VERSION 0.1.0)

add_executable(echo main.cpp)
target_include_directories(echo PUBLIC /usr/local/include)
target_link_directories(echo PUBLIC /usr/local/lib)
target_link_libraries(echo ev fmt)
