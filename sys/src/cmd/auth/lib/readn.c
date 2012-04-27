#include <u.h>
#include <libc.h>
#include <bio.h>
#include "authcmdlib.h"

/*
 *  read exactly len bytes
 */
int
readn(int fd, char *buf, int len)
{
	int m, n;

	for(n = 0; n < len; n += m){
		m = read(fd, buf+n, len-n);
		if(m <= 0)
			return -1;
	}
	return n;
}
