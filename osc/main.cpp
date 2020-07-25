#include <string>
#include <array>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fmt/core.h>
#include <ev++.h>
#include "tinyosc.h"

void log_error(const std::string &message)
{
    if (errno == 0)
    {
        fmt::print(stderr, "{}\n", message);
    }
    else
    {
        fmt::print(stderr, "{}: {}\n", message, strerror(errno));
    }
}

void on_receive(ev::io &watcher, int revents)
{
    // Guard events to ignore.
    if ((revents & EV_ERROR) > 0)
    {
        return;
    }
    if ((revents & EV_READ) == 0)
    {
        return;
    }

    std::array<char, 256> buf;
    buf.fill('\0');

    // Receive data from the socket.
    const int sock_flags = 0;
    ssize_t read_size = recv(watcher.fd, buf.data(), buf.size(), sock_flags);
    if (read_size <= 0)
    {
        log_error("Reading data from client was failed.");
        return;
    }
    fmt::print("Read from client ({}): {}\n", watcher.fd, read_size);

    if (tosc_isBundle(buf.data()))
    {
        tosc_bundle bundle;
        tosc_parseBundle(&bundle, buf.data(), read_size);
        const uint64_t timetag = tosc_getTimetag(&bundle);
        tosc_message osc;
        while (tosc_getNextMessage(&bundle, &osc))
        {
            tosc_printMessage(&osc);
        }
    }
    else
    {
        tosc_message osc;
        tosc_parseMessage(&osc, buf.data(), read_size);
        tosc_printMessage(&osc);
    }
}

int main()
{
    // Create socket for server.
    int sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0)
    {
        log_error("Could not create socket");
        return 1;
    }

    // Create network address.
    const int port = 9000;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket with UDP port.
    if (bind(sock_fd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0)
    {
        log_error("Could not bind socket");
        return 1;
    }

    // Start observation of the socket.
    ev::default_loop loop;
    ev::io receive_watcher(loop);
    receive_watcher.set(sock_fd, ev::READ);
    receive_watcher.set<on_receive>();
    receive_watcher.start();
    loop.run();

    return 0;
}