#include "cc.h"

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

Sym*
acidsue(Type *t)
{
	int h;
	Sym *s;

	if(t != T)
	for(h=0; h<nelem(hash); h++)
		for(s = hash[h]; s != S; s = s->link)
			if(s->suetag && s->suetag->link == t)
				return s;
	return 0;
}

Sym*
acidfun(Type *t)
{
	int h;
	Sym *s;

	for(h=0; h<nelem(hash); h++)
		for(s = hash[h]; s != S; s = s->link)
			if(s->type == t)
				return s;
	return 0;
}

char	acidchar[] =
{
	[TCHAR]		'C',
	[TUCHAR]	'b',
	[TSHORT]	'd',
	[TUSHORT]	'u',
	[TLONG]		'D',
	[TULONG]	'U',
	[TVLONG]	'V',
	[TUVLONG]	'W',
	[TFLOAT]	'f',
	[TDOUBLE]	'F',
	[TARRAY]	'a',
	[TIND]		'X',
};

void
acidmember(Type *t, long off, int flag)
{
	Sym *s, *s1;
	Type *l;

	s = t->sym;
	switch(t->etype) {
	default:
		Bprint(&outbuf, "	T%d\n", t->etype);
		break;

	case TIND:
		if(s == S)
			break;
		if(flag) {
			for(l=t; l->etype==TIND; l=l->link)
				;
			if(typesu[l->etype]) {
				s1 = acidsue(l->link);
				if(s1 != S) {
					Bprint(&outbuf, "	'A' %s %ld %s;\n",
						amap(s1->name),
						t->offset+off, amap(s->name));
					break;
				}
			}
		} else {
			Bprint(&outbuf, "\tprint(\"\t%s\t\", addr.%s\\X, \"\\n\");\n",
				amap(s->name), amap(s->name));
			break;
		}

	case TCHAR:
	case TUCHAR:
	case TSHORT:
	case TUSHORT:
	case TLONG:
	case TULONG:
	case TVLONG:
	case TUVLONG:
	case TFLOAT:
	case TDOUBLE:
	case TARRAY:
		if(s == S)
			break;
		if(flag) {
			Bprint(&outbuf, "	'%c' %ld %s;\n",
			acidchar[t->etype], t->offset+off, amap(s->name));
		} else {
			Bprint(&outbuf, "\tprint(\"\t%s\t\", addr.%s, \"\\n\");\n",
				amap(s->name), amap(s->name));
		}
		break;

	case TSTRUCT:
	case TUNION:
		s1 = acidsue(t->link);
		if(s1 == S)
			break;
		if(flag) {
			if(s == S) {
				Bprint(&outbuf, "	{\n");
				for(l = t->link; l != T; l = l->down)
					acidmember(l, t->offset+off, flag);
				Bprint(&outbuf, "	};\n");
			} else {
				Bprint(&outbuf, "	%s %ld %s;\n",
					amap(s1->name),
					t->offset+off, amap(s->name));
			}
		} else {
			if(s != S) {
				Bprint(&outbuf, "\tprint(\"%s %s {\\n\");\n",
					amap(s1->name), amap(s->name));
				Bprint(&outbuf, "\t%s(addr.%s);\n",
					amap(s1->name), amap(s->name));
				Bprint(&outbuf, "\tprint(\"}\\n\");\n");
			} else {
				Bprint(&outbuf, "\tprint(\"%s {\\n\");\n",
					amap(s1->name));
				Bprint(&outbuf, "\t\t%s(addr+%d);\n",
					amap(s1->name), t->offset+off);
				Bprint(&outbuf, "\tprint(\"}\\n\");\n");
			}
		}
		break;
	}
}

void
acidtype(Type *t)
{
	Sym *s;
	Type *l;
	Io *i;
	int n;
	char *an;

	if(!debug['a'])
		return;
	if(debug['a'] > 1) {
		n = 0;
		for(i=iostack; i; i=i->link)
			n++;
		if(n > 1)
			return;
	}
	s = acidsue(t->link);
	if(s == S)
		return;
	switch(t->etype) {
	default:
		Bprint(&outbuf, "T%d\n", t->etype);
		return;

	case TUNION:
	case TSTRUCT:
		if(debug['s'])
			goto asmstr;
		an = amap(s->name);
		Bprint(&outbuf, "sizeof%s = %d;\n", an, t->width);
		Bprint(&outbuf, "aggr %s\n{\n", an);
		for(l = t->link; l != T; l = l->down)
			acidmember(l, 0, 1);
		Bprint(&outbuf, "};\n\n");

		Bprint(&outbuf, "defn\n%s(addr) {\n\tcomplex %s addr;\n", an, an);
		for(l = t->link; l != T; l = l->down)
			acidmember(l, 0, 0);
		Bprint(&outbuf, "};\n\n");
		break;
	asmstr:
		if(s == S)
			break;
		for(l = t->link; l != T; l = l->down)
			if(l->sym != S)
				Bprint(&outbuf, "#define\t%s.%s\t%ld\n",
					s->name,
					l->sym->name,
					l->offset);
		break;
	}
}

void
acidvar(Sym *s)
{
	int n;
	Io *i;
	Type *t;
	Sym *s1, *s2;

	if(!debug['a'] || debug['s'])
		return;
	if(debug['a'] > 1) {
		n = 0;
		for(i=iostack; i; i=i->link)
			n++;
		if(n > 1)
			return;
	}
	t = s->type;
	while(t && t->etype == TIND)
		t = t->link;
	if(t == T)
		return;
	if(!typesu[t->etype])
		return;
	s1 = acidsue(t->link);
	if(s1 == S)
		return;
	switch(s->class) {
	case CAUTO:
	case CPARAM:
		s2 = acidfun(thisfn);
		if(s2)
			Bprint(&outbuf, "complex %s %s:%s;\n",
				amap(s1->name), amap(s2->name), amap(s->name));
		break;
	
	case CSTATIC:
	case CEXTERN:
	case CGLOBL:
	case CLOCAL:
		Bprint(&outbuf, "complex %s %s;\n",
			amap(s1->name), amap(s->name));
		break;
	}
}
