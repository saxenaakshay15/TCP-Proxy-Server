#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class Bridge {
public:
    Bridge(SOCKET clientSocket, SOCKET serverSocket)
        : clientSocket_(clientSocket), serverSocket_(serverSocket) {}

    void Start() {
        std::thread clientToServerThread(&Bridge::ForwardClientToServer, this);
        std::thread serverToClientThread(&Bridge::ForwardServerToClient, this);

        clientToServerThread.join();
        serverToClientThread.join();
    }

private:
    void ForwardClientToServer() {
        char buffer[8192];
        int bytesReceived;

        while ((bytesReceived = recv(clientSocket_, buffer, sizeof(buffer), 0)) > 0) {
            send(serverSocket_, buffer, bytesReceived, 0);
        }

        Close();
    }

    void ForwardServerToClient() {
        char buffer[8192];
        int bytesReceived;

        while ((bytesReceived = recv(serverSocket_, buffer, sizeof(buffer), 0)) > 0) {
            send(clientSocket_, buffer, bytesReceived, 0);
        }

        Close();
    }

    void Close() {
        closesocket(clientSocket_);
        closesocket(serverSocket_);
    }

    SOCKET clientSocket_;
    SOCKET serverSocket_;
};

class Acceptor {
public:
    Acceptor(const std::string& localHost, unsigned short localPort,
             const std::string& remoteHost, unsigned short remotePort)
        : localPort_(localPort), remotePort_(remotePort) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }

        localSocket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (localSocket_ == INVALID_SOCKET) {
            throw std::runtime_error("Socket creation failed");
        }

        sockaddr_in localAddr;
        localAddr.sin_family = AF_INET;
        localAddr.sin_addr.s_addr = inet_addr(localHost.c_str());
        localAddr.sin_port = htons(localPort);

        if (bind(localSocket_, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
            throw std::runtime_error("Bind failed");
        }

        if (listen(localSocket_, SOMAXCONN) == SOCKET_ERROR) {
            throw std::runtime_error("Listen failed");
        }

        remoteHost_ = remoteHost;
    }

    ~Acceptor() {
        closesocket(localSocket_);
        WSACleanup();
    }

    void AcceptConnections() {
        while (true) {
            SOCKET clientSocket = accept(localSocket_, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Accept failed" << std::endl;
                continue;
            }

            SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket == INVALID_SOCKET) {
                std::cerr << "Socket creation failed" << std::endl;
                closesocket(clientSocket);
                continue;
            }

            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = inet_addr(remoteHost_.c_str());
            serverAddr.sin_port = htons(remotePort_);

            if (connect(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                std::cerr << "Connect failed" << std::endl;
                closesocket(clientSocket);
                closesocket(serverSocket);
                continue;
            }

            Bridge bridge(clientSocket, serverSocket);
            bridge.Start();
        }
    }

private:
    SOCKET localSocket_;
    unsigned short localPort_;
    unsigned short remotePort_;
    std::string remoteHost_;
};

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: proxy_server <local_host_ip> <local_port> <forward_host_ip> <forward_port>" << std::endl;
        return 1;
    }

    const std::string localHost = argv[1];
    const unsigned short localPort = static_cast<unsigned short>(std::atoi(argv[2]));
    const std::string remoteHost = argv[3];
    const unsigned short remotePort = static_cast<unsigned short>(std::atoi(argv[4]));

    try {
        Acceptor acceptor(localHost, localPort, remoteHost, remotePort);
        acceptor.AcceptConnections();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
