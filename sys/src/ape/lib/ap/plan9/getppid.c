#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "sys9.h"

pid_t
getppid(void)
{
	int n, f;
	char ppidbuf[15];

	f = open("#c/ppid", 0);
	n = read(f, ppidbuf, sizeof ppidbuf);
	if(n < 0)
		errno = EINVAL;
	else
		n = atoi(ppidbuf);
	close(f);
	return n;
}
