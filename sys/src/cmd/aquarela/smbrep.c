#include "headers.h"

int
smbmatch(char *name, Reprog *rep)
{
	Resub sub;
	sub.sp = nil;
	sub.ep = nil;
	if (regexec(rep, name, &sub, 1) && sub.sp == name && *sub.ep == 0)
		return 1;
	return 0;
}

Reprog *
smbmkrep(char *pattern)
{
	Reprog *r;
	int l;
	char *tmp, *p, *q, *t;
	l = strlen(pattern);
	tmp = smbemalloc(l * 5 + 1);
	t = tmp;
	p = pattern;
	while (*p) {
		if (*p == '*') {
			if (p[1] == '.') {
				strcpy(t, "[^.]*");
				t += 5;
				p++;
			}
			else {
				*t++ = '.';
				*t++ = '*';
				p++;
			}
		}
		else if (*p == '?') {
			for (q = p + 1; *q && *q == '?'; q++)
				;
			if (*q == 0 || *q == '.') {
				/* at most n copies */
				strcpy(t, "[^.]?");
				t += 5;
				p++;
			}
			else {
				/* exactly n copies */
				strcpy(t, "[^.]");
				t += 4;
				p++;
			}
		}
		else if (strchr(".+{}()|\\^$", *p) != 0) {
			/* regexp meta */
			*t++ = '\\';
			*t++ = *p++;
		}
		else
			*t++ = *p++;
	}
	*t = 0;
	smblogprintif(smbglobals.log.rep, "%s => %s\n", pattern, tmp);
	r = regcomp(tmp);
	free(tmp);
	return r;
}

