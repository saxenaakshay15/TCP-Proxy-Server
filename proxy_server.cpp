#include <iostream>
#include <cstring>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 8192

void handle_client(int client_sock, const char* remote_host, int remote_port) {
    int remote_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (remote_sock < 0) {
        std::cerr << "Error creating remote socket." << std::endl;
        close(client_sock);
        return;
    }

    struct sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(remote_port);
    inet_pton(AF_INET, remote_host, &remote_addr.sin_addr);

    if (connect(remote_sock, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
        std::cerr << "Error connecting to remote server." << std::endl;
        close(client_sock);
        close(remote_sock);
        return;
    }

    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    int max_fd = std::max(client_sock, remote_sock) + 1;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(client_sock, &read_fds);
        FD_SET(remote_sock, &read_fds);

        int activity = select(max_fd, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            std::cerr << "Select error." << std::endl;
            break;
        }

        if (FD_ISSET(client_sock, &read_fds)) {
            int bytes_read = read(client_sock, buffer, BUFFER_SIZE);
            if (bytes_read <= 0) {
                break;
            }
            write(remote_sock, buffer, bytes_read);
        }

        if (FD_ISSET(remote_sock, &read_fds)) {
            int bytes_read = read(remote_sock, buffer, BUFFER_SIZE);
            if (bytes_read <= 0) {
                break;
            }
            write(client_sock, buffer, bytes_read);
        }
    }

    close(client_sock);
    close(remote_sock);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: proxy_server <local host ip> <local port> <remote host ip> <remote port>" << std::endl;
        return 1;
    }

    const char* local_host = argv[1];
    int local_port = std::stoi(argv[2]);
    const char* remote_host = argv[3];
    int remote_port = std::stoi(argv[4]);

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        std::cerr << "Error creating listening socket." << std::endl;
        return 1;
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    inet_pton(AF_INET, local_host, &local_addr.sin_addr);

    if (bind(listen_sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        std::cerr << "Error binding to local address." << std::endl;
        close(listen_sock);
        return 1;
    }

    if (listen(listen_sock, 10) < 0) {
        std::cerr << "Error listening on socket." << std::endl;
        close(listen_sock);
        return 1;
    }

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            std::cerr << "Error accepting client connection." << std::endl;
            continue;
        }

        std::thread(handle_client, client_sock, remote_host, remote_port).detach();
    }

    close(listen_sock);
    return 0;
}
