#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <ctype.h>

char id[10][10];

doline(char *s)
{
	char *p;
	char buf[200];
	int i, n=0;
	for (p = id[n]; *s != ' '; s++)
		*p++ = *s;
	*p++ = 0;
	n++;
	for (; !islower(*s); s++)
		;
	for (; *s;) {
		for (; !isupper(*s) && !isdigit(*s); s++)
			if (*s == '\n')
				goto wrapup;
			else
				putchar(*s);
		if (isdigit(*s)) {
			fprintf(stdout, "C%c", *s++);
			continue;
		}
		else
			putchar('e');
		for (p = id[n]; isalnum(*s); s++)
			*p++ = *s;
		*p++  = 0;
		n++;
	}
wrapup:
	fprintf(stdout, "	#	%s	Y", id[0]);
	for (i = 1; i < n; i++)
		fprintf(stdout, "	%s", id[i]);
	putchar('\n');
}

main(void)
{
	char ibuf[100];
	while (fgets(ibuf, sizeof ibuf, stdin))
		doline(ibuf);
	exits(0);
}
