#include "l.h"

void
listinit(void)
{

	fmtinstall('A', Aconv);
	fmtinstall('C', Cconv);
	fmtinstall('D', Dconv);
	fmtinstall('P', Pconv);
	fmtinstall('S', Sconv);
	fmtinstall('N', Nconv);
}

void
prasm(Prog *p)
{
	print("%P\n", p);
}

int
Pconv(va_list *arg, Fconv *fp)
{
	char str[STRINGSZ], *s;
	Prog *p;
	int a;

	p = va_arg(*arg, Prog*);
	curp = p;
	a = p->as;
	switch(a) {
	default:
		s = str;
		s += sprint(s, "(%ld)", p->line);
		if(p->reg == NREG)
			sprint(s, "	%A%C	%D,%D",
				a, p->scond, &p->from, &p->to);
		else
		if(p->from.type != D_FREG)
			sprint(s, "	%A%C	%D,R%d,%D",
				a, p->scond, &p->from, p->reg, &p->to);
		else
			sprint(s, "	%A%C	%D,F%d,%D",
				a, p->scond, &p->from, p->reg, &p->to);
		break;

	case ASWPW:
	case ASWPBU:
		sprint(str, "(%ld)	%A%C	R%d,%D,%D",
			p->line, a, p->scond, p->reg, &p->from, &p->to);
		break;

	case ADATA:
	case AINIT:
	case ADYNT:
		sprint(str, "(%ld)	%A%C	%D/%d,%D",
			p->line, a, p->scond, &p->from, p->reg, &p->to);
		break;
	}
	strconv(str, fp);
	return 0;
}

int
Aconv(va_list *arg, Fconv *fp)
{
	char *s;
	int a;

	a = va_arg(*arg, int);
	s = "???";
	if(a >= AXXX && a < ALAST)
		s = anames[a];
	strconv(s, fp);
	return 0;
}

char*	strcond[16] =
{
	".EQ",
	".NE",
	".HS",
	".LO",
	".MI",
	".PL",
	".VS",
	".VC",
	".HI",
	".LS",
	".GE",
	".LT",
	".GT",
	".LE",
	"",
	".NV"
};

int
Cconv(va_list *arg, Fconv *fp)
{
	char s[20];
	int c;

	c = va_arg(*arg, int);
	strcpy(s, strcond[c & C_SCOND]);
	if(c & C_SBIT)
		strcat(s, ".S");
	if(c & C_PBIT)
		strcat(s, ".P");
	if(c & C_WBIT)
		strcat(s, ".W");
	if(c & C_UBIT)		/* ambiguous with FBIT */
		strcat(s, ".U");
	strconv(s, fp);
	return 0;
}

int
Dconv(va_list *arg, Fconv *fp)
{
	char str[STRINGSZ];
	char *op;
	Adr *a;
	long v;

	a = va_arg(*arg, Adr*);
	switch(a->type) {

	default:
		sprint(str, "GOK-type(%d)", a->type);
		break;

	case D_NONE:
		str[0] = 0;
		if(a->name != D_NONE || a->reg != NREG || a->sym != S)
			sprint(str, "%N(R%d)(NONE)", a, a->reg);
		break;

	case D_CONST:
		if(a->reg == NREG)
			sprint(str, "$%N", a);
		else
			sprint(str, "$%N(R%d)", a, a->reg);
		break;

	case D_SHIFT:
		v = a->offset;
		op = "<<>>->@>" + (((v>>5) & 3) << 1);
		if(v & (1<<4))
			sprint(str, "R%ld%c%cR%ld", v&15, op[0], op[1], (v>>8)&15);
		else
			sprint(str, "R%ld%c%c%ld", v&15, op[0], op[1], (v>>7)&31);
		if(a->reg != NREG)
			sprint(str+strlen(str), "(R%d)", a->reg);
		break;

	case D_OCONST:
		sprint(str, "$*$%N", a);
		if(a->reg != NREG)
			sprint(str, "%N(R%d)(CONST)", a, a->reg);
		break;

	case D_OREG:
		if(a->reg != NREG)
			sprint(str, "%N(R%d)", a, a->reg);
		else
			sprint(str, "%N", a);
		break;

	case D_REG:
		sprint(str, "R%d", a->reg);
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(R%d)(REG)", a, a->reg);
		break;

	case D_REGREG:
		sprint(str, "(R%d,R%d)", a->reg, (int)a->offset);
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(R%d)(REG)", a, a->reg);
		break;

	case D_FREG:
		sprint(str, "F%d", a->reg);
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(R%d)(REG)", a, a->reg);
		break;

	case D_PSR:
		switch(a->reg) {
		case 0:
			sprint(str, "CPSR");
			break;
		case 1:
			sprint(str, "SPSR");
			break;
		default:
			sprint(str, "PSR%d", a->reg);
			break;
		}
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(PSR%d)(REG)", a, a->reg);
		break;

	case D_FPCR:
		switch(a->reg){
		case 0:
			sprint(str, "FPSR");
			break;
		case 1:
			sprint(str, "FPCR");
			break;
		default:
			sprint(str, "FCR%d", a->reg);
			break;
		}
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(FCR%d)(REG)", a, a->reg);

		break;

	case D_BRANCH:	/* botch */
		if(curp->cond != P) {
			v = curp->cond->pc;
			if(a->sym != S)
				sprint(str, "%s+%.5lux(BRANCH)", a->sym->name, v);
			else
				sprint(str, "%.5lux(BRANCH)", v);
		} else
			if(a->sym != S)
				sprint(str, "%s+%ld(APC)", a->sym->name, a->offset);
			else
				sprint(str, "%ld(APC)", a->offset);
		break;

	case D_FCONST:
		sprint(str, "$%e", ieeedtod(a->ieee));
		break;

	case D_SCONST:
		sprint(str, "$\"%S\"", a->sval);
		break;
	}
	strconv(str, fp);
	return 0;
}

int
Nconv(va_list *arg, Fconv *fp)
{
	char str[STRINGSZ];
	Adr *a;
	Sym *s;

	a = va_arg(*arg, Adr*);
	s = a->sym;
	switch(a->name) {
	default:
		sprint(str, "GOK-name(%d)", a->name);
		break;

	case D_NONE:
		sprint(str, "%ld", a->offset);
		break;

	case D_EXTERN:
		if(s == S)
			sprint(str, "%ld(SB)", a->offset);
		else
			sprint(str, "%s+%ld(SB)", s->name, a->offset);
		break;

	case D_STATIC:
		if(s == S)
			sprint(str, "<>+%ld(SB)", a->offset);
		else
			sprint(str, "%s<>+%ld(SB)", s->name, a->offset);
		break;

	case D_AUTO:
		if(s == S)
			sprint(str, "%ld(SP)", a->offset);
		else
			sprint(str, "%s-%ld(SP)", s->name, -a->offset);
		break;

	case D_PARAM:
		if(s == S)
			sprint(str, "%ld(FP)", a->offset);
		else
			sprint(str, "%s+%ld(FP)", s->name, a->offset);
		break;
	}
	strconv(str, fp);
	return 0;
}

int
Sconv(va_list *arg, Fconv *fp)
{
	int i, c;
	char str[STRINGSZ], *p, *a;

	a = va_arg(*arg, char*);
	p = str;
	for(i=0; i<sizeof(long); i++) {
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
		}
		*p++ = (c>>6) + '0';
		*p++ = ((c>>3) & 7) + '0';
		*p++ = (c & 7) + '0';
	}
	*p = 0;
	strconv(str, fp);
	return 0;
}

void
diag(char *fmt, ...)
{
	char buf[STRINGSZ], *tn;
	va_list arg;

	tn = "??none??";
	if(curtext != P && curtext->from.sym != S)
		tn = curtext->from.sym->name;
	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	print("%s: %s\n", tn, buf);

	nerrors++;
	if(nerrors > 10) {
		print("too many errors\n");
		errorexit();
	}
}
