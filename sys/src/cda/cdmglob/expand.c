#include "cdmglob.h"
#include <ctype.h>
#define NSTRS	256
#define NSTRBUF	4000




static char	buf[NSTRBUF],
		*strs[NSTRS],
		strbuf[NSTRBUF];

static int	mask,
		nstrs,
		nstrbuf;


/*
 * x<0:3>y ==> x0y, x1y, x2y, x3y
 * x<3:0>y ==> x3y, x2y, x1y, x0y
 * x<00:03>y ==> x00y, x01y, x02y, x03y
 */

int
anglexpand (char *head, char *tail)
{
	int	hi = 0, hilen = 0, incr, len, lo = 0, lolen = 0, n;

	if (*tail++ != '<')
		goto syntax;

	for (; isdigit (*tail); lolen++, lo = 10 * lo + *tail++ - '0');

	if (*tail++ != ':')
		goto syntax;

	for (; isdigit (*tail); hilen++, hi = 10 * hi + *tail++ - '0');

	if (*tail == ':') {
		incr = hi;
		for (hi = hilen = 0, tail++; isdigit (*tail); hilen++)
			hi = 10 * hi + *tail++ - '0';
	}
	else	incr = 1;
	if (*tail++ != '>' || lolen <= 0 || hilen <= 0 || incr < 0)
		goto syntax;

	if (lo > hi)
		incr = -incr;
	if (hilen < lolen)
		hilen = lolen;

	for (;; lo += incr) {
		for (len = hilen-1, n = lo; len >= 0; len--, n /= 10)
			head[len] = n % 10 + '0';
		doexpand (head + hilen, tail);
		if (incr > 0 && lo >= hi ||
		    incr < 0 && lo <= hi)
			break;
	}

	return;

syntax:
	error ("<> syntax");
}



/*
 * copy "n" bytes from "s" to "t".  
 * (the VAX asm version is limited to 65535 bytes).
 */

void
bcopy (char *t, char *s, int n)
{
	while (n-- > 0)
		*t++ = *s++;
}


/*
 * a{x,y,z}b or a{{x,y},z}b ==> axb, ayb, azb
 * a{,x}b ==> a, xb
 */

void
curlyexpand (char *head, char *tail)
{
	char	buf2[NSTRBUF];
	char	*s, *t;
	int	depth;

	if (*tail++ != '{')
		goto syntax;

	for (s = tail, depth = 1; *tail; tail++)
		if (*tail == '{')
			depth++;
		else if (*tail == '}' && --depth <= 0)
			break;

	if (*tail++ != '}')
		goto syntax;

	for (depth = 1;; s++) {
		for (t = buf2;; *t++ = *s++)
			if (*s == '{')
				depth++;
			else if (*s == ',' && depth <= 1 ||
				 *s == '}' && depth-- <= 1)
					break;
		(void) strcpy (t, tail);
		doexpand (head, buf2);
		if (*s == '}')
			return;
	}

syntax:
	error ("{} syntax");
}


int
doexpand (char *head, char *tail)
{
	register int	len;

	for (; *tail; *head++ = *tail++) {
		if ((*tail == '[' || *tail == ']') && mask)
			squarexpand (head, tail);
		else if (*tail == '<' || *tail == '>')
			anglexpand (head, tail);
		else if (*tail == '{' || *tail == '}')
			curlyexpand (head, tail);
		else	continue;
		return;
	}

	*head = 0;
	if (1 >= (len = 1 + head - buf))
		return;
	if (nstrs >= NSTRS)
		fatal ("expand: nstrs");
	if (nstrbuf + len > NSTRBUF)
		fatal ("expand: nstrbuf");
	bcopy (strs[nstrs++] = strbuf + nstrbuf, buf, len);
	nstrbuf += len;
	return;
}

/*
 * x.y ==> (x>>y)&1
 */
void
numify(char *s)
{
	int x,y,c;
	char *p = s;
	if (!isdigit(c=*p++))
		return;
	for (x = c-'0'; isdigit(c=*p++);)
		x = x*10 + c-'0';
	if (c != '.')
		return;
	if (!isdigit(c=*p++))
		return;
	for (y = c-'0'; isdigit(c=*p++);)
		y = y*10 + c-'0';
	if (c != 0)
		return;
	*s++ = "01"[(x>>y)&1];
	*s = 0;
}

int
expand (char *s, char ***v, int msk)
{
	int i;
	mask = msk;
	nstrs = nstrbuf = 0;
	*v = 0;
	doexpand (buf, s);
	if (nstrs <= 0)
		error ("%s: empty expanse", s);
	for (i = 0; i < nstrs; i++)
		numify(strs[i]);	/* bart bit vector hack */
	*v = strs;
	return (nstrs);
}


/*
 * x[a-c]y, x[abc]y, x[a-bc-c], etc. ==> xay, xby, xcy
 * x[c-a] ==> xcy, xby, xay
 */

int
squarexpand (char *head, char *tail)
{
	char	*t;
	int	hi, incr, lo;

	if (*tail++ != '[')
		goto syntax;

	for (t = tail; *tail && *tail != ']'; tail++);

	if (*tail++ != ']')
		goto syntax;

	for (; *t != ']'; t++) {
		lo = *t;
		if (t[1] == '-' && t[2] != ']')
			hi = *(t += 2);
		else	hi = lo;
		incr = lo > hi ? -1 : 1;
		for (; lo - incr != hi; lo += incr) {
			*head = lo;
			doexpand (head + 1, tail);
		}
	}

	return;

syntax:
	error ("[] syntax");
	return;
}
