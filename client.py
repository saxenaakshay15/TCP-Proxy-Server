import socket

def start_client(host, port):
    client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_sock.connect((host, port))

    try:
        while True:
            message = input("Enter message to send: ")
            if message.lower() == 'exit':
                break
            client_sock.sendall(message.encode())
            data = client_sock.recv(1024)
            print(f"Received: {data.decode()}")
    finally:
        client_sock.close()

if __name__ == "__main__":
    start_client('127.0.0.1', 8080)
