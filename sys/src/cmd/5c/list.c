#define	EXTERN
#include "gc.h"

void
listinit(void)
{

	fmtinstall('A', Aconv);
	fmtinstall('P', Pconv);
	fmtinstall('S', Sconv);
	fmtinstall('N', Nconv);
	fmtinstall('B', Bconv);
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

char *extra [] = {
	".EQ", ".NE", ".CS", ".CC", 
	".MI", ".PL", ".VS", ".VC", 
	".HI", ".LS", ".GE", ".LT", 
	".GT", ".LE", "", ".NV",
};

int
Pconv(Fmt *fp)
{
	char str[STRINGSZ], sc[20];
	Prog *p;
	int a, s;

	p = va_arg(fp->args, Prog*);
	a = p->as;
	s = p->scond; 
	strcpy(sc, extra[s & C_SCOND]);
	if(s & C_SBIT)
		strcat(sc, ".S");
	if(s & C_PBIT)
		strcat(sc, ".P");
	if(s & C_WBIT)
		strcat(sc, ".W");
	if(s & C_UBIT)		/* ambiguous with FBIT */
		strcat(sc, ".U");
	if(a == AMOVM) {
		if(p->from.type == D_CONST)
			snprint(str, sizeof(str), "	%A%s	%R,%D", a, sc, &p->from, &p->to);
		else
		if(p->to.type == D_CONST)
			snprint(str, sizeof(str), "	%A%s	%D,%R", a, sc, &p->from, &p->to);
		else
			snprint(str, sizeof(str), "	%A%s	%D,%D", a, sc, &p->from, &p->to);
	} else
	if(a == ADATA)
		snprint(str, sizeof(str), "	%A	%D/%d,%D", a, &p->from, p->reg, &p->to);
	else
	if(p->as == ATEXT)
		snprint(str, sizeof(str), "	%A	%D,%d,%D", a, &p->from, p->reg, &p->to);
	else
	if(p->reg == NREG)
		snprint(str, sizeof(str), "	%A%s	%D,%D", a, sc, &p->from, &p->to);
	else
	if(p->from.type != D_FREG)
		snprint(str, sizeof(str), "	%A%s	%D,R%d,%D", a, sc, &p->from, p->reg, &p->to);
	else
		snprint(str, sizeof(str), "	%A%s	%D,F%d,%D", a, sc, &p->from, p->reg, &p->to);
	return fmtstrcpy(fp, str);
}

int
Aconv(Fmt *fp)
{
	char *s;
	int a;

	a = va_arg(fp->args, int);
	s = "???";
	if(a >= AXXX && a < ALAST)
		s = anames[a];
	return fmtstrcpy(fp, s);
}

int
Dconv(Fmt *fp)
{
	char str[STRINGSZ];
	Adr *a;
	char *op;
	int v;

	a = va_arg(fp->args, Adr*);
	switch(a->type) {

	default:
		snprint(str, sizeof(str), "GOK-type(%d)", a->type);
		break;

	case D_NONE:
		str[0] = 0;
		if(a->name != D_NONE || a->reg != NREG || a->sym != S)
			snprint(str, sizeof(str), "%N(R%d)(NONE)", a, a->reg);
		break;

	case D_CONST:
		if(a->reg != NREG)
			snprint(str, sizeof(str), "$%N(R%d)", a, a->reg);
		else
			snprint(str, sizeof(str), "$%N", a);
		break;

	case D_SHIFT:
		v = a->offset;
		op = "<<>>->@>" + (((v>>5) & 3) << 1);
		if(v & (1<<4))
			snprint(str, sizeof(str), "R%d%c%cR%d", v&15, op[0], op[1], (v>>8)&15);
		else
			snprint(str, sizeof(str), "R%d%c%c%d", v&15, op[0], op[1], (v>>7)&31);
		if(a->reg != NREG)
			sprint(str+strlen(str), "(R%d)", a->reg);
		break;

	case D_OREG:
		if(a->reg != NREG)
			snprint(str, sizeof(str), "%N(R%d)", a, a->reg);
		else
			snprint(str, sizeof(str), "%N", a);
		break;

	case D_REG:
		snprint(str, sizeof(str), "R%d", a->reg);
		if(a->name != D_NONE || a->sym != S)
			snprint(str, sizeof(str), "%N(R%d)(REG)", a, a->reg);
		break;

	case D_FREG:
		snprint(str, sizeof(str), "F%d", a->reg);
		if(a->name != D_NONE || a->sym != S)
			snprint(str, sizeof(str), "%N(R%d)(REG)", a, a->reg);
		break;

	case D_PSR:
		snprint(str, sizeof(str), "PSR");
		if(a->name != D_NONE || a->sym != S)
			snprint(str, sizeof(str), "%N(PSR)(REG)", a);
		break;

	case D_BRANCH:
		snprint(str, sizeof(str), "%ld(PC)", a->offset-pc);
		break;

	case D_FCONST:
		snprint(str, sizeof(str), "$%.17e", a->dval);
		break;

	case D_SCONST:
		snprint(str, sizeof(str), "$\"%S\"", a->sval);
		break;
	}
	return fmtstrcpy(fp, str);
}

int
Rconv(Fmt *fp)
{
	char str[STRINGSZ], *p, *e;
	Adr *a;
	int i, v;

	a = va_arg(fp->args, Adr*);
	snprint(str, sizeof(str), "GOK-reglist");
	switch(a->type) {
	case D_CONST:
		if(a->reg != NREG)
			break;
		if(a->sym != S)
			break;
		v = a->offset;
		p = str;
		e = str+sizeof(str);
		for(i=0; i<NREG; i++) {
			if(v & (1<<i)) {
				if(p == str)
					p = seprint(p, e, "[R%d", i);
				else
					p = seprint(p, e, ",R%d", i);
			}
		}
		seprint(p, e, "]");
	}
	return fmtstrcpy(fp, str);
}

int
Sconv(Fmt *fp)
{
	int i, c;
	char str[STRINGSZ], *p, *a;

	a = va_arg(fp->args, char*);
	p = str;
	for(i=0; i<NSNAME; i++) {
		c = a[i] & 0xff;
		if(c >= 'a' && c <= 'z' ||
		   c >= 'A' && c <= 'Z' ||
		   c >= '0' && c <= '9' ||
		   c == ' ' || c == '%') {
			*p++ = c;
			continue;
		}
		*p++ = '\\';
		switch(c) {
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
		case '\r':
			*p++ = 'r';
			continue;
		case '\f':
			*p++ = 'f';
			continue;
		}
		*p++ = (c>>6) + '0';
		*p++ = ((c>>3) & 7) + '0';
		*p++ = (c & 7) + '0';
	}
	*p = 0;
	return fmtstrcpy(fp, str);
}

int
Nconv(Fmt *fp)
{
	char str[STRINGSZ];
	Adr *a;
	Sym *s;

	a = va_arg(fp->args, Adr*);
	s = a->sym;
	if(s == S) {
		snprint(str, sizeof(str), "%ld", a->offset);
		goto out;
	}
	switch(a->name) {
	default:
		snprint(str, sizeof(str), "GOK-name(%d)", a->name);
		break;

	case D_NONE:
		snprint(str, sizeof(str), "%ld", a->offset);
		break;

	case D_EXTERN:
		snprint(str, sizeof(str), "%s+%ld(SB)", s->name, a->offset);
		break;

	case D_STATIC:
		snprint(str, sizeof(str), "%s<>+%ld(SB)", s->name, a->offset);
		break;

	case D_AUTO:
		snprint(str, sizeof(str), "%s-%ld(SP)", s->name, -a->offset);
		break;

	case D_PARAM:
		snprint(str, sizeof(str), "%s+%ld(FP)", s->name, a->offset);
		break;
	}
out:
	return fmtstrcpy(fp, str);
}
