#include <alef.h>
#include "authlocal.h"

int
_asreadn(int fd, byte *buf, int len)
{
	int m, n;

	for(n = 0; n < len; n += m){
		m = read(fd, buf+n, len-n);
		if(m <= 0)
			return -1;
	}
	return n;
}
