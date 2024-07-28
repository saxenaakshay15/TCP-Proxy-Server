all: proxy_server

proxy_server: proxy_server.cpp
	g++ -Wall -Werror -O3 -o proxy_server proxy_server.cpp -pthread
