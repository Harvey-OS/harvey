#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <ctype.h>

#define NID	20
#define DEPTH	20
char id[NID][10];
char pos[NID][DEPTH];

doline(char *s)
{
	char *p;
	char buf[400];
	char stack[DEPTH],*sp;
	int i, j, n=0;
	int dup;
	for (p = id[n]; *s != ' '; s++)
		*p++ = *s;
	*p++ = 0;
	n++;
	for (; !islower(*s); s++)
		;
	sp = stack;
	for (; *s;) {
		for (; !isupper(*s) && !isdigit(*s); s++) {
			switch (*s) {
			case '(':
				sp[1] = '.';
				sp[2] = '1';
				sp[3] = 0;
				sp += 2;
				break;
			case ',':
				*sp += 1;
				break;
			case ')':
				sp -= 2;
				sp[1] = 0;
				break;
			case '\n':
				goto wrapup;
			}
			putchar(*s);
		}
		if (isdigit(*s)) {
			fprintf(stdout, "C%c", *s++);
			continue;
		}
		else
			putchar('e');
		for (p = id[n]; isalnum(*s); s++)
			*p++ = *s;
		*p++  = 0;
		strcpy(pos[n], &stack[2]);
		n++;
	}
wrapup:
	printf("\t");
	for (i = 1, dup = 0; i < n-1; i++)
		for (j = i+1; j < n; j++)
			if (id[i][0] != '#' && strcmp(id[i], id[j]) == 0) {
				printf("%s%s=%s", dup ? ":" : "", pos[i], pos[j]);
				strcpy(id[j], "#");
				dup = 1;
			}
	if (dup == 0)
		printf("#");
	fprintf(stdout, "	%s	Y", id[0]);
	for (i = 1; i < n; i++)
		fprintf(stdout, "	%s", id[i]);
	putchar('\n');
}

dofile(FILE *f)
{
	char ibuf[400];
	while (fgets(ibuf, sizeof ibuf, f))
		doline(ibuf);
}

main(int argc, char *argv[])
{
	ARGBEGIN {
	case 'l':
		printf(": %s\n", ARGF());
	} ARGEND;
	if (argc > 0)
		freopen(argv[0], "r", stdin);
	if (stdin)
		dofile(stdin);
	exits(0);
}
