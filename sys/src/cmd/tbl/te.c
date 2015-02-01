/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* te.c: error message control, input line count */
# include "t.h"

void
error(char *s)
{
	fprint(2, "\n%s:%d: %s\n", ifile, iline, s);
	fprint(2, "tbl quits\n");
	exits(s);
}


char	*
gets1(char *s, int size)
{
	char	*p, *ns;
	int	nbl;

	iline++;
	ns = s;
	p = Brdline(tabin, '\n');
	while (p == 0) {
		if (swapin() == 0)
			return(0);
		p = Brdline(tabin, '\n');
	}
	nbl = Blinelen(tabin)-1;
	if(nbl >= size)
		error("input buffer too small");
	p[nbl] = 0;
	strcpy(s, p);
	s += nbl;
	for (nbl = 0; *s == '\\' && s > ns; s--)
		nbl++;
	if (linstart && nbl % 2) /* fold escaped nl if in table */
		gets1(s + 1, size - (s-ns));

	return(p);
}


# define BACKMAX 500
char	backup[BACKMAX];
char	*backp = backup;

void
un1getc(int c)
{
	if (c == '\n')
		iline--;
	*backp++ = c;
	if (backp >= backup + BACKMAX)
		error("too much backup");
}


int
get1char(void)
{
	int	c;
	if (backp > backup)
		c = *--backp;
	else
		c = Bgetc(tabin);
	if (c == 0) /* EOF */ {
		if (swapin() == 0)
			error("unexpected EOF");
		c = Bgetc(tabin);
	}
	if (c == '\n')
		iline++;
	return(c);
}


