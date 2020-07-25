project(libev-osc VERSION 0.1.0)

add_subdirectory(tinyosc)
add_executable(osc_reciever main.cpp)
target_include_directories(
    osc_reciever
    PUBLIC /usr/local/include
    PUBLIC tinyosc
)
target_link_directories(osc_reciever PUBLIC /usr/local/lib)
target_link_libraries(osc_reciever ev fmt tinyosc)