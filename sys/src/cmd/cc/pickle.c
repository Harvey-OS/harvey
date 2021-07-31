#include "cc.h"

static char *kwd[] =
{
	"$adt", "$aggr", "$append", "$complex", "$defn",
	"$delete", "$do", "$else", "$eval", "$head", "$if",
	"$local", "$loop", "$return", "$tail", "$then",
	"$union", "$whatis", "$while",
};
static char picklestr[] = "\tpickle(s, un, ";

static char*
pmap(char *s)
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
picklesue(Type *t)
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
picklefun(Type *t)
{
	int h;
	Sym *s;

	for(h=0; h<nelem(hash); h++)
		for(s = hash[h]; s != S; s = s->link)
			if(s->type == t)
				return s;
	return 0;
}

char	picklechar[NTYPE];
Init	picklecinit[] =
{
	TCHAR,		'C',	0,
	TUCHAR,		'b',	0,
	TSHORT,		'd',	0,
	TUSHORT,		'u',	0,
	TLONG,		'D',	0,
	TULONG,		'U',	0,
	TVLONG,		'V',	0,
	TUVLONG,	'W',	0,
	TFLOAT,		'f',	0,
	TDOUBLE,		'F',	0,
	TARRAY,		'a',	0,
	TIND,		'X',	0,
	-1,		0,	0,
};

static void
pickleinit(void)
{
	Init *p;

	for(p=picklecinit; p->code >= 0; p++)
		picklechar[p->code] = p->value;

	picklechar[TINT] = picklechar[TLONG];
	picklechar[TUINT] = picklechar[TULONG];
	if(types[TINT]->width != types[TLONG]->width) {
		picklechar[TINT] = picklechar[TSHORT];
		picklechar[TUINT] = picklechar[TUSHORT];
		if(types[TINT]->width != types[TSHORT]->width)
			warn(Z, "picklemember int not long or short");
	}
	
}

void
picklemember(Type *t, long off)
{
	Sym *s, *s1;
	static int picklecharinit = 0;

	if(picklecharinit == 0) {
		pickleinit();
		picklecharinit = 1;
	}
	s = t->sym;
	switch(t->etype) {
	default:
		Bprint(&outbuf, "	T%d\n", t->etype);
		break;

	case TIND:
		if(s == S)
			Bprint(&outbuf,
				"%s\"p\", (char*)addr+%ld+_i*%ld);\n",
				picklestr, t->offset+off, t->width);
		else
			Bprint(&outbuf,
				"%s\"p\", &addr->%s);\n",
				picklestr, pmap(s->name));
		break;

	case TINT:
	case TUINT:
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
		if(s == S)
			Bprint(&outbuf, "%s\"%c\", (char*)addr+%ld+_i*%ld);\n",
				picklestr, picklechar[t->etype], t->offset+off, t->width);
		else
			Bprint(&outbuf, "%s\"%c\", &addr->%s);\n",
				picklestr, picklechar[t->etype], pmap(s->name));
		break;
	case TARRAY:
		Bprint(&outbuf, "\tfor(_i = 0; _i < %ld; _i++) {\n\t",
			t->width/t->link->width);
		picklemember(t->link, t->offset+off);
		Bprint(&outbuf, "\t}\n\t_i = 0;\n\tUSED(_i);\n");
		break;

	case TSTRUCT:
	case TUNION:
		s1 = picklesue(t->link);
		if(s1 == S)
			break;
		if(s == S) {
			Bprint(&outbuf, "\tpickle_%s(s, un, (%s*)((char*)addr+%ld+_i*%ld));\n",
				pmap(s1->name), pmap(s1->name), t->offset+off, t->width);
		} else {
			Bprint(&outbuf, "\tpickle_%s(s, un, &addr->%s);\n",
				pmap(s1->name), pmap(s->name));
		}
		break;
	}
}

void
pickletype(Type *t)
{
	Sym *s;
	Type *l;
	Io *i;
	int n;
	char *an;

	if(!debug['P'])
		return;
	if(debug['P'] > 1) {
		n = 0;
		for(i=iostack; i; i=i->link)
			n++;
		if(n > 1)
			return;
	}
	s = picklesue(t->link);
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
		an = pmap(s->name);

		Bprint(&outbuf, "void\npickle_%s(void *s, int un, %s *addr)\n{\n\tint _i = 0;\n\n\tUSED(_i);\n", an, an);
		for(l = t->link; l != T; l = l->down)
			picklemember(l, 0);
		Bprint(&outbuf, "}\n\n");
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
picklevar(Sym *s)
{
	int n;
	Io *i;
	Type *t;
	Sym *s1, *s2;

	if(!debug['P'] || debug['s'])
		return;
	if(debug['P'] > 1) {
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
	if(t->etype == TENUM) {
		Bprint(&outbuf, "%s = ", pmap(s->name));
		if(!typefd[t->etype])
			Bprint(&outbuf, "%lld;\n", s->vconst);
		else
			Bprint(&outbuf, "%f\n;", s->fconst);
		return;
	}
	if(!typesu[t->etype])
		return;
	s1 = picklesue(t->link);
	if(s1 == S)
		return;
	switch(s->class) {
	case CAUTO:
	case CPARAM:
		s2 = picklefun(thisfn);
		if(s2)
			Bprint(&outbuf, "complex %s %s:%s;\n",
				pmap(s1->name), pmap(s2->name), pmap(s->name));
		break;
	
	case CSTATIC:
	case CEXTERN:
	case CGLOBL:
	case CLOCAL:
		Bprint(&outbuf, "complex %s %s;\n",
			pmap(s1->name), pmap(s->name));
		break;
	}
}
