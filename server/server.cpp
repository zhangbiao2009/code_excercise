#include "anet.h"
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>

using namespace std;

#define MAX_CLIENTS 100
#define MAX_BUFLEN 10000
#define MAX_CMD_PARTS 3

enum State{CMD_CHECK, CMD_PARSE, GET_CMD, SET_CMD};
struct client{
	client():fd(-1),rpos(0),cmd_end(NULL),state(CMD_CHECK),wpos(0),wend(0){}
	void reset_for_next_read(){
		rpos = 0;
		cmd_end = NULL;
		state = CMD_CHECK;
	}
	void reset_for_next_write(){
		wpos = 0;
		wend = 0;
	}
	int fd;
	char readbuf[MAX_BUFLEN];
	int rpos;		//next pos which input need to store
	char* cmd_end;		//for command parsing, init as NULL
	char* cmd_part[MAX_CMD_PARTS];  //parsed command
	State state;
	char writebuf[MAX_BUFLEN];
	int wpos;		//next pos which need to output 
	int wend;		//the end of the data in write buf, equals to the array index of the last character + 1
};

/*
typedef struct aeFileEvent {
    int mask; 
    aeFileProc *rfileProc;
    aeFileProc *wfileProc;
    void *clientData;
} aeFileEvent;

*/

struct epoll_event events[MAX_CLIENTS];
struct client clients[MAX_CLIENTS];
int nclients;

int handle_read_event(client* c)
{
	while(1){
		switch(c->state){
			case CMD_CHECK:
				c->cmd_end = (char*)memchr(c->readbuf, '\n', c->rpos);
				if(!c->cmd_end)	//not a complete cmd
					return 0;

				//received a complete cmd
				if(*(c->cmd_end-1) != '\r')	//bad format
					return -1;
				c->state = CMD_PARSE;
				break;
			case CMD_PARSE:
				{
					//parse command
					int npart = 0;
					*(c->cmd_end-1) = '\0';	//set for strtok
					for (char* p=c->readbuf; ; p = NULL) {
						char* token = strtok(p, " ");
						if (token == NULL)
							break;
						c->cmd_part[npart++] = token;
					}
					if(strcmp(c->cmd_part[0], "get") == 0) //get command
						c->state = GET_CMD;
					else if(strcmp(c->cmd_part[0], "set") == 0)
						c->state = SET_CMD;
					else return -1;		//bad command
					break;
				}
			case GET_CMD:
				sprintf(c->writebuf, "get key: %s, value: val1\r\n", c->cmd_part[1]);
				c->wend = strlen(c->writebuf) + 1;		//ignore '\0' in the end of C style string
				c->wpos = 0;
				return 1;
				break;
			case SET_CMD:
				{
					int bytes = atoi(c->cmd_part[2]);	//bytes of data block, to do: what if byte is 0 ?
					if(c->readbuf+c->rpos-(c->cmd_end+1) < bytes + 2) //data not received completelly, +2 for '\r\n'
						return 0;

					c->readbuf[c->rpos-2] = '\0'; //replace '\r' with '\0', make the data as a C style string
					sprintf(c->writebuf, "set key: %s, value: %s\r\n", c->cmd_part[1], c->cmd_end+1);
					c->wend = strlen(c->writebuf) + 1;
					c->wpos = 0;
					return 1;
					break;
				}
		}
	}
}

int main()
{
	char neterr[ANET_ERR_LEN];
	struct epoll_event ev; 

	int listenfd;
	int epfd; 

	signal(SIGPIPE, SIG_IGN);	//ignore sigpipe to prevent server shut down

	if((listenfd = anetTcpServer(neterr, 9999, NULL)) == ANET_ERR){
		fprintf(stderr, "%s\n", neterr);
		exit(1);
	}

	anetNonBlock(neterr, listenfd); 

	if((epfd = epoll_create(MAX_CLIENTS)) < 0){
		perror("epoll_create");
		exit(1);
	}

	ev.events = EPOLLIN; 
	ev.data.fd =listenfd; 
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev)<0){
		perror("epoll_ctl");
		exit(1);
	} 

	for(;;){ 

		int nfds = epoll_wait(epfd, events, MAX_CLIENTS, -1);  
		for(int i = 0; i < nfds; ++i) { 

			int fd = events[i].data.fd;

			if(fd  == listenfd) { 
				struct sockaddr_in	cliaddr;
				socklen_t addrlen = sizeof(cliaddr);
				int connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &addrlen); 
				if(connfd < 0){ 
					if(errno == ECONNABORTED)	//connection aborted, ignore it
						continue; 
					else if(errno == EAGAIN)	//no connections are ready to be accepted
						continue; 
					else{	//error happened in accept, report it
						perror("accept"); 
						continue;
					}
				} 
				
				fprintf(stderr, "a new connfd: %d\n", connfd);  
				anetNonBlock(neterr, connfd); 

				ev.events = EPOLLIN; 
				ev.data.fd = connfd; 
				if (epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev) < 0) { 
					perror("epoll_ctl");
					return -1; 
				} 
				clients[connfd].fd = connfd;
				nclients++;
			} 
			else if(events[i].events & EPOLLIN)  
            {  
				int n;
				char* rbuf = clients[fd].readbuf;
                while ((n = read(fd, rbuf+clients[fd].rpos, MAX_BUFLEN-clients[fd].rpos)) > 0)
                    clients[fd].rpos += n;

				if (n == 0) {  //client has been closed
                    close(fd);  
					nclients--;
				}
				if (n<0 && errno != EAGAIN) {
					perror("read");
					close(fd);  
					nclients--;
				}
				int res = handle_read_event(&clients[fd]);
				if(res<0){ //error happened
					fprintf(stderr, "an error happened\n");  
					close(fd);  
					nclients--;
				}else if(res>0){	//read event is handled successfully
					clients[fd].reset_for_next_read();
					//register as write event
					ev.data.fd=fd;  
					ev.events=EPOLLOUT;  
					epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);  
				}else{ //res == 0, needs more data, continue reading
				}
            }
            else if(events[i].events & EPOLLOUT)  
            {     
				char* wbuf = clients[fd].writebuf;
				int wend = clients[fd].wend;
				int n;
                while (wend>clients[fd].wpos && 
						(n = write(fd, wbuf+clients[fd].wpos, wend-clients[fd].wpos)) > 0)
					clients[fd].wpos += n;

				if (n<0 && errno != EAGAIN) {
					perror("write");
					close(fd);  
					nclients--;
				}

				if(wend == clients[fd].wpos){ //write finished
					//fprintf(stderr, "write to fd %d, content: %s", fd, wbuf);  
					//no need to rest for next write cause read event handler will do it
					//register as read event
					ev.data.fd=fd;  
					ev.events=EPOLLIN;  
					epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);  
				}
            }  
			else{ 
				//impossible!
		  	}
		}
	}

	return 0;
}
