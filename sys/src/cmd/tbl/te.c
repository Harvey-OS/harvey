/* te.c: error message control, input line count */
# include "t.h"

void
error(char *s)
{
	fprintf(stderr, "\n%s: line %d: %s\n", ifile, iline, s);
	fprintf(stderr, "tbl quits\n");
	exits(s);
}


char	*
gets1(char *s, int size)
{
	char	*p, *ns;
	int	nbl;

	iline++;
	ns = s;
	p = fgets(s, size, tabin);
	while (p == 0) {
		if (swapin() == 0)
			return(0);
		p = fgets(s, size, tabin);
	}

	while (*s) 
		s++;
	s--;
	if (*s == '\n') 
		*s-- = 0;
	else error("input buffer too small");
	for (nbl = 0; *s == '\\' && s > p; s--)
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
		c = getc(tabin);
	if (c == EOF) /* EOF */ {
		if (swapin() == 0)
			error("unexpected EOF");
		c = getc(tabin);
	}
	if (c == '\n')
		iline++;
	return(c);
}


