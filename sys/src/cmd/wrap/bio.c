#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include "wrap.h"

int
Bcopy(Biobuf *bout, Biobuf *bin, vlong length)
{
	int c;

	while(length > 0 && (c = Bgetc(bin)) >= 0) {
		Bputc(bout, c);
		length--;
	}
	if(length != 0) {
		werrstr("copy ran out of input");
		return -1;
	}
	return 0;
}

int
Bcopyfile(Biobuf *b, char *file, vlong length)
{
	Biobuf *bin;
	int rv;

	if((bin = Bopen(file, OREAD)) == nil)
		return -1;

	rv = Bcopy(b, bin, length);
	Bterm(bin);
	return rv;
}

char*
Bgetline(Biobuf *b)
{
	char *p, *q;

	if((p = Brdline(b, '\n')) == nil)
		return nil;

	q = emalloc(Blinelen(b));
	memmove(q, p, Blinelen(b)-1);
	q[Blinelen(b)-1] = '\0';
	return q;
}

int
Bdrain(Biobuf *b, vlong len)
{
	char buf[8192];
	int n;

	while(len > 0) {
		n = sizeof(buf);
		if(n > len)
			n = len;
		if((n = Bread(b, buf, n)) < 0)
			return -1;
		if(n == 0) {
			werrstr("drain ran out of input");
			return -1;
		}
		len -= n;
	}
	return 0;
}

static int
Bgetbackc(Biobuf *b)
{
	int c;
	long m, n;

	/* attempt to keep what we're looking at in the buffer */
	m = Boffset(b);
	n = m-1024;
	if(n < 0)
		n = 0;
	Bseek(b, n, 0);
	Bgetc(b);	/* actually fetch buffer */

	Bseek(b, m-1, 0);
	c = Bgetc(b);
	Bungetc(b);
	return c;
}

/*
 * binary search for a line with first field `p'
 * in a file.
 */
char*
Bsearch(Biobuf *b, char *p)
{
	long lo, hi, m;
	int l;
	char *s;

	if(b == nil)
		return nil;

	lo = 0;
	Bseek(b, 0, 2);
	hi = Boffset(b);

	l = strlen(p);
	while(lo < hi) {
		m = (lo+hi)/2;
		Bseek(b, m, 0);
		Brdline(b, '\n');
		if(Boffset(b) == hi) {
			/* must back up one line */
			Bgetbackc(b);
			m = Boffset(b);
			while(m-- > lo)
				if(Bgetbackc(b) == '\n') {
					Bgetc(b);
					break;
				}
		}
		s = Brdline(b, '\n');
		if(strprefix(s, p) == 0 && (s[l] == ' ' || s[l] == '\n'))
			return s;
		if(strcmp(s, p) < 0)
			lo = Boffset(b);
		else
			hi = Boffset(b)-Blinelen(b);
	}
	return nil;
}

