#define _POSIX_SOURCE
#define _RESEARCH_SOURCE
#include <error.h>
#include <stdio.h>
#include <libv.h>

char *_progname;

void
_perror(char *s)
{
	fprintf(stderr, "%s: ", _progname);
	perror(s);
}
