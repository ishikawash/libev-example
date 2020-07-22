#include <array>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cerrno>
#include <fmt/core.h>
#include <ev++.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using ev_io_ref = std::unique_ptr<ev::io>;

std::unordered_map<int, ev_io_ref> watchers;
std::vector<int> fds_to_close;

void async_close(ev::io &watcher)
{
    watcher.stop();
    fds_to_close.push_back(watcher.fd);
}

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
    const int sock_flags = 0;
    std::array<char, 16> buf;
    buf.fill('\0');

    // Receive data from the socket.
    ssize_t read_size = recv(watcher.fd, buf.data(), buf.size() - 1, sock_flags);
    if (read_size <= 0)
    {
        async_close(watcher);
        log_error(fmt::format("Connection will be closed ({})", watcher.fd));
        return;
    }
    fmt::print("Read from client ({}): {}\n", watcher.fd, buf.data());

    // Send data to the socket.
    if (send(watcher.fd, buf.data(), read_size, sock_flags) < 0)
    {
        async_close(watcher);
        log_error("Failed to write client");
    }
}

void on_accept(ev::io &watcher, int revents)
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

    // Accept connection from remote.
    struct sockaddr_in remote_address;
    socklen_t remote_address_size = sizeof(remote_address);
    int remote_fd = accept(watcher.fd, reinterpret_cast<struct sockaddr *>(&remote_address), &remote_address_size);
    if (remote_fd < 0)
    {
        log_error("Could not establish new connection");
        return;
    }
    fmt::print("Connected ({}): {}:{}\n",
               remote_fd,
               inet_ntoa(remote_address.sin_addr),
               ntohs(remote_address.sin_port));

    // Start observation of the remote socket.
    // MEMO: Create a watcher per one socket.
    auto receive_watcher = std::make_unique<ev::io>(watcher.loop);
    receive_watcher->set(remote_fd, ev::READ);
    receive_watcher->set<on_receive>();
    receive_watcher->start();
    watchers.emplace(remote_fd, std::move(receive_watcher));
}

void on_idle(ev::idle &watcher, int revents)
{
    if (watchers.empty())
    {
        return;
    }
    if (fds_to_close.empty())
    {
        return;
    }
    for (int fd : fds_to_close)
    {
        auto it = watchers.find(fd);
        if (it != watchers.end())
        {
            it->second->stop();
            watchers.erase(it);
        }
        close(fd);
        fmt::print("Connection closed ({})\n", fd);
    }
    fds_to_close.clear();
}

int main(void)
{
    // Create socket for server.
    int sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        log_error("Could not create socket");
        return 1;
    }

    // Create network address.
    const int port = 10080;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket with TCP port.
    if (bind(sock_fd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0)
    {
        log_error("Could not bind socket");
        return 1;
    }

    // MaKe the socket ready to listen.
    const int backlog = 16;
    if (listen(sock_fd, backlog) < 0)
    {
        log_error("Could not listen on socket");
        return 1;
    }
    fmt::print("Server is listening on {}\n", port);

    watchers.reserve(8);
    fds_to_close.reserve(8);

    // Start observation of the socket.
    ev::default_loop loop;
    ev::io accept_watcher(loop);
    accept_watcher.set(sock_fd, ev::READ);
    accept_watcher.set<on_accept>();
    accept_watcher.start();
    ev::idle idle_watcher(loop);
    idle_watcher.set<on_idle>();
    idle_watcher.start();
    loop.run();

    return 0;
}