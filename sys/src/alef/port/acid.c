#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"
#include "y.tab.h"

char lcode[] =
{
	[TINT]		'D',
	[TUINT]		'U',
	[TSINT]		'd',
	[TSUINT]	'u',
	[TCHAR]		'C',
	[TFLOAT]	'F',
	[TIND]		'X',
	[TCHANNEL]	'X',
	[TARRAY]	'a',
};

static char *kwd[] =
{
	"$adt", "$aggr", "$append", "$complex", "$defn",
	"$delete", "$do", "$else", "$eval", "$head", "$if",
	"$local", "$loop", "$return", "$tail", "$then",
	"$union", "$whatis", "$while",
};

char*
amap(char *s)
{
	int i, bot, top, new;

	bot = 0;
	top = bot + nelem(kwd) - 1;
	while(bot <= top){
		new = bot + (top - bot)/2;
		i = strcmp(kwd[new]+1, s);
		if(i == 0)
			return kwd[new];

		if(i < 0)
			bot = new + 1;
		else
			top = new - 1;
	}
	return s;
}

int
icomplex(int o, Type *t)
{
	Type *f;
	Sym *s;

	for(f = t; f->type == t->type; f = f->next)
		;
	switch(f->type) {
	default:
		break;
	case TXXX:		/* Forward declaration */
	case TADT:
	case TUNION:
	case TAGGREGATE:
		s = f->tag;
		if(s == 0)
			s = f->sym;
		if(s == 0)
			break;

		Bprint(&ao, "\t'A' %s %4d %s;\n",
			amap(s->name), o+t->offset, amap(t->tag->name));
		return 1;
	}

	return 0;
}

/*
 * print a type record for acid
 */
void
member(int o, Type *t)
{
	Sym *s;

	while(t) {
		switch(t->type) {
		case TIND:
			if(icomplex(o, t)) {
				t = t->member;
				continue;
			}
			/* No break */
		case TINT:
		case TUINT:
		case TSINT:
		case TSUINT:
		case TCHAR:
		case TFLOAT:
		case TCHANNEL:
		case TARRAY:
			if(t->tag == 0)
				break;

			Bprint(&ao, "\t'%c' %4d %s;\n",
			lcode[t->type], o+t->offset, amap(t->tag->name));
			break;	
		case TAGGREGATE:
		case TUNION:
		case TADT:
			/* Flatten unnamed structures */
			if(t->tag == 0) {
				Bprint(&ao, "\t{\n");
				member(o+t->offset, t->next);
				Bprint(&ao, "\t};\n");
				break;
			}
			s = ltytosym(t);
			if(s == 0)
				break;
			Bprint(&ao, "\t%s %4d %s;\n",
			amap(s->name), o+t->offset, amap(t->tag->name));
			break;
		}
		t = t->member;
	}
}

void
pfun(Type *t)
{
	Sym *s;
	char *c, *m;

	c = amap(t->tag->name);
	Bprint(&ao, "defn\n%s(addr) {\n\tcomplex %s addr;\n", c, c);
	t = t->next;

	while(t) {
		if(t->tag == 0) {
			s = ltytosym(t);
			if(s != 0) {
				c = amap(s->name);
				Bprint(&ao, "\tprint(\"%s {\\n\");\n", c);
				Bprint(&ao, "\t\t%s(addr+%d);\n", c, t->offset);
				Bprint(&ao, "\tprint(\"}\\n\");\n");
			}
		}
		else
		switch(t->type) {
		case TIND:
			c = amap(t->tag->name);
			Bprint(&ao, "\tprint(\"\t%s\t\", addr.%s\\X, \"\\n\");\n", c, c);
			break;	
		case TINT:
		case TUINT:
		case TSINT:
		case TSUINT:
		case TCHAR:
		case TFLOAT:
		case TCHANNEL:
		case TARRAY:
			c = amap(t->tag->name);
			Bprint(&ao, "\tprint(\"\t%s\t\", addr.%s, \"\\n\");\n", c, c);
			break;	
		case TAGGREGATE:
		case TUNION:
		case TADT:
			c = amap(t->tag->name);
			s = ltytosym(t);
			if(s == 0) {
				break;
			}
			m = amap(s->name);
			Bprint(&ao, "\tprint(\"%s %s {\\n\");\n", m, c);
			Bprint(&ao, "\t%s(addr.%s);\n", m, c);
			Bprint(&ao, "\tprint(\"}\\n\");\n");
			break;
		}
		t = t->member;
	}
	Bprint(&ao, "};\n\n");
}

void
typerec(Node *n, Type *t)
{
	char *s, *u;

	switch(t->type) {
	default:
		return;
	case TADT:
		u = "adt";
		break;
	case TUNION:
		u = "union";
		break;
	case TAGGREGATE:
		u = "aggr";
		break;
	}
	s = amap(t->tag->name);
	Bprint(&ao, "// decl %L\n", n->srcline);
	Bprint(&ao, "sizeof%s = %d;\n", s, t->size);
	Bprint(&ao, "%s %s {\n", u, s);
	member(0, t->next);
	Bprint(&ao, "};\n\n");
	pfun(t);
}
