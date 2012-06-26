#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

char*
mktemp(char *template)
{
	int n;
	long x;
	char *p;
	int c;
	struct stat stbuf;

	n = strlen(template);
	p = template+n-6;
	if (n < 6 || strcmp(p, "XXXXXX") != 0) {
		*template = 0;
	} else {
		x = getpid() % 100000;
		sprintf(p, "%05d", x);
		p += 5;
		for(c = 'a'; c <= 'z'; c++) {
			*p = c;
			if (stat(template, &stbuf) < 0)
				return template;
		}
		*template = 0;
	}
	return template;
}
