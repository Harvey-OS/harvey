#include "gc.h"

void
listinit(void)
{

	fmtinstall('A', Aconv);
	fmtinstall('P', Pconv);
	fmtinstall('Z', Zconv);
	fmtinstall('X', Xconv);
	fmtinstall('D', Dconv);
	fmtinstall('R', Rconv);
	fmtinstall('B', Bconv);
}

int
Pconv(void *o, int f1, int f2, int f3, int chr)
{
	char str[STRINGSZ];
	Prog *p;

	USED(chr);
	p = *(Prog**)o;
	if(p->as == ADATA)
		sprint(str, "(%ld)	%A	%D/%d,%D",
			p->lineno, p->as, &p->from, p->from.scale, &p->to);
	else
	if(p->type != D_NONE) {
		if(p->type >= D_R0 && p->type < D_R0+32)
			sprint(str, "(%ld)	%A	%D,%R,%D",
				p->lineno, p->as, &p->from, p->type, &p->to);
		else
		if(p->type == D_CONST)
			sprint(str, "(%ld)	%A	%D,$%d,%D",
				p->lineno, p->as, &p->from, p->offset, &p->to);
		else
			sprint(str, "(%ld)	%A	%D,gok(%d),%D",
				p->lineno, p->as, &p->from, p->type, &p->to);
	} else
		sprint(str, "(%ld)	%A	%D,%D",
			p->lineno, p->as, &p->from, &p->to);
	strconv(str, f1, f2, f3);
	return sizeof(Prog*);
}

int
Aconv(void *o, int f1, int f2, int f3, int chr)
{

	USED(chr);
	strconv(anames[*(int*)o], f1, f2, f3);
	return sizeof(int);
}

int
Xconv(void *o, int f1, int f2, int f3, int chr)
{
	char str[20];
	int i;

	USED(chr);
	str[0] = 0;
	i = ((int*)o)[0];
	if(i != D_NONE)
		sprint(str, "(%R*%d)", i, ((int*)o)[1]);
	strconv(str, f1, f2, f3);
	return sizeof(int[2]);
}

int
Dconv(void *o, int f1, int f2, int f3, int chr)
{
	char str[40], s[20];
	Adr *a;
	int i;

	USED(chr);
	a = *(Adr**)o;
	i = a->type;
	if(i >= D_INDIR) {
		if(a->offset)
			sprint(str, "%ld(%R)", a->offset, i-D_INDIR);
		else
			sprint(str, "(%R)", i-D_INDIR);
		goto brk;
	}
	switch(i) {

	default:
		if(a->offset)
			sprint(str, "$%ld,%R", a->offset, i);
		else
			sprint(str, "%R", i);
		break;

	case D_NONE:
		str[0] = 0;
		break;

	case D_BRANCH:
		sprint(str, "%ld(PC)", a->offset-pc);
		break;

	case D_EXTERN:
		sprint(str, "%s+%ld(SB)", a->sym->name, a->offset);
		break;

	case D_STATIC:
		sprint(str, "%s<>+%ld(SB)", a->sym->name,
			a->offset);
		break;

	case D_AUTO:
		sprint(str, "%s+%ld(SP)", a->sym->name, a->offset);
		break;

	case D_PARAM:
		if(a->sym)
			sprint(str, "%s+%ld(FP)", a->sym->name, a->offset);
		else
			sprint(str, "%ld(FP)", a->offset);
		break;

	case D_CONST:
		sprint(str, "$%ld", a->offset);
		break;

	case D_FCONST:
		sprint(str, "$(%g)", a->dval);
		break;

	case D_SCONST:
		sprint(str, "$\"%Z\"", a->sval);
		break;

	case D_ADDR:
		a->type = a->index;
		a->index = D_NONE;
		sprint(str, "$%D", a);
		a->index = a->type;
		a->type = D_ADDR;
		goto conv;
	}
brk:
	if(a->index != D_NONE) {
		sprint(s, "%X", a->index, a->scale);
		strcat(str, s);
	}
conv:
	strconv(str, f1, f2, f3);
	return sizeof(Adr*);
}

int
Rconv(void *o, int f1, int f2, int f3, int chr)
{
	char str[20];
	int r;

	USED(chr);
	r = *(int*)o;
	if(r >= D_R0 && r <= D_R0+31)
		sprint(str, "R%d", r-D_R0);
	else
	if(r == D_NONE)
		sprint(str, "");
	else
		sprint(str, "gok(%d)", r);

	strconv(str, f1, f2, f3);
	return sizeof(int);
}

int
Zconv(void *o, int f1, int f2, int f3, int chr)
{
	int i, c;
	char str[30], *p, *a;

	USED(chr);
	a = *(char**)o;
	p = str;
	for(i=0; i<sizeof(double); i++) {
		c = a[i] & 0xff;
		if(c >= 'a' && c <= 'z' ||
		   c >= 'A' && c <= 'Z' ||
		   c >= '0' && c <= '9') {
			*p++ = c;
			continue;
		}
		*p++ = '\\';
		switch(c) {
		default:
			if(c < 040 || c >= 0177)
				break;	/* not portable */
			p[-1] = c;
			continue;
		case 0:
			*p++ = 'z';
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
	return sizeof(char*);
}
