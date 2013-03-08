all: server client
server: server.cpp db.cpp anet.cpp utils.cpp db.h anet.h utils.h
	g++ -Wall -g server.cpp db.cpp anet.cpp utils.cpp -lpthread -o $@
client: client.cpp anet.cpp anet.h
	g++ -Wall -g $^ -o $@
clean:
	@rm -f server client core
