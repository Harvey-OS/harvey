#define EXTERN
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
Bconv(Fmt *fp)
{
	char str[STRINGSZ], ss[STRINGSZ], *s;
	Bits bits;
	int i;

	str[0] = 0;
	bits = va_arg(fp->args, Bits);
	while(bany(&bits)) {
		i = bnum(bits);
		if(str[0])
			strcat(str, " ");
		if(var[i].sym == S) {
			sprint(ss, "$%ld", var[i].offset);
			s = ss;
		} else
			s = var[i].sym->name;
		if(strlen(str) + strlen(s) + 1 >= STRINGSZ)
			break;
		strcat(str, s);
		bits.b[i/32] &= ~(1L << (i%32));
	}
	return fmtstrcpy(fp, str);
}

int
Pconv(Fmt *fp)
{
	char str[STRINGSZ];
	Prog *p;

	p = va_arg(fp->args, Prog*);
	if(p->as == ADATA)
		sprint(str, "(%ld)	%A	%D/%d,%D",
			p->lineno, p->as, &p->from, p->from.scale, &p->to);
	else
	if(p->type != D_NONE) {
		if(p->type >= D_R0 && p->type < D_R0+32)
			sprint(str, "(%ld)	%A	%D,%R,%D",
				p->lineno, p->as, &p->from, (int)p->type, &p->to);
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
	return fmtstrcpy(fp, str);
}

int
Aconv(Fmt *fp)
{
	int i;

	i = va_arg(fp->args, int);
	return fmtstrcpy(fp, anames[i]);
}

int
Xconv(Fmt *fp)
{
	char str[20];
	Index x;

	str[0] = 0;
	x = va_arg(fp->args, Index);
	if(x.i0 != D_NONE)
		sprint(str, "(%R*%d)", x.i0, x.i1);
	return fmtstrcpy(fp, str);
}

int
Dconv(Fmt *fp)
{
	char str[40], s[20];
	Adr *a;
	int i;

	a = va_arg(fp->args, Adr*);
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
		sprint(str, "$(%.17e)", a->dval);
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
		sprint(s, "%X", (Index){a->index, a->scale});
		strcat(str, s);
	}
conv:
	return fmtstrcpy(fp, str);
}

int
Rconv(Fmt *fp)
{
	char str[20];
	int r;

	r = va_arg(fp->args, int);
	if(r >= D_R0 && r <= D_R0+31)
		sprint(str, "R%d", r-D_R0);
	else
	if(r == D_NONE)
		sprint(str, "");
	else
		sprint(str, "gok(%d)", r);

	return fmtstrcpy(fp, str);
}

int
Zconv(Fmt *fp)
{
	int i, c;
	char str[30], *p, *a;

	a = va_arg(fp->args, char*);
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
	return fmtstrcpy(fp, str);
}
