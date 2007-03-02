#include <u.h>
#include <libc.h>
#include <bio.h>
#include "authcmdlib.h"

int
readarg(int fd, char *arg, int len)
{
	char buf[1];
	int i;

	i = 0;
	memset(arg, 0, len);
	while(read(fd, buf, 1) == 1){
		if(i < len - 1)
			arg[i++] = *buf;
		if(*buf == '\0'){
			arg[i] = '\0';
			return 0;
		}
	}
	return -1;
}
