#include	"l.h"

void
listinit(void)
{

	fmtinstall('R', Rconv);
	fmtinstall('A', Aconv);
	fmtinstall('D', Dconv);
	fmtinstall('X', Xconv);
	fmtinstall('S', Sconv);
	fmtinstall('P', Pconv);
}

static	Prog	*bigP;

int
Pconv(void *o, Fconv *fp)
{
	char str[STRINGSZ], s[20];
	Prog *p;

	p = *(Prog**)o;
	bigP = p;
	sprint(str, "(%ld)	%A	%D,%D",
		p->line, p->as, &p->from, &p->to);
	if(p->from.field) {
		sprint(s, ",%d,%d", p->to.field, p->from.field);
		strcat(str, s);
	}
	strconv(str, fp);
	bigP = P;
	return sizeof(Prog*);
}

int
Aconv(void *o, Fconv *fp)
{

	strconv(anames[*(int*)o], fp);
	return sizeof(int);
}

int
Xconv(void *o, Fconv *fp)
{
	char str[20], s[10];
	int i;

	str[0] = 0;
	i = ((int*)o)[0] & D_MASK;
	if(i != D_NONE) {
		sprint(str, "(%R.", i);
		i = ((int*)o)[1];
		sprint(s, "%c*%c)",
			"WWWWLLLL"[i],
			"12481248"[i]);
		strcat(str, s);
	}
	strconv(str, fp);
	return sizeof(int[2]);
}

int
Dconv(void *o, Fconv *fp)
{
	char str[40], s[20];
	Adr *a;
	int i, j;
	long d;

	a = *(Adr**)o;
	i = a->index;
	if(i != D_NONE) {
		a->index = D_NONE;
		d = a->displace;
		a->displace = 0;
		switch(i & I_MASK) {
		default:
			sprint(str, "???%ld(%D)%X", d, a, i, a->scale);
			break;

		case I_INDEX1:
			sprint(str, "%D%X", a, i, a->scale);
			break;

		case I_INDEX2:
			if(d)
				sprint(str, "%ld(%D)%X", d, a, i, a->scale);
			else
				sprint(str, "(%D)%X", a, i, a->scale);
			break;

		case I_INDEX3:
			if(d)
				sprint(str, "%ld(%D%X)", d, a, i, a->scale);
			else
				sprint(str, "(%D%X)", a, i, a->scale);
			break;
		}
		a->displace = d;
		a->index = i;
		goto out;
	}
	i = a->type;
	j = i & I_MASK;
	if(j) {
		a->type = i & D_MASK;
		d = a->offset;
		a->offset = 0;
		switch(j) {
		case I_INDINC:
			sprint(str, "(%D)+", a);
			break;

		case I_INDDEC:
			sprint(str, "-(%D)", a);
			break;

		case I_INDIR:
			if(d)
				sprint(str, "%ld(%D)", d, a);
			else
				sprint(str, "(%D)", a);
			break;

		case I_ADDR:
			a->offset = d;
			sprint(str, "$%D", a);
			break;
		}
		a->type = i;
		a->offset = d;
		goto out;
	}
	switch(i) {

	default:
		sprint(str, "%R", i);
		break;

	case D_NONE:
		str[0] = 0;
		break;

	case D_BRANCH:
		if(bigP != P && bigP->cond != P)
			if(a->sym != S)
				sprint(str, "%lux+%s", bigP->cond->pc,
					a->sym->name);
			else
				sprint(str, "%lux", bigP->cond->pc);
		else
			sprint(str, "%ld(PC)", a->offset);
		break;

	case D_EXTERN:
		sprint(str, "%s+%ld(SB)", a->sym->name, a->offset);
		break;

	case D_STATIC:
		sprint(str, "%s<%d>+%ld(SB)", a->sym->name,
			a->sym->version, a->offset);
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

	case D_STACK:
		sprint(str, "TOS+%ld", a->offset);
		break;

	case D_QUICK:
		sprint(str, "$Q%ld", a->offset);
		break;

	case D_FCONST:
		sprint(str, "$(%.8lux,%.8lux)", a->ieee.h, a->ieee.l);
		goto out;

	case D_SCONST:
		sprint(str, "$\"%S\"", a->scon);
		goto out;
	}
	if(a->displace) {
		sprint(s, "/%ld", a->displace);
		strcat(str, s);
	}
out:
	strconv(str, fp);
	return sizeof(Adr*);
}

int
Rconv(void *o, Fconv *fp)
{
	char str[20];
	int r;

	r = *(int*)o;
	if(r >= D_R0 && r < D_R0+NREG)
		sprint(str, "R%d", r-D_R0);
	else
	if(r >= D_A0 && r < D_A0+NREG)
		sprint(str, "A%d", r-D_A0);
	else
	if(r >= D_F0 && r < D_F0+NREG)
		sprint(str, "F%d", r-D_F0);
	else
	switch(r) {

	default:
		sprint(str, "gok(%d)", r);
		break;

	case D_NONE:
		sprint(str, "NONE");
		break;

	case D_TOS:
		sprint(str, "TOS");
		break;

	case D_CCR:
		sprint(str, "CCR");
		break;

	case D_SR:
		sprint(str, "SR");
		break;

	case D_SFC:
		sprint(str, "SFC");
		break;

	case D_DFC:
		sprint(str, "DFC");
		break;

	case D_CACR:
		sprint(str, "CACR");
		break;

	case D_USP:
		sprint(str, "USP");
		break;

	case D_VBR:
		sprint(str, "VBR");
		break;

	case D_CAAR:
		sprint(str, "CAAR");
		break;

	case D_MSP:
		sprint(str, "MSP");
		break;

	case D_ISP:
		sprint(str, "ISP");
		break;

	case D_FPCR:
		sprint(str, "FPCR");
		break;

	case D_FPSR:
		sprint(str, "FPSR");
		break;

	case D_FPIAR:
		sprint(str, "FPIAR");
		break;

	case D_TREE:
		sprint(str, "TREE");
		break;

	case D_TC:
		sprint(str, "TC");
		break;

	case D_ITT0:
		sprint(str, "ITT0");
		break;

	case D_ITT1:
		sprint(str, "ITT1");
		break;

	case D_DTT0:
		sprint(str, "DTT0");
		break;

	case D_DTT1:
		sprint(str, "DTT1");
		break;

	case D_MMUSR:
		sprint(str, "MMUSR");
		break;
	case D_URP:
		sprint(str, "URP");
		break;

	case D_SRP:
		sprint(str, "SRP");
		break;
	}
	strconv(str, fp);
	return sizeof(int);
}

int
Sconv(void *o, Fconv *fp)
{
	int i, c;
	char str[30], *p, *a;

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
	strconv(str, fp);
	return sizeof(char*);
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
