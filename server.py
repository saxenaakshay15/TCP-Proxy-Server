import socket

def start_server(host, port):
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.bind((host, port))
    server_sock.listen(5)
    print(f"Server listening on {host}:{port}")

    while True:
        client_sock, addr = server_sock.accept()
        print(f"Connection from {addr}")
        while True:
            data = client_sock.recv(1024)
            if not data:
                break
            print(f"Received: {data.decode()}")
            client_sock.sendall(data)
        client_sock.close()

if __name__ == "__main__":
    start_server('127.0.0.1', 9090)