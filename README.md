# TCP Proxy Server

## Description

The TCP Proxy Server is a simple utility that proxies (tunnels or redirects) connections from external clients to a specific server. It is designed to:

- Limit the number of client connections to the server.
- Load balance client connections between multiple server instances.
- Provide IP or connection time-based filtering and access control mechanisms.
- Inspect (log), filter, or otherwise modify data flowing between the clients and the server.

## Features

- Connection limiting
- Load balancing
- Filtering and access control
- Data inspection and modification

## Compatibility

The C++ TCP Proxy Server implementation is compatible with the following C++ compilers:

- GNU Compiler Collection (GCC)
- Intel® C++ Compiler
- Clang/LLVM
- PGI C++ Compiler
- Microsoft Visual Studio C++ Compiler
- IBM XL C/C++ Compiler

## Internals of the Proxy

The proxy implementation consists of three main components:

1. **Acceptor**: Sets up the proxy, binds to the given IP and port, and listens for incoming connections.
2. **Bridge**: Handles the actual data transfer between the client and the remote server.
3. **I/O Service**: Manages asynchronous I/O operations.

## Building and Running the Project on Linux

### Prerequisites

- GCC or any other compatible C++ compiler
- Python 3

### Steps

1. **Clone the Repository**:
   ```bash
   git clone <repository-url>
   cd <repository-directory>
   ```

2. **Build the Project**:
   Use the provided Makefile to compile the proxy server.
   ```bash
   make
   ```

3. **Run the Python Server**:
   Open a terminal and navigate to the project directory, then start the Python server.
   ```bash
   python3 server.py
   ```

4. **Run the Proxy Server**:
   In another terminal, navigate to the project directory, then run the compiled proxy server.
   ```bash
   ./proxy_server 127.0.0.1 8080 127.0.0.1 9090
   ```

5. **Test the Proxy Server**:
   Open another terminal, navigate to the project directory, then run the Python client.
   ```bash
   python3 client.py
   ```

   You can now send messages through the client, which will be proxied to the server.

## Files

### `proxy_server.cpp`

The main C++ file containing the implementation of the TCP proxy server.

### `Makefile`

The Makefile to build the proxy server.

### `server.py`

A simple Python server script to test the proxy server.

### `client.py`

A simple Python client script to test the proxy server.

### `.vscode/c_cpp_properties.json`

VS Code configuration for IntelliSense and other C++ features.

### `.vscode/tasks.json`

VS Code tasks for building and cleaning the project.
