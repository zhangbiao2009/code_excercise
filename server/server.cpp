#include "anet.h"
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>

using namespace std;

#define MAX_CLIENTS 100
#define MAX_BUFLEN 10000

struct client{
	int fd;
	char readbuf[MAX_BUFLEN];		//+1 for the end of C style string
	char writebuf[MAX_BUFLEN];
	int rpos;
	int wpos;
	int wend;
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

int main()
{
	char neterr[ANET_ERR_LEN];
	struct epoll_event ev; 

	int listenfd;
	int epfd; 

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
				if(rbuf[clients[fd].rpos-2] == '\n' && rbuf[clients[fd].rpos-1] == '\0'){ //read a line finished, [rpos-1] should be '\0'
					//process the request
					strcpy(clients[fd].writebuf, rbuf); //copy to write buf for response
					//fprintf(stderr, "from fd %d, received : %s", fd, rbuf);  
					clients[fd].wend = clients[fd].rpos ;

					clients[fd].rpos = 0;	//reset rpos
					//register as write event
					ev.data.fd=fd;  
					ev.events=EPOLLOUT;  
					epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);  
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
					clients[fd].wpos = 0;
					clients[fd].wend = 0;

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
