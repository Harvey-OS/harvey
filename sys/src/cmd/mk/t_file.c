#include	"mk.h"

#define	MYSEP(r)	((r == ' ') || (r == ',') || (r == '\n'))

void
timeinit(char *s)
{
	long t;
	char *cp;
	Rune r;
	int c, n;

	t = time(0);
	while (*s) {
		for(cp = s; *s; s += n) {
			n = chartorune(&r, s);
			if (MYSEP(r))
				break;
		}
		c = *s;
		*s = 0;
		symlook(strdup(cp), S_TIME, (char *)t)->value = (char *)t;
		if (c)
			*s++ = c;
		while (*s) {
			n = chartorune(&r, s);
			if (!MYSEP(r))
				break;
			s += n;
		}
	}
}

long
ftimeof(int force, char *name, char *x1)
{
	USED(x1);
	if(force)
		return mtime(name);
	return filetime(name);
}

void
ftouch(char *name, char *x1, char *x2)
{
	Dir sbuf;

	USED(x1, x2);
	if(dirstat(name, &sbuf) >= 0) {
		sbuf.mtime = time((long *)0);
		if(dirwstat(name, &sbuf) >= 0)
			return;
	} else if(create(name, OWRITE, 0666) >= 0)
		return;
	perror(name);
	Exit();
}

void
fdelete(char *s, char *x1, char *x2)
{
	USED(x1, x2);
	if(remove(s) < 0)
		perror(s);
}
