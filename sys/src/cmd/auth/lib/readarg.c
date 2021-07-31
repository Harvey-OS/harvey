#include <u.h>
#include <libc.h>
#include "authcmdlib.h"

int
readarg(int fd, char *arg, int len)
{
	char buf[1];
	int i;

	i = 0;
	for(;;){
		if(read(fd, buf, 1) != 1)
			return -1;
		if(i < len - 1)
			arg[i++] = *buf;
		if(*buf == '\0'){
			arg[i] = '\0';
			return 0;
		}
	}
}
