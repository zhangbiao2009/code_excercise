all: server client
server: server.cpp anet.cpp 
	g++ -Wall -g $^ -lpthread -o $@
client: client.cpp anet.cpp
	g++ -Wall -g $^ -o $@
clean:
	@rm -f server client
