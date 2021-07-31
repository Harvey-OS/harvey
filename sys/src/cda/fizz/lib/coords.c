#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

void f_minor(char *, ...);
void f_major(char *, ...);

void
f_coords(char *s, short *n, Coord **coords)
{
	int loop, nm;
	Coord cds[MAXNET];
	Coord *c;
	char *os;

	if(loop = *s == '{')
		s = f_inline();
	c = cds;
	do {
		BLANK(s);
		if(*s == '}') break;
		if(c == &cds[MAXNET]){
			f_minor("more than %d coords", MAXNET);
			c = cds;
		}
		os = s;
		NAME(s);
		c->chip = f_strdup(os);
		NUM(s, c->pin);
		if(*s) EOLN(s)
		c++;
	} while(loop && (s = f_inline()));
	*n = c-cds;
	*coords = (Coord *)f_malloc(sizeof(Coord)*(long)*n);
	memcpy((char *)*coords, (char *)cds, *n*sizeof(Coord));
}
