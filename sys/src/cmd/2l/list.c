#include	"l.h"

void
listinit(void)
{

	fmtinstall('R', Rconv);
	fmtinstall('A', Aconv);
	fmtinstall('D', Dconv);
	fmtinstall('S', Sconv);
	fmtinstall('P', Pconv);
}

static	Prog	*bigP;

int
Pconv(Fmt *fp)
{
	char str[STRINGSZ], s[20];
	Prog *p;

	p = va_arg(fp->args, Prog*);
	bigP = p;
	sprint(str, "(%ld)	%A	%D,%D",
		p->line, p->as, &p->from, &p->to);
	if(p->from.field) {
		sprint(s, ",%d,%d", p->to.field, p->from.field);
		strcat(str, s);
	}
	bigP = P;
	return fmtstrcpy(fp, str);
}

int
Aconv(Fmt *fp)
{

	return fmtstrcpy(fp, anames[va_arg(fp->args, int)]);
}

int
Xconv(Fmt *fp)
{
	char str[20], s[10];
	int i0, i1;

	str[0] = 0;
	i0 = va_arg(fp->args, int) & D_MASK;
	i1 = va_arg(fp->args, int);
	if(i0 != D_NONE) {
		sprint(str, "(%R.", i0);
		sprint(s, "%c*%c)",
			"WWWWLLLL"[i1],
			"12481248"[i1]);
		strcat(str, s);
	}
	return fmtstrcpy(fp, str);
}

int
Dconv(Fmt *fp)
{
	char str[40], s[20];
	Adr *a;
	int i, j;
	long d;

	a = va_arg(fp->args, Adr*);
	i = a->index;
	if(i != D_NONE) {
		a->index = D_NONE;
		d = a->displace;
		a->displace = 0;
		switch(i & I_MASK) {
		default:
			sprint(str, "???%ld(%D)", d, a);
			break;

		case I_INDEX1:
			sprint(str, "%D", a);
			break;

		case I_INDEX2:
			if(d)
				sprint(str, "%ld(%D)", d, a);
			else
				sprint(str, "(%D)", a);
			break;

		case I_INDEX3:
			if(d)
				sprint(str, "%ld(%D", d, a);
			else
				sprint(str, "(%D", a);
			break;
		}

		if(i != D_NONE) {
			j = a->scale & 7;
			sprint(strchr(str,0), "(%R.", i);
			sprint(strchr(str,0), "%c*%c)",
				"WWWWLLLL"[j],
				"12481248"[j]);
		}
		if((i & I_MASK) == I_INDEX3)
			strcat(str, ")");
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
		if(bigP != P && bigP->pcond != P)
			if(a->sym != S)
				sprint(str, "%lux+%s", bigP->pcond->pc,
					a->sym->name);
			else
				sprint(str, "%lux", bigP->pcond->pc);
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
	return fmtstrcpy(fp, str);
}

int
Rconv(Fmt *fp)
{
	char str[20];
	int r;

	r = va_arg(fp->args, int);
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

void
diag(char *fmt, ...)
{
	char buf[STRINGSZ], *tn;
	va_list arg;

	tn = "??none??";
	if(curtext != P && curtext->from.sym != S)
		tn = curtext->from.sym->name;
	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	print("%s: %s\n", tn, buf);

	nerrors++;
	if(nerrors > 10) {
		print("too many errors\n");
		errorexit();
	}
}
