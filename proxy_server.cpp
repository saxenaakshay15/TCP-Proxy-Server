#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <functional>

class ThreadPool {
public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

    void worker_thread();
};

ThreadPool::ThreadPool(size_t num_threads) : stop(false) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back([this] { worker_thread(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (auto& worker : workers) {
        worker.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

void ThreadPool::worker_thread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });
            if (stop && tasks.empty()) {
                return;
            }
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}

void set_non_blocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

void handle_client(int client_sock, const char* remote_ip, int remote_port) {
    int remote_sock;
    struct sockaddr_in remote_addr;

    // Create socket for connecting to the remote server
    if ((remote_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        close(client_sock);
        return;
    }

    set_non_blocking(remote_sock);

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(remote_port);
    if (inet_pton(AF_INET, remote_ip, &remote_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(client_sock);
        close(remote_sock);
        return;
    }

    // Connect to the remote server
    if (connect(remote_sock, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
        if (errno != EINPROGRESS) {
            perror("connect");
            close(client_sock);
            close(remote_sock);
            return;
        }
    }

    char buffer[8192];
    ssize_t bytes_read;
    
    // Read from client and write to remote server
    while ((bytes_read = read(client_sock, buffer, sizeof(buffer))) > 0) {
        ssize_t bytes_written = write(remote_sock, buffer, bytes_read);
        if (bytes_written < 0) {
            perror("write");
            break;
        }
    }

    if (bytes_read < 0) {
        perror("read");
    }

    // Read from remote server and write to client
    while ((bytes_read = read(remote_sock, buffer, sizeof(buffer))) > 0) {
        ssize_t bytes_written = write(client_sock, buffer, bytes_read);
        if (bytes_written < 0) {
            perror("write");
            break;
        }
    }

    if (bytes_read < 0) {
        perror("read");
    }

    close(client_sock);
    close(remote_sock);
}

void start_proxy(const char* local_ip, int local_port, const char* remote_ip, int remote_port) {
    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        perror("epoll_create1");
        return;
    }

    // Create socket for accepting client connections
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return;
    }

    set_non_blocking(server_sock);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(local_ip);
    server_addr.sin_port = htons(local_port);

    // Bind the socket to the local address and port
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        return;
    }

    // Listen for incoming connections
    if (listen(server_sock, 10) < 0) {
        perror("listen");
        close(server_sock);
        return;
    }

    std::cout << "Proxy server listening on " << local_ip << ":" << local_port << std::endl;

    struct epoll_event ev, events[10];
    ev.events = EPOLLIN;
    ev.data.fd = server_sock;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sock, &ev) == -1) {
        perror("epoll_ctl: server_sock");
        close(server_sock);
        return;
    }

    ThreadPool thread_pool(4);

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, 10, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            close(server_sock);
            return;
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_sock) {
                int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
                if (client_sock == -1) {
                    perror("accept");
                    continue;
                }

                set_non_blocking(client_sock);

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_sock;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &ev) == -1) {
                    perror("epoll_ctl: client_sock");
                    close(client_sock);
                    continue;
                }
            } else {
                int client_sock = events[n].data.fd;
                thread_pool.enqueue([client_sock, remote_ip, remote_port] {
                    handle_client(client_sock, remote_ip, remote_port);
                });
            }
        }
    }

    close(server_sock);
    close(epoll_fd);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <local_ip> <local_port> <remote_ip> <remote_port>" << std::endl;
        return 1;
    }

    const char* local_ip = argv[1];
    int local_port = std::stoi(argv[2]);
    const char* remote_ip = argv[3];
    int remote_port = std::stoi(argv[4]);

    start_proxy(local_ip, local_port, remote_ip, remote_port);

    return 0;
}
