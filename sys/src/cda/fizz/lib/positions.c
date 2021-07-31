#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

void f_minor(char *, ...);

void
f_positions(char *s)
{
	int nm, loop;
	char *os;
	Chip *c;

	BLANK(s);
	if(loop = *s == '{')
		s = f_inline();
	do {
		if(*s == '}') break;
		BLANK(s)
		os = s;
		NAME(s)
		if((c = (Chip *)symlook(os, S_CHIP, (void *)0)) == 0){
			c = NEW(Chip);
			c->name = f_strdup(os);
			(void)symlook(c->name, S_CHIP, (void *)c);
		}
		COORD(s, c->pt.x, c->pt.y)
		NUM(s, c->rotation)
		NUM(s, c->flags)
		if(*s) EOLN(s)
	} while(loop && (s = f_inline()));
}
