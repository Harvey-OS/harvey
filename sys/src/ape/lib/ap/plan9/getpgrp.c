#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "sys9.h"

pid_t
getpgrp(void)
{
	int n, f;
	char pgrpbuf[15];

	f = open("#c/pgrp", 0);
	n = read(f, pgrpbuf, sizeof pgrpbuf);
	if(n < 0)
		errno = EINVAL;
	else
		n = atoi(pgrpbuf);
	close(f);
	return n;
}
