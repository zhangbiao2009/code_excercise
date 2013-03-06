all: server client
server: server.cpp anet.cpp 
	g++ -g server.cpp anet.cpp -o server
client: client.cpp anet.cpp
	g++ -g client.cpp anet.cpp -o client
