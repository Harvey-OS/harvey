#include "gc.h"

void
listinit(void)
{

	fmtinstall('A', Aconv);
	fmtinstall('P', Pconv);
	fmtinstall('S', Sconv);
	fmtinstall('D', Dconv);
	fmtinstall('W', Wconv);
}

int
Pconv(void *o, int f1, int f2, int f3, int chr)
{
	char str[STRINGSZ];
	Prog *p;

	USED(chr);
	p = *(Prog**)o;
	sprint(str, "\t%A\t%D,%D", p->as, &p->from, &p->to);
	strconv(str, f1, f2, f3);
	return sizeof(p);
}

int
Aconv(void *o, int f1, int f2, int f3, int chr)
{
	char *s;
	int a;

	USED(chr);
	a = *(int*)o;
	s = "???";
	if(a >= AXXX && a <= AEND)
		s = anames[a];
	strconv(s, f1, f2, f3);
	return sizeof(a);
}

int
Dconv(void *o, int f1, int f2, int f3, int chr)
{
	char str[STRINGSZ], *s;
	Adr *a;
	int i;

	USED(chr);
	a = *(Adr**)o;
	if(!a) {
		sprint(str, "????");
		goto out;
	}
	i = a->type;
	if(i & D_INDIR) {
		a->type &= ~D_INDIR;
		sprint(str, "*%D", a);
		a->type = i;
		goto out;
	}
	s = "???";
	switch(i) {

	default:
		sprint(str, "GOK(%d)", i);
		break;

	case D_NONE:
		str[0] = 0;
		break;

	case D_BRANCH:
		sprint(str, "%ld(PC)", a->offset-pc);
		break;

	case D_EXTERN:
		if(a->sym && a->sym->name)
			s = a->sym->name;
		sprint(str, "%s+%ld(SB)%W", s, a->offset, a->width);
		break;

	case D_STATIC:
		if(a->sym && a->sym->name)
			s = a->sym->name;
		sprint(str, "%s<>+%ld(SB)%W", s, a->offset, a->width);
		break;

	case D_AUTO:
		if(a->sym && a->sym->name)
			s = a->sym->name;
		sprint(str, "%s-%ld(SP)%W", s, -a->offset, a->width);
		break;

	case D_PARAM:
		if(a->sym && a->sym->name)
			s = a->sym->name;
		sprint(str, "%s+%ld(FP)%W", s, a->offset, a->width);
		break;

	case D_FCONST:
		if(a->width == W_NONE)
			sprint(str, "$%.8e", a->dval);
		else
			sprint(str, "$(%.8e)%W", a->dval, a->width);
		break;

	case D_CONST:
		if(a->width == W_NONE)
			sprint(str, "$%ld", a->offset);
		else
			sprint(str, "$(%ld)%W", a->offset, a->width);
		break;

	case D_ADDR:
		a->type = a->width + D_NONE;
		a->width = W_NONE;
		sprint(str, "$%D", a);
		a->width = a->type - D_NONE;
		a->type = D_ADDR;
		break;

	case D_SCONST:
		sprint(str, "$\"%S\"", a);
		goto out;

	case D_REG:
		sprint(str, "R%ld%W", a->offset, a->width);
		goto out;

	case D_CPU:
		sprint(str, "%%%ld", a->offset);
		goto out;
	}
out:
	strconv(str, f1, f2, f3);
	return sizeof(a);
}

int
Wconv(void *o, int f1, int f2, int f3, int chr)
{
	char *s;
	int w;

	USED(chr);
	w = *(int*)o;
	s = "???";
	if(w >= 0 && w <= W_NONE)
		s = wnames[w];
	strconv(s, f1, f2, f3);
	return sizeof(w);
}

int
Sconv(void *o, int f1, int f2, int f3, int chr)
{
	int i, c;
	char str[30], *p;
	Adr *a;

	USED(chr);
	a = *(Adr**)o;
	p = str;
	for(i=0; i<a->width; i++) {
		c = a->sval[i] & 0xff;
		if(c >= 'a' && c <= 'z' ||
		   c >= 'A' && c <= 'Z' ||
		   c >= '0' && c <= '9' ||
		   c == ' ' ||
		   c == '%') {
			*p++ = c;
			continue;
		}
		*p++ = '\\';
		switch(c) {
		case 0:
			*p++ = '0';
			continue;
		case '\\':
		case '"':
			*p++ = c;
			continue;
		case '\n':
			*p++ = 'n';
			continue;
		case '\t':
			*p++ = 't';
			continue;
		}
		*p++ = (c>>6) + '0';
		*p++ = ((c>>3) & 7) + '0';
		*p++ = (c & 7) + '0';
	}
	*p = 0;
	strconv(str, f1, f2, f3);
	return sizeof(a);
}
