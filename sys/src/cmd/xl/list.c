#include "l.h"

void
listinit(void)
{

	fmtinstall('A', Aconv);
	fmtinstall('D', Dconv);
	fmtinstall('P', Pconv);
	fmtinstall('S', Sconv);
	fmtinstall('N', Nconv);
	fmtinstall('J', Jconv);
	fmtinstall('R', Rconv);
	fmtinstall('X', Xconv);
}

void
prasm(Prog *p)
{
	print("%P\n", p);
}

int
Pconv(void *o, Fconv *fp)
{
	char str[STRINGSZ], *s;
	Prog *p;
	int a;

	p = *(Prog**)o;
	a = p->as;
	s = str;
	if(p->nosched)
		s += sprint(s, "-");
	if(a == ADATA)
		sprint(s, "	%A	%D/%d,%D", a, &p->from, p->reg, &p->to);
	else if(p->isf){
		s += sprint(s, "	%A	%D", a, &p->from);
		if(p->from1.type != D_NONE || p->from2.type != D_NONE)
			s += sprint(s, ",%D", &p->from1);
		if(p->from2.type != D_NONE)
			s += sprint(s, ",%D", &p->from2);
		if(p->reg != NREG)
			s += sprint(s, ",F%d", p->reg);
		if(p->to.type != D_NONE)
			sprint(s, ",%D", &p->to);
	}else if(p->reg == NREG && p->cc)
		sprint(s, "	%A	%J,%D,%D", a, p->cc, &p->from, &p->to);
	else if(p->reg == NREG)
		sprint(s, "	%A	%D,%D", a, &p->from, &p->to);
	else if(p->cc)
		sprint(s, "	%A	%J,%D,R%d,%D", a, p->cc, &p->from, p->reg, &p->to);
	else
		sprint(s, "	%A	%D,R%d,%D", a, &p->from, p->reg, &p->to);
	strconv(str, fp);
	return sizeof(p);
}

int
Aconv(void *o, Fconv *fp)
{
	char *s;
	int a;

	a = *(int*)o;
	s = "???";
	if(a >= AXXX && a <= AEND)
		s = anames[a];
	strconv(s, fp);
	return sizeof(a);
}

int
Dconv(void *o, Fconv *fp)
{
	char str[STRINGSZ];
	Adr *a;
	long v;

	a = *(Adr**)o;
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
		if(a->reg != NREG)
			sprint(str, "$%N(R%d)", a, a->reg);
		else
			sprint(str, "$%N", a);
		break;

	case D_INC:
		sprint(str, "(R%d)+", a->reg);
		break;

	case D_DEC:
		sprint(str, "(R%d)-", a->reg);
		break;

	case D_INCREG:
		sprint(str, "(R%d)+R%d", a->reg, a->increg);
		break;

	case D_NAME:
		sprint(str, "%N", a);
		break;

	case D_INDREG:
		if(a->reg != NREG)
			sprint(str, "(R%d)", a->reg);
		else
			sprint(str, "(R??)");
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

	case D_FREG:
		sprint(str, "F%d", a->reg);
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(F%d)(REG)", a, a->reg);
		break;

	case D_CREG:
		sprint(str, "C%d", a->reg);
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(C%d)(REG)", a, a->reg);
		break;

	case D_BRANCH:
		if(curp != P) {
			v = curp->pc;
			if(curp->cond)
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
		sprint(str, "$%e", dsptod(a->dsp));
		break;

	case D_AFCONST:
		sprint(str, "$$%e", dsptod(a->dsp));
		break;

	case D_SCONST:
		sprint(str, "$\"%S\"", a->sval);
		break;
	}
	strconv(str, fp);
	return sizeof(a);
}

int
Nconv(void *o, Fconv *fp)
{
	char str[STRINGSZ];
	Adr *a;
	Sym *s;

	a = *(Adr**)o;
	s = a->sym;
	if(s == S) {
		sprint(str, "%X", a->offset);
		goto out;
	}
	switch(a->name) {
	default:
		sprint(str, "GOK-name(%d)", a->name);
		break;

	case D_EXTERN:
		sprint(str, "%s+%X(SB)", s->name, a->offset);
		break;

	case D_STATIC:
		sprint(str, "%s<>+%X(SB)", s->name, a->offset);
		break;

	case D_AUTO:
		sprint(str, "%s+%X(SP)", s->name, a->offset);
		break;

	case D_PARAM:
		sprint(str, "%s+%X(FP)", s->name, a->offset);
		break;
	}
out:
	strconv(str, fp);
	return sizeof(a);
}

int
Xconv(void *o, Fconv *fp)
{
	char str[STRINGSZ];
	int off;

	off = *(int*)o;
	if(off > -10 && off < 10)
		sprint(str, "%x", off);
	else
		sprint(str, "0x%lux", off);
	strconv(str, fp);
	return sizeof(off);
}

int
Sconv(void *o, Fconv *fp)
{
	int i, c;
	char str[STRINGSZ], *p, *a;

	a = *(char**)o;
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
		}
		*p++ = (c>>6) + '0';
		*p++ = ((c>>3) & 7) + '0';
		*p++ = (c & 7) + '0';
	}
	*p = 0;
	strconv(str, fp);
	return sizeof(a);
}

int
Jconv(void *o, Fconv *fp)
{
	char *s;
	int cc;

	cc = *(int*)o;
	s = "CC";
	if(cc >= CCXXX && cc <= CCEND)
		s = ccnames[cc];
	strconv(s, fp);
	return sizeof(cc);
}

int
Rconv(void *o, Fconv *fp)
{
	char *s;
	int a;

	a = *(int*)o;
	s = "C_??";
	if(a >= C_NONE && a <= C_NCLASS)
		s = cnames[a];
	strconv(s, fp);
	return sizeof(a);
}

void
diag(char *a, ...)
{
	char buf[STRINGSZ], *tn;

	tn = "??none??";
	if(curtext != P && curtext->from.sym != S)
		tn = curtext->from.sym->name;
	doprint(buf, buf+sizeof(buf), a, &(&a)[1]);	/* ugly */
	print("%s: %s\n", tn, buf);

	nerrors++;
	if(nerrors > 10) {
		print("too many errors\n");
		errorexit();
	}
}
