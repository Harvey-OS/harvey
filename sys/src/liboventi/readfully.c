#include <u.h>
#include <libc.h>
#include <oventi.h>
#include "session.h"

int
vtFdReadFully(int fd, uchar *p, int n)
{
	int nn;

	while(n > 0) {
		nn = vtFdRead(fd, p, n);
		if(nn <= 0)
			return 0;
		n -= nn;
		p += nn;
	}
	return 1;
}
