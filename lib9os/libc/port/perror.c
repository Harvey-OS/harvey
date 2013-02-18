#include <u.h>
#include <libc.h>

void
perror(char *s)
{
	char buf[ERRMAX];

	buf[0] = '\0';
	errstr(buf, sizeof buf);
	if(s && *s)
		fprint(2, "%s: %s\n", s, buf);
	else
		fprint(2, "%s\n", buf);
}
