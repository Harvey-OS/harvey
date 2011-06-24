#define EXTERN
#include "gc.h"

void
listinit(void)
{

	fmtinstall('A', Aconv);
	fmtinstall('B', Bconv);
	fmtinstall('P', Pconv);
	fmtinstall('S', Sconv);
	fmtinstall('D', Dconv);
	fmtinstall('R', Rconv);
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
			snprint(ss, sizeof(ss), "$%ld", var[i].offset);
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
		snprint(str, sizeof(str), "	%A	%D/%d,%D",
			p->as, &p->from, p->from.scale, &p->to);
	else if(p->as == ATEXT)
		snprint(str, sizeof(str), "	%A	%D,%d,%D",
			p->as, &p->from, p->from.scale, &p->to);
	else
		snprint(str, sizeof(str), "	%A	%D,%D",
			p->as, &p->from, &p->to);
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
Dconv(Fmt *fp)
{
	char str[40], s[20];
	Adr *a;
	int i;

	a = va_arg(fp->args, Adr*);
	i = a->type;
	if(i >= D_INDIR) {
		if(a->offset)
			snprint(str, sizeof(str), "%ld(%R)", a->offset, i-D_INDIR);
		else
			snprint(str, sizeof(str), "(%R)", i-D_INDIR);
		goto brk;
	}
	switch(i) {

	default:
		if(a->offset)
			snprint(str, sizeof(str), "$%ld,%R", a->offset, i);
		else
			snprint(str, sizeof(str), "%R", i);
		break;

	case D_NONE:
		str[0] = 0;
		break;

	case D_BRANCH:
		snprint(str, sizeof(str), "%ld(PC)", a->offset-pc);
		break;

	case D_EXTERN:
		snprint(str, sizeof(str), "%s+%ld(SB)", a->sym->name, a->offset);
		break;

	case D_STATIC:
		snprint(str, sizeof(str), "%s<>+%ld(SB)", a->sym->name,
			a->offset);
		break;

	case D_AUTO:
		snprint(str, sizeof(str), "%s+%ld(SP)", a->sym->name, a->offset);
		break;

	case D_PARAM:
		if(a->sym)
			snprint(str, sizeof(str), "%s+%ld(FP)", a->sym->name, a->offset);
		else
			snprint(str, sizeof(str), "%ld(FP)", a->offset);
		break;

	case D_CONST:
		snprint(str, sizeof(str), "$%ld", a->offset);
		break;

	case D_FCONST:
		snprint(str, sizeof(str), "$(%.17e)", a->dval);
		break;

	case D_SCONST:
		snprint(str, sizeof(str), "$\"%S\"", a->sval);
		break;

	case D_ADDR:
		a->type = a->index;
		a->index = D_NONE;
		snprint(str, sizeof(str), "$%D", a);
		a->index = a->type;
		a->type = D_ADDR;
		goto conv;
	}
brk:
	if(a->index != D_NONE) {
		fmtstrcpy(fp, str);
		snprint(s, sizeof(s), "(%R*%d)", (int)a->index, (int)a->scale);
		return fmtstrcpy(fp, s);
	}
conv:
	return fmtstrcpy(fp, str);
}

char*	regstr[] =
{
	"AL",	/*[D_AL]*/	
	"CL",
	"DL",
	"BL",
	"AH",
	"CH",
	"DH",
	"BH",

	"AX",	/*[D_AX]*/
	"CX",
	"DX",
	"BX",
	"SP",
	"BP",
	"SI",
	"DI",

	"F0",	/*[D_F0]*/
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",

	"CS",	/*[D_CS]*/
	"SS",
	"DS",
	"ES",
	"FS",
	"GS",

	"GDTR",	/*[D_GDTR]*/
	"IDTR",	/*[D_IDTR]*/
	"LDTR",	/*[D_LDTR]*/
	"MSW",	/*[D_MSW] */
	"TASK",	/*[D_TASK]*/

	"CR0",	/*[D_CR]*/
	"CR1",
	"CR2",
	"CR3",
	"CR4",
	"CR5",
	"CR6",
	"CR7",

	"DR0",	/*[D_DR]*/
	"DR1",
	"DR2",
	"DR3",
	"DR4",
	"DR5",
	"DR6",
	"DR7",

	"TR0",	/*[D_TR]*/
	"TR1",
	"TR2",
	"TR3",
	"TR4",
	"TR5",
	"TR6",
	"TR7",

	"NONE",	/*[D_NONE]*/
};

int
Rconv(Fmt *fp)
{
	char str[20];
	int r;

	r = va_arg(fp->args, int);
	if(r >= D_AL && r <= D_NONE)
		snprint(str, sizeof(str), "%s", regstr[r-D_AL]);
	else
		snprint(str, sizeof(str), "gok(%d)", r);

	return fmtstrcpy(fp, str);
}

int
Sconv(Fmt *fp)
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
