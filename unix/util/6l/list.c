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
	char str[STRINGSZ];
	Prog *p;

	p = va_arg(fp->args, Prog*);
	bigP = p;
	switch(p->as) {
	case ATEXT:
		if(p->from.scale) {
			sprint(str, "(%ld)	%A	%D,%d,%D",
				p->line, p->as, &p->from, p->from.scale, &p->to);
			break;
		}
	default:
		sprint(str, "(%ld)	%A	%D,%D",
			p->line, p->as, &p->from, &p->to);
		break;
	case ADATA:
	case AINIT:
	case ADYNT:
		sprint(str, "(%ld)	%A	%D/%d,%D",
			p->line, p->as, &p->from, p->from.scale, &p->to);
		break;
	}
	bigP = P;
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
			sprint(str, "%lld(%R)", a->offset, i-D_INDIR);
		else
			sprint(str, "(%R)", i-D_INDIR);
		goto brk;
	}
	switch(i) {

	default:
		if(a->offset)
			sprint(str, "$%lld,%R", a->offset, i);
		else
			sprint(str, "%R", i);
		break;

	case D_NONE:
		str[0] = 0;
		break;

	case D_BRANCH:
		if(bigP != P && bigP->pcond != P)
			if(a->sym != S)
				sprint(str, "%llux+%s", bigP->pcond->pc,
					a->sym->name);
			else
				sprint(str, "%llux", bigP->pcond->pc);
		else
			sprint(str, "%lld(PC)", a->offset);
		break;

	case D_EXTERN:
		sprint(str, "%s+%lld(SB)", a->sym->name, a->offset);
		break;

	case D_STATIC:
		sprint(str, "%s<%d>+%lld(SB)", a->sym->name,
			a->sym->version, a->offset);
		break;

	case D_AUTO:
		sprint(str, "%s+%lld(SP)", a->sym->name, a->offset);
		break;

	case D_PARAM:
		if(a->sym)
			sprint(str, "%s+%lld(%s)", a->sym->name, a->offset, paramspace);
		else
			sprint(str, "%lld(%s)", a->offset, paramspace);
		break;

	case D_CONST:
		sprint(str, "$%lld", a->offset);
		break;

	case D_FCONST:
		sprint(str, "$(%.8lux,%.8lux)", a->ieee.h, a->ieee.l);
		break;

	case D_SCONST:
		sprint(str, "$\"%S\"", a->scon);
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
		sprint(s, "(%R*%d)", a->index, a->scale);
		strcat(str, s);
	}
conv:
	return fmtstrcpy(fp, str);
}

char*	regstr[] =
{
	"AL",		/* [D_AL] */
	"CL",
	"DL",
	"BL",
	"SPB",
	"BPB",
	"SIB",
	"DIB",
	"R8B",
	"R9B",
	"R10B",
	"R11B",
	"R12B",
	"R13B",
	"R14B",
	"R15B",

	"AX",		/* [D_AX] */
	"CX",
	"DX",
	"BX",
	"SP",
	"BP",
	"SI",
	"DI",
	"R8",
	"R9",
	"R10",
	"R11",
	"R12",
	"R13",
	"R14",
	"R15",

	"AH",
	"CH",
	"DH",
	"BH",

	"F0",		/* [D_F0] */
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",

	"M0",
	"M1",
	"M2",
	"M3",
	"M4",
	"M5",
	"M6",
	"M7",

	"X0",
	"X1",
	"X2",
	"X3",
	"X4",
	"X5",
	"X6",
	"X7",
	"X8",
	"X9",
	"X10",
	"X11",
	"X12",
	"X13",
	"X14",
	"X15",

	"CS",		/* [D_CS] */
	"SS",
	"DS",
	"ES",
	"FS",
	"GS",

	"GDTR",		/* [D_GDTR] */
	"IDTR",		/* [D_IDTR] */
	"LDTR",		/* [D_LDTR] */
	"MSW",		/* [D_MSW] */
	"TASK",		/* [D_TASK] */

	"CR0",		/* [D_CR] */
	"CR1",
	"CR2",
	"CR3",
	"CR4",
	"CR5",
	"CR6",
	"CR7",
	"CR8",
	"CR9",
	"CR10",
	"CR11",
	"CR12",
	"CR13",
	"CR14",
	"CR15",

	"DR0",		/* [D_DR] */
	"DR1",
	"DR2",
	"DR3",
	"DR4",
	"DR5",
	"DR6",
	"DR7",

	"TR0",		/* [D_TR] */
	"TR1",
	"TR2",
	"TR3",
	"TR4",
	"TR5",
	"TR6",
	"TR7",

	"NONE",		/* [D_NONE] */
};

int
Rconv(Fmt *fp)
{
	char str[20];
	int r;

	r = va_arg(fp->args, int);
	if(r >= D_AL && r <= D_NONE)
		sprint(str, "%s", regstr[r-D_AL]);
	else
		sprint(str, "gok(%d)", r);

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
	if(nerrors > 20) {
		print("too many errors\n");
		errorexit();
	}
}
