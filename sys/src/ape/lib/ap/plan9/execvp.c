/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <unistd.h>
#include <sys/limits.h>
#include <string.h>

extern char **environ;

/*
 * BUG: instead of looking at PATH env variable,
 * just try prepending /bin/ if name fails...
 */

int
execvp(const char *name, const char **argv)
{
	int n;
	char buf[PATH_MAX];

	if((n=execve(name, argv, environ)) < 0){
		strcpy(buf, "/bin/");
		strcpy(buf+5, name);
		n = execve(buf, argv, environ);
	}
	return n;
}
