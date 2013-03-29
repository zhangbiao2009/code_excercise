all: server
server: server.cpp db.cpp anet.cpp utils.cpp db.h anet.h utils.h
	g++ -Wall -Wextra -Werror -Wshadow -g server.cpp db.cpp anet.cpp utils.cpp -lpthread -o $@
clean:
	@rm -f server client core
#client: client.cpp anet.cpp anet.h #	g++ -Wall -g $^ -o $@
