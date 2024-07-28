#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class Bridge {
public:
    Bridge(int client_socket, const std::string& upstream_host, unsigned short upstream_port)
        : client_socket_(client_socket), upstream_host_(upstream_host), upstream_port_(upstream_port) {}

    void start() {
        upstream_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (upstream_socket_ < 0) {
            std::cerr << "Failed to create upstream socket." << std::endl;
            close(client_socket_);
            return;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(upstream_port_);
        inet_pton(AF_INET, upstream_host_.c_str(), &server_addr.sin_addr);

        if (connect(upstream_socket_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Failed to connect to upstream server." << std::endl;
            close(client_socket_);
            close(upstream_socket_);
            return;
        }

        std::thread([this] { this->handleClientToServer(); }).detach();
        std::thread([this] { this->handleServerToClient(); }).detach();
    }

private:
    void handleClientToServer() {
        char buffer[8192];
        ssize_t bytes_read;
        while ((bytes_read = read(client_socket_, buffer, sizeof(buffer))) > 0) {
            write(upstream_socket_, buffer, bytes_read);
        }
        close(upstream_socket_);
    }

    void handleServerToClient() {
        char buffer[8192];
        ssize_t bytes_read;
        while ((bytes_read = read(upstream_socket_, buffer, sizeof(buffer))) > 0) {
            write(client_socket_, buffer, bytes_read);
        }
        close(client_socket_);
    }

    int client_socket_;
    int upstream_socket_;
    std::string upstream_host_;
    unsigned short upstream_port_;
};

class Acceptor {
public:
    Acceptor(const std::string& local_host, unsigned short local_port, const std::string& upstream_host, unsigned short upstream_port)
        : local_host_(local_host), local_port_(local_port), upstream_host_(upstream_host), upstream_port_(upstream_port) {}

    bool start() {
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            std::cerr << "Failed to create server socket." << std::endl;
            return false;
        }

        sockaddr_in local_addr;
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(local_port_);
        inet_pton(AF_INET, local_host_.c_str(), &local_addr.sin_addr);

        if (bind(server_socket, (sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
            std::cerr << "Failed to bind to local address." << std::endl;
            close(server_socket);
            return false;
        }

        if (listen(server_socket, 10) < 0) {
            std::cerr << "Failed to listen on socket." << std::endl;
            close(server_socket);
            return false;
        }

        while (true) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
            if (client_socket >= 0) {
                Bridge* bridge = new Bridge(client_socket, upstream_host_, upstream_port_);
                bridge->start();
            }
        }

        close(server_socket);
        return true;
    }

private:
    std::string local_host_;
    unsigned short local_port_;
    std::string upstream_host_;
    unsigned short upstream_port_;
};

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: tcpproxy_server <local host ip> <local port> <forward host ip> <forward port>" << std::endl;
        return 1;
    }

    const unsigned short local_port = static_cast<unsigned short>(::atoi(argv[2]));
    const unsigned short forward_port = static_cast<unsigned short>(::atoi(argv[4]));
    const std::string local_host = argv[1];
    const std::string forward_host = argv[3];

    Acceptor acceptor(local_host, local_port, forward_host, forward_port);
    if (!acceptor.start()) {
        std::cerr << "Failed to start TCP proxy server." << std::endl;
        return 1;
    }

    return 0;
}
