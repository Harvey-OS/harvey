#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

typedef struct Reserved	Reserved;

struct Reserved {
	char *	name;
	int	token;
};

static Reserved keywords[] = {
	"BLOB", BLOB,
	"END", END,
	"ETC", ETC,
	"INC", INC,
	"PATH", PATH,
	"RECT", RECT,
	"RECTI", RECTI,
	"REPEAT", REPEAT,
	"RING", RING,
	"TEXT", TEXT,
	0, 0
};

Sym *	op_plus;
Sym *	op_minus;

void
xyinit(void)
{
	static int inited;
	Reserved *r;
	Sym *s;

	if(inited)
		return;
	inited = 1;
	strunique = 1;
	for(r=keywords; r->name; r++){
		s = symstore(r->name);
		if(s->type)
			error("keyword botch");
		s->ptr = (void *)r->token;
		s->type = Keyword;
	}

	op_plus = symstore("+");
	op_plus->type = Operator;
	op_plus->ptr = (void *)(S|D);

	op_minus = symstore("-");
	op_minus->type = Operator;
	op_minus->ptr = (void *)(D&~S);

	strunique = 0;
}

char *
nextstr(char **pp)
{
	char *p, *q;

	p = *pp;
	while(*p == ' ')
		p++;
	q = p;
	while(*q && *q != ' ' && *q != ',')
		q++;
	if(*q == ',')
		*q++ = 0;
	while(*q == ' ')
		*q++ = 0;
	*pp = q;
	return p;
}

int
nextint(char **pp, int missing)
{
	char *p = *pp;
	int x = missing;

	while(*p == ' ')
		p++;
	if(*p && *p != ','){
		if(*p != '*')
			x = strtol(p, &p, 10);
		else
			++p;
	}
	if(*p == ',')
		++p;
	while(*p == ' ')
		p++;
	if(x == Required)
		error("required parameter missing");
	*pp = p;
	return x;
}

Point
nextpt(char **pp)
{
	int x, y;

	x = nextint(pp, 0);
	y = nextint(pp, 0);

	return (Point){x, y};
}
