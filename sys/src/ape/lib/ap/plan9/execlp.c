/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <unistd.h>
#include <string.h>
#include <sys/limits.h>

/*
 * BUG: instead of looking at PATH env variable,
 * just try prepending /bin/ if name fails...
 */

extern char **environ;

int
execlp(const char *name, const char *arg0, ...)
{
	int n;
	char buf[PATH_MAX];

	if((n=execve(name, &arg0, environ)) < 0){
		strcpy(buf, "/bin/");
		strcpy(buf+5, name);
		n = execve(buf, &name+1, environ);
	}
	return n;
}
