import socket

def main():
    host = '127.0.0.1'
    port = 8080

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((host, port))

    message = "Hello, Proxy Server!"
    client_socket.sendall(message.encode())

    response = client_socket.recv(1024)
    print(f"Received from server: {response.decode()}")

    client_socket.close()

if __name__ == "__main__":
    main()
