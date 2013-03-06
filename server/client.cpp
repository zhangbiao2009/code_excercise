#include "anet.h"
#include <netinet/in.h>
#include <errno.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>

using namespace std;

#define MAX_CLIENTS 100
#define MAX_BUFLEN 10000

char buf[MAX_BUFLEN];
char recv_buf[MAX_BUFLEN];

int main()
{
	char neterr[ANET_ERR_LEN];
	int rpos = 0;
	int sockfd;

	if((sockfd = anetTcpConnect(neterr, (char*)"127.0.0.1", 9999)) == ANET_ERR){
		fprintf(stderr, "%s\n", neterr);
		exit(1);
	}

	while(fgets(buf, MAX_BUFLEN, stdin) != NULL){
		rpos = 0;
		int len = strlen(buf) + 1;
		if(write(sockfd, buf, len)<0){
			perror("write");
			exit(1);
		}
		int n;
		while((n= read(sockfd, recv_buf+rpos, len-rpos)) > 0)
			rpos += n;

		if(rpos == len){ //read finished
			fprintf(stderr, "%s", recv_buf);
			continue;
		}

		//arriving here means error happened

		if(n == 0){	//server closed the socket
			close(sockfd);
			return 0;
		}else if (n<0){
			perror("read");
			close(sockfd);
			exit(1);
		}
			
	}

	return 0;
}
