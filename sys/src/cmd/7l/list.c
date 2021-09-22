#include "l.h"

void
listinit(void)
{

	fmtinstall('A', Aconv);
	fmtinstall('D', Dconv);
	fmtinstall('P', Pconv);
	fmtinstall('S', Sconv);
	fmtinstall('N', Nconv);
	fmtinstall('R', Rconv);
}

int
Pconv(Fmt *fp)
{
	char str[STRINGSZ], *s;
	Prog *p;
	int a;

	p = va_arg(fp->args, Prog*);
	curp = p;
	a = p->as;
	switch(a) {
	default:
		s = str;
		s += sprint(s, "(%ld)", p->line);
		if(p->reg == NREG && p->from3.type == D_NONE)
			sprint(s, "	%A	%D,%D",
				a, &p->from, &p->to);
		else if(p->from.type != D_FREG){
			s += sprint(s, "	%A	%D", a, &p->from);
			if(p->from3.type != D_NONE)
				s += sprint(s, ",%D", &p->from3);
			if(p->reg != NREG)
				s += sprint(s, ",R%d", p->reg);
			sprint(s, ",%D", &p->to);
		}else
			sprint(s, "	%A	%D,F%d,%D",
				a, &p->from, p->reg, &p->to);
		break;

	case ADATA:
	case AINIT:
	case ADYNT:
		sprint(str, "(%ld)	%A	%D/%d,%D",
			p->line, a, &p->from, p->reg, &p->to);
		break;
	}
	return fmtstrcpy(fp, str);
}

int
Aconv(Fmt *fp)
{
	char *s;
	int a;

	a = va_arg(fp->args, int);
	s = "???";
	if(a >= AXXX && a < ALAST && anames[a])
		s = anames[a];
	return fmtstrcpy(fp, s);
}

char*	strcond[16] =
{
	"EQ",
	"NE",
	"HS",
	"LO",
	"MI",
	"PL",
	"VS",
	"VC",
	"HI",
	"LS",
	"GE",
	"LT",
	"GT",
	"LE",
	"AL",
	"NV"
};

int
Dconv(Fmt *fp)
{
	char str[STRINGSZ];
	char *op;
	Adr *a;
	long v;
	static char *extop[] = {".UB", ".UH", ".UW", ".UX", ".SB", ".SH", ".SW", ".SX"};

	a = va_arg(fp->args, Adr*);
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
		if(a->reg == NREG || a->reg == REGZERO)
			sprint(str, "$%N", a);
		else
			sprint(str, "$%N(R%d)", a, a->reg);
		break;

	case D_SHIFT:
		v = a->offset;
		op = "<<>>->@>" + (((v>>22) & 3) << 1);
		sprint(str, "R%ld%c%c%ld", (v>>16)&0x1F, op[0], op[1], (v>>10)&0x3F);
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

	case D_XPRE:
		if(a->reg != NREG)
			sprint(str, "%N(R%d)!", a, a->reg);
		else
			sprint(str, "%N!", a);
		break;

	case D_XPOST:
		if(a->reg != NREG)
			sprint(str, "(R%d)%N!", a->reg, a);
		else
			sprint(str, "%N!", a);
		break;

	case D_EXTREG:
		v = a->offset;
		if(v & (7<<10))
			snprint(str, sizeof(str), "R%ld%s<<%ld", (v>>16)&31, extop[(v>>13)&7], (v>>10)&7);
		else
			snprint(str, sizeof(str), "R%ld%s", (v>>16)&31, extop[(v>>13)&7]);
		break;

	case D_ROFF:
		v = a->offset;
		if(v & (1<<16))
			snprint(str, sizeof(str), "(R%d)[R%ld%s]", a->reg, v&31, extop[(v>>8)&7]);
		else
			snprint(str, sizeof(str), "(R%d)(R%ld%s)", a->reg, v&31, extop[(v>>8)&7]);
		break;

	case D_REG:
		sprint(str, "R%d", a->reg);
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(R%d)(REG)", a, a->reg);
		break;

	case D_SP:
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(R%d)(REG)", a, a->reg);
		else
			strcpy(str, "SP");
		break;

	case D_COND:
		strcpy(str, strcond[a->reg & 0xF]);
		break;

	case D_FREG:
		sprint(str, "F%d", a->reg);
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(R%d)(REG)", a, a->reg);
		break;

	case D_SPR:
		switch((int)a->offset){		/* for 5c; ick */
		case D_FPSR:
			sprint(str, "FPSR");
			break;
		case D_FPCR:
			sprint(str, "FPCR");
			break;
		case D_NZCV:
			sprint(str, "NZCV");
			break;
		default:
			sprint(str, "SPR(%#llux)", a->offset);
			break;
		}
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(SPR%lld)(REG)", a, a->offset);
		break;

	case D_BRANCH:	/* botch */
		if(curp->cond != P) {
			v = curp->cond->pc;
			if(a->sym != S)
				sprint(str, "%s+%#.5lux(BRANCH)", a->sym->name, v);
			else
				sprint(str, "%.5lux(BRANCH)", v);
		} else
			if(a->sym != S)
				sprint(str, "%s+%lld(APC)", a->sym->name, a->offset);
			else
				sprint(str, "%lld(APC)", a->offset);
		break;

	case D_FCONST:
		sprint(str, "$%e", ieeedtod(a->ieee));
		break;

	case D_SCONST:
		sprint(str, "$\"%S\"", a->sval);
		break;
	}
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
	switch(a->name) {
	default:
		sprint(str, "GOK-name(%d)", a->name);
		break;

	case D_NONE:
		sprint(str, "%lld", a->offset);
		break;

	case D_EXTERN:
		if(s == S)
			sprint(str, "%lld(SB)", a->offset);
		else
			sprint(str, "%s+%lld(SB)", s->name, a->offset);
		break;

	case D_STATIC:
		if(s == S)
			sprint(str, "<>+%lld(SB)", a->offset);
		else
			sprint(str, "%s<>+%lld(SB)", s->name, a->offset);
		break;

	case D_AUTO:
		if(s == S)
			sprint(str, "%lld(SP)", a->offset);
		else
			sprint(str, "%s-%lld(SP)", s->name, -a->offset);
		break;

	case D_PARAM:
		if(s == S)
			sprint(str, "%lld(FP)", a->offset);
		else
			sprint(str, "%s+%lld(FP)", s->name, a->offset);
		break;
	}
	return fmtstrcpy(fp, str);
}

int
Rconv(Fmt *fp)
{
	char *s;
	int a;

	a = va_arg(fp->args, int);
	s = "C_??";
	if(a >= C_NONE && a <= C_NCLASS)
		s = cnames[a];
	return fmtstrcpy(fp, s);
}
