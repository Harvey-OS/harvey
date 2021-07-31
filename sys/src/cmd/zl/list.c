#include "l.h"

void
listinit(void)
{

	fmtinstall('A', Aconv);
	fmtinstall('D', Dconv);
	fmtinstall('S', Sconv);
	fmtinstall('W', Wconv);
	fmtinstall('P', Pconv);
}

int
Pconv(void *o, Fconv *fp)
{
	char str[STRINGSZ];
	Prog *p;

	p = *(Prog**)o;
	sprint(str, "\t%A\t%D,%D", p->as, &p->from, &p->to);
	strconv(str, fp);
	return sizeof(p);
}

int
Aconv(void *o, Fconv *fp)
{
	int a;
	char *s;

	a = *(int*)o;
	s = "???";
	if(a >= 0 && a <= AEND)
		s = anames[a];
	strconv(s, fp);
	return sizeof(a);
}

int
Dconv(void *o, Fconv *fp)
{
	char str[40];
	Adr *a;
	int i;

	a = *(Adr**)o;
	i = a->type;
	if(i & D_INDIR) {
		a->type &= ~D_INDIR;
		sprint(str, "*%D", a);
		a->type = i;
		goto out;
	}
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
		sprint(str, "%s+%ld(SB)%W", a->sym->name, a->offset, a->width);
		break;

	case D_STATIC:
		sprint(str, "%s<>+%ld(SB)%W", a->sym->name, a->offset, a->width);
		break;

	case D_AUTO:
		sprint(str, "%s-%ld(SP)%W", a->sym->name, -a->offset, a->width);
		break;

	case D_PARAM:
		sprint(str, "%s+%ld(FP)%W", a->sym->name, a->offset, a->width);
		break;

	case D_CONST:
		sprint(str, "$%ld%W", a->offset, a->width);
		break;

	case D_FCONST:
		sprint(str, "$%e%W", ieeedtod(&a->ieee), a->width);
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
		break;

	case D_REG:
		sprint(str, "R%ld%W", a->offset, a->width);
		break;

	case D_CPU:
		sprint(str, "%%%ld", a->offset);
		break;
	}
out:
	strconv(str, fp);
	return sizeof(a);
}

int
Wconv(void *o, Fconv *fp)
{
	char *s;
	int w;

	w = *(int*)o;
	s = "???";
	if(w >= 0 && w <= W_NONE)
		s = wnames[w];
	strconv(s, fp);
	return sizeof(w);
}

int
Sconv(void *ao, Fconv *fp)
{
	int i, c;
	char str[30], *p, **o;
	Adr *a;

	a = *(Adr**)ao;
	p = str;
	for(i=0; i<a->width; i++) {
		c = a->sval[i] & 0xff;
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
	strconv(str, fp);
	return sizeof(*o);
}

void
names(void)
{
	int i;
	Sym *s;

	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type == STEXT) {
			Bprint(&bso, "T %6lux %s", s->value, s->name);
			goto prv;
		}
		if(s->type == SDATA) {
			Bprint(&bso, "D %6lux %s", s->value+INITDAT, s->name);
			goto prv;
		}
		if(s->type == SBSS) {
			Bprint(&bso, "B %6lux %s", s->value+INITDAT, s->name);
			goto prv;
		}
		continue;

	prv:
		if(s->version)
			Bprint(&bso, "<%d>", s->version);
		Bprint(&bso, "\n");
	}
}

void
diag(char *a, ...)
{
	char buf[STRINGSZ], *tn;

	tn = "??none??";
	if(curtext != P && curtext->from.sym != S)
		tn = curtext->from.sym->name;
	doprint(buf, buf+sizeof(buf), a, &(&a)[1]);	/* ugly */
	Bprint(&bso, "%s: %s\n", tn, buf);

	nerrors++;
	if(nerrors > 10) {
		Bprint(&bso, "too many errors\n");
		errorexit();
	}
}
