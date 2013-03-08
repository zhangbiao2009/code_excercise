/*
using a memcached like protocol:

get <key>\r\n
-----------------------
value <key> <nbytes>\r\n
<data block>\r\n
|NOT_FOUND\r\n


del <key>\r\n
-----------------------
DELETED|NOT_FOUND\r\n


add <key> <nbytes>\r\n
<data block>\r\n
-----------------------
STORED|EXISTS\r\n


set <key> <nbytes>\r\n
<data block>\r\n
-----------------------
STORED|NOT_FOUND\r\n

*/

#include "anet.h"
#include "utils.h"
#include "db.h"
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <map>

using namespace std;

DB db;

int epfd = -1; 

#define MAX_CLIENTS 100
#define MAX_BUFLEN 10000
#define MAX_CMD_PARTS 3

enum State{CMD_CHECK, CMD_PARSE, GET_CMD, DEL_CMD, ADD_CMD, SET_CMD};
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

struct epoll_event events[MAX_CLIENTS];
struct client clients[MAX_CLIENTS];
int nclients;

void deregister_event_helper(client* c, uint32_t event)
{
	struct epoll_event ev; 
	ev.data.fd=c->fd;  
	ev.events=event;  
	epoll_ctl(epfd, EPOLL_CTL_DEL, c->fd, &ev);  
}

void get_ready_to_write(client* c)
{
	c->wend = strlen(c->writebuf) + 1;		//ignore '\0' in the end of C style string
	c->wpos = 0;
	struct epoll_event ev; 
	ev.data.fd=c->fd;  
	ev.events=EPOLLOUT;  
	epoll_ctl(epfd, EPOLL_CTL_ADD, c->fd, &ev);  
}

class GetTask : public Task{
	public:
		GetTask(client* c):client_(c){}
		virtual ~GetTask(){}
		virtual void Process(){
			string key = client_->cmd_part[1];
			string val;
			if(db.Get(key, &val))
				sprintf(client_->writebuf, "VALUE %s %d\r\n%s\r\n", 
						key.c_str(), (int)key.length(), val.c_str());
			else
				sprintf(client_->writebuf, "NOT_FOUND\r\n");

			get_ready_to_write(client_);
		}
	private:
	client* client_;
};

class DelTask : public Task{
	public:
		DelTask(client* c):client_(c){}
		virtual ~DelTask(){}
		virtual void Process(){
			string key = client_->cmd_part[1];
			string val;
			if(db.Del(key))
				sprintf(client_->writebuf, "DELETED\r\n"); 
			else
				sprintf(client_->writebuf, "NOT_FOUND\r\n"); 

			get_ready_to_write(client_);
		}
	private:
	client* client_;
};

class AddTask : public Task{
	public:
		AddTask(client* c):client_(c){}
		virtual ~AddTask(){}
		virtual void Process(){
			client_->readbuf[client_->rpos-2] = '\0'; //replace '\r' with '\0', make the data as a C style string
			string key = client_->cmd_part[1];
			string val = client_->cmd_end+1;

			if(db.Add(key, val))
				sprintf(client_->writebuf, "STORED\r\n");
			else
				sprintf(client_->writebuf, "EXISTS\r\n");
			get_ready_to_write(client_);
		}
	private:
	client* client_;
};

class SetTask : public Task{
	public:
		SetTask(client* c):client_(c){}
		virtual ~SetTask(){}
		virtual void Process(){
			client_->readbuf[client_->rpos-2] = '\0'; //replace '\r' with '\0', make the data as a C style string
			string key = client_->cmd_part[1];
			string val = client_->cmd_end+1;

			if(db.Set(key, val))
				sprintf(client_->writebuf, "STORED\r\n");
			else
				sprintf(client_->writebuf, "NOT_FOUND\r\n");
			get_ready_to_write(client_);
		}
	private:
	client* client_;
};

ThreadPool thread_pool(5);

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
					else if(strcmp(c->cmd_part[0], "del") == 0)
						c->state = DEL_CMD;
					else if(strcmp(c->cmd_part[0], "add") == 0)
						c->state = ADD_CMD;
					else if(strcmp(c->cmd_part[0], "set") == 0)
						c->state = SET_CMD;
					else return -2;		//bad command
					break;
				}
			case GET_CMD:
			case DEL_CMD:
				{
					Task* p;
					if(c->state == GET_CMD) 
						p = new GetTask(c);
					else
						p = new DelTask(c);
					deregister_event_helper(c, EPOLLIN);	//stop receiving data from client to protect member variables in this client
					thread_pool.AddTask(p);
					return 1;
					break;
				}
			case ADD_CMD:
			case SET_CMD:
				{
					int bytes = atoi(c->cmd_part[2]);	//bytes of data block, to do: what if byte is 0 ?
					if(c->readbuf+c->rpos-(c->cmd_end+1) < bytes + 2) //data not received completelly, +2 for '\r\n'
						return 0;

					Task* p;
					if(c->state == ADD_CMD) 
						p = new AddTask(c);
					else
						p = new SetTask(c);
					deregister_event_helper(c, EPOLLIN);
					thread_pool.AddTask(p);
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

	if(!db.Open("mydb")){
		cerr<<"db open failed:"<<endl;
		return 0;
	}

	signal(SIGPIPE, SIG_IGN);	//ignore sigpipe to prevent server shut down

	thread_pool.Run();

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
					fprintf(stderr, "client has been closed, so server also close it, fd=%d\n", fd);  
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
					fprintf(stderr, "bad command, res=%d\n", res);  
					close(fd);  
					nclients--;
				}else if(res>0){	//the command has been hand over to a background thread, nothing to do here
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
					//no need to rest for next write cause read event handler will do it
					//register as read event
					clients[fd].reset_for_next_read();	//reset for read
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
