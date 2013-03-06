#include "anet.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace std;

int main()
{
	char buf[4000];
	int i;
	for(i=0; i<3000; i++)
		buf[i] = 'a';
	buf[i++] = '\n';
	buf[i] = '\0';

	cout<<buf;
	
	return 0;
}
