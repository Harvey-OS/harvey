#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern char **environ;

/*
 * we need to put the environment into
 * the extern char **environ variable rather than #e/name so that
 * getenv can find it and execve doesn't overwrite it.
 *
 * this is horribly non-thread-safe.
 */

int
putenv(char *s)
{
	int f, n, l, cnt;
	char *value;
	char **p, *q;
	char buf[300];

//	fprintf(stderr, "putenv %s\n", s);
	value = strchr(s, '=');
	if(!value)
		return -1;

	l = value+1-s;
	for(p=environ, cnt=0; *p; p++, cnt++) {
		if(strncmp(*p, s, l) == 0)
			break;
	}

	/* already same value? */
	if(*p && strcmp(*p, s) == 0)
		return 0;

	/* allocate new string before messing with environ */
	q = strdup(s);
	if(q == 0)
		return -1;

	if(*p == 0) {
		/* did not find: make room; what a kludge */
		p = realloc(environ, (cnt+2)*sizeof(*p));
		if(p == 0)
			return -1;

		environ = p;
		p[cnt+1] = 0;
		p = environ+cnt;
	}

	*p = q;
	return 0;
}
