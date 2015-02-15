/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int
putenv(char *s)
{
	int f, n;
	char *value;
	char buf[300];

	value = strchr(s, '=');
	if (value) {
		n = value-s;
		if(n<=0 || n > sizeof(buf)-6)
			return -1;
		strcpy(buf, "/env/");
		strncpy(buf+5, s, n);
		buf[n+5] = 0;
		f = creat(buf, 0666);
		if(f < 0)
			return 1;
		value++;
		n = strlen(value);
		if(write(f, value, n) != n)
			return -1;
		close(f);
		return 0;
	} else
		return -1;
}
