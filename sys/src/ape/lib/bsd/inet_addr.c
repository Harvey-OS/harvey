/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define CLASS(x)	(x[0]>>6)

unsigned long
inet_addr(char *from)
{
	int i;
	char *p;
	unsigned char to[4];
	unsigned long x;
 
	p = from;
	memset(to, 0, 4);
	for(i = 0; i < 4 && *p; i++){
		to[i] = strtoul(p, &p, 0);
		if(*p == '.')
			p++;
	}

	switch(CLASS(to)){
	case 0:	/* class A - 1 byte net */
	case 1:
		if(i == 3){
			to[3] = to[2];
			to[2] = to[1];
			to[1] = 0;
		} else if (i == 2){
			to[3] = to[1];
			to[1] = 0;
		}
		break;
	case 2:	/* class B - 2 byte net */
		if(i == 3){
			to[3] = to[2];
			to[2] = 0;
		}
		break;
	}
	x = nptohl(to);
	x = htonl(x);
	return x;
}
