#include <ctype.h>
#define	EXTERN
#include "a.h"
#include "y.tab.h"

void
main(int argc, char *argv[])
{
	char ofile[100], incfile[20], *p;
	int nout, nproc, status, i, c, of;

	thechar = '7';			/* of 9 */
	thestring = "alpha";
	memset(debug, 0, sizeof(debug));
	cinit();
	outfile = 0;
	include[ninclude++] = ".";
	ARGBEGIN {
	default:
		c = ARGC();
		if(c >= 0 || c < sizeof(debug))
			debug[c] = 1;
		break;

	case 'o':
		outfile = ARGF();
		break;

	case 'D':
		p = ARGF();
		if(p)
			Dlist[nDlist++] = p;
		break;

	case 'I':
		p = ARGF();
		if(p)
			include[ninclude++] = p;
		break;
	} ARGEND
	if(*argv == 0) {
		print("usage: %ca [-options] file.s\n", thechar);
		errorexit();
	}
	nproc = 3;
	if(p = getenv("NPROC"))
		nproc = atol(p);
	if(argc > 1) {
		c = 0;
		nout = 0;
		for(;;) {
			while(nout < nproc && argc > 0) {
				i = fork();
				if(i < 0) {
					i = mywait(&status);
					if(i < 0)
						errorexit();
					if(status)
						c++;
					nout--;
					continue;
				}
				if(i == 0) {
					print("%s:\n", *argv);
					goto child;
				}
				nout++;
				argc--;
				argv++;
			}
			i = mywait(&status);
			if(i < 0) {
				if(c)
					errorexit();
				exits(0);
			}
			if(status)
				c++;
			nout--;
		}
	}

child:
	strecpy(ofile, ofile+sizeof ofile, *argv);
	if(p = strrchr(ofile, '/')) {
		include[0] = ofile;
		*p++ = 0;
	} else
		p = ofile;
	if(outfile == 0) {
		outfile = p;
		if(p = strrchr(outfile, '.'))
			if(p[1] == 's' && p[2] == 0)
				p[0] = 0;
		p = strrchr(outfile, 0);
		p[0] = '.';
		p[1] = thechar;
		p[2] = 0;
	}
	if(0) {
		strcpy(incfile, "/usr/%include");
		p = strrchr(incfile, '%');
		if(p)
			*p = thechar;
	} else {
		strcpy(incfile, "/");
		strcat(incfile, thestring);
		strcat(incfile, "/include");
	}
	include[ninclude++] = incfile;
	if(p = getenv("INCLUDE"))
		include[ninclude-1] = p;	/* */
	of = mycreat(outfile, 0664);
	if(of < 0) {
		yyerror("%ca: cannot create %s", thechar, outfile);
		errorexit();
	}
	Binit(&obuf, of, OWRITE);

	pass = 1;
	pinit(*argv);
	for(i=0; i<nDlist; i++)
		dodefine(Dlist[i]);
	yyparse();
	if(nerrors) {
		cclean();
		errorexit();
	}

	pass = 2;
	outhist();
	pinit(*argv);
	for(i=0; i<nDlist; i++)
		dodefine(Dlist[i]);
	yyparse();
	cclean();
	if(nerrors)
		errorexit();
	exits(0);
}

struct
{
	char	*name;
	ushort	type;
	ushort	value;
} itab[] =
{
	"SP",		LSP,	D_AUTO,
	"SB",		LSB,	D_EXTERN,
	"FP",		LFP,	D_PARAM,
	"PC",		LPC,	D_BRANCH,

	"R",		LR,	0,
	"R0",		LREG,	0,
	"R1",		LREG,	1,
	"R2",		LREG,	2,
	"R3",		LREG,	3,
	"R4",		LREG,	4,
	"R5",		LREG,	5,
	"R6",		LREG,	6,
	"R7",		LREG,	7,
	"R8",		LREG,	8,
	"R9",		LREG,	9,
	"R10",		LREG,	10,
	"R11",		LREG,	11,
	"R12",		LREG,	12,
	"R13",		LREG,	13,
	"R14",		LREG,	14,
	"R15",		LREG,	15,
	"R16",		LREG,	16,
	"R17",		LREG,	17,
	"R18",		LREG,	18,
	"R19",		LREG,	19,
	"R20",		LREG,	20,
	"R21",		LREG,	21,
	"R22",		LREG,	22,
	"R23",		LREG,	23,
	"R24",		LREG,	24,
	"R25",		LREG,	25,
	"R26",		LREG,	26,
	"R27",		LREG,	27,
	"R28",		LREG,	28,
	"R29",		LREG,	29,
	"R30",		LREG,	30,
	"R31",		LREG,	31,

	"P",		LP,	0,
	"I",		LP,	32,
	"A",		LP,	64,
	"IA",		LP,	96,
	"T",		LP,	128,
	"IT",		LP,	32+128,
	"AT",		LP,	64+128,
	"IAT",		LP,	96+128,
	"T0",		LPREG,	0+128,
	"T1",		LPREG,	1+128,
	"T2",		LPREG,	2+128,
	"T3",		LPREG,	3+128,
	"T4",		LPREG,	4+128,
	"T5",		LPREG,	5+128,
	"T6",		LPREG,	6+128,
	"T7",		LPREG,	7+128,
	"T8",		LPREG,	8+128,
	"T9",		LPREG,	9+128,
	"T10",		LPREG,	10+128,
	"T11",		LPREG,	11+128,
	"T12",		LPREG,	12+128,
	"T13",		LPREG,	13+128,
	"T14",		LPREG,	14+128,
	"T15",		LPREG,	15+128,
	"T16",		LPREG,	16+128,
	"T17",		LPREG,	17+128,
	"T18",		LPREG,	18+128,
	"T19",		LPREG,	19+128,
	"T20",		LPREG,	20+128,
	"T21",		LPREG,	21+128,
	"T22",		LPREG,	22+128,
	"T23",		LPREG,	23+128,
	"T24",		LPREG,	24+128,
	"T25",		LPREG,	25+128,
	"T26",		LPREG,	26+128,
	"T27",		LPREG,	27+128,
	"T28",		LPREG,	28+128,
	"T29",		LPREG,	29+128,
	"T30",		LPREG,	30+128,
	"T31",		LPREG,	31+128,

	"F",		LF,	0,

	"F0",		LFREG,	0,
	"F1",		LFREG,	1,
	"F2",		LFREG,	2,
	"F3",		LFREG,	3,
	"F4",		LFREG,	4,
	"F5",		LFREG,	5,
	"F6",		LFREG,	6,
	"F7",		LFREG,	7,
	"F8",		LFREG,	8,
	"F9",		LFREG,	9,
	"F10",		LFREG,	10,
	"F11",		LFREG,	11,
	"F12",		LFREG,	12,
	"F13",		LFREG,	13,
	"F14",		LFREG,	14,
	"F15",		LFREG,	15,
	"F16",		LFREG,	16,
	"F17",		LFREG,	17,
	"F18",		LFREG,	18,
	"F19",		LFREG,	19,
	"F20",		LFREG,	20,
	"F21",		LFREG,	21,
	"F22",		LFREG,	22,
	"F23",		LFREG,	23,
	"F24",		LFREG,	24,
	"F25",		LFREG,	25,
	"F26",		LFREG,	26,
	"F27",		LFREG,	27,
	"F28",		LFREG,	28,
	"F29",		LFREG,	29,
	"F30",		LFREG,	30,
	"F31",		LFREG,	31,

	"FPCR",		LFPCR,	0,
	"PCC",		LPCC,	0,

	/* 1: integer operates */
	"ADDQ",		LTYPE1, AADDQ,
	"ADDL",		LTYPE1, AADDL,
	"SUBL",		LTYPE1, ASUBL,
	"SUBQ",		LTYPE1, ASUBQ,
	"CMPEQ",	LTYPE1, ACMPEQ,
	"CMPGT",	LTYPE1, ACMPGT,
	"CMPGE",	LTYPE1, ACMPGE,
	"CMPUGT",	LTYPE1, ACMPUGT,
	"CMPUGE",	LTYPE1, ACMPUGE,
	"CMPBLE",	LTYPE1, ACMPBLE,

	"AND",		LTYPE1, AAND,
	"ANDNOT",	LTYPE1, AANDNOT,
	"OR",		LTYPE1, AOR,
	"ORNOT",	LTYPE1, AORNOT,
	"XOR",		LTYPE1, AXOR,
	"XORNOT",	LTYPE1, AXORNOT,

	"CMOVEQ",	LTYPE1, ACMOVEQ,
	"CMOVNE",	LTYPE1, ACMOVNE,
	"CMOVLT",	LTYPE1, ACMOVLT,
	"CMOVGE",	LTYPE1, ACMOVGE,
	"CMOVLE",	LTYPE1, ACMOVLE,
	"CMOVGT",	LTYPE1, ACMOVGT,
	"CMOVLBS",	LTYPE1, ACMOVLBS,
	"CMOVLBC",	LTYPE1, ACMOVLBC,

	"MULL",		LTYPE1, AMULL,
	"MULQ",		LTYPE1, AMULQ,
	"UMULH",	LTYPE1, AUMULH,
	"DIVQ",		LTYPE1, ADIVQ,
	"MODQ",		LTYPE1, AMODQ,
	"DIVQU",	LTYPE1, ADIVQU,
	"MODQU",	LTYPE1, AMODQU,
	"DIVL",		LTYPE1, ADIVL,
	"MODL",		LTYPE1, AMODL,
	"DIVLU",	LTYPE1, ADIVLU,
	"MODLU",	LTYPE1, AMODLU,

	"SLLQ",		LTYPE1, ASLLQ,
	"SRLQ",		LTYPE1, ASRLQ,
	"SRAQ",		LTYPE1, ASRAQ,

	"SLLL",		LTYPE1, ASLLL,
	"SRLL",		LTYPE1, ASRLL,
	"SRAL",		LTYPE1, ASRAL,

	"EXTBL",	LTYPE1, AEXTBL,
	"EXTWL",	LTYPE1, AEXTWL,
	"EXTLL",	LTYPE1, AEXTLL,
	"EXTQL",	LTYPE1, AEXTQL,
	"EXTWH",	LTYPE1, AEXTWH,
	"EXTLH",	LTYPE1, AEXTLH,
	"EXTQH",	LTYPE1, AEXTQH,

	"INSBL",	LTYPE1, AINSBL,
	"INSWL",	LTYPE1, AINSWL,
	"INSLL",	LTYPE1, AINSLL,
	"INSQL",	LTYPE1, AINSQL,
	"INSWH",	LTYPE1, AINSWH,
	"INSLH",	LTYPE1, AINSLH,
	"INSQH",	LTYPE1, AINSQH,

	"MSKBL",	LTYPE1, AMSKBL,
	"MSKWL",	LTYPE1, AMSKWL,
	"MSKLL",	LTYPE1, AMSKLL,
	"MSKQL",	LTYPE1, AMSKQL,
	"MSKWH",	LTYPE1, AMSKWH,
	"MSKLH",	LTYPE1, AMSKLH,
	"MSKQH",	LTYPE1, AMSKQH,

	"ZAP",		LTYPE1, AZAP,
	"ZAPNOT",	LTYPE1, AZAPNOT,

	/* 2: floating operates with 2 operands */
	"CVTQS",	LTYPE2, ACVTQS,
	"CVTQT",	LTYPE2, ACVTQT,
	"CVTTS",	LTYPE2, ACVTTS,
	"CVTTQ",	LTYPE2, ACVTTQ,
	"CVTLQ",	LTYPE2, ACVTLQ,
	"CVTQL",	LTYPE2, ACVTQL,

	/* 3: floating operates with 2 or 3 operands */
	"CPYS",		LTYPE3, ACPYS,
	"CPYSN",	LTYPE3, ACPYSN,
	"CPYSE",	LTYPE3, ACPYSE,
	"ADDS",		LTYPE3, AADDS,
	"ADDT",		LTYPE3, AADDT,
	"CMPTEQ",	LTYPE3, ACMPTEQ,
	"CMPTGT",	LTYPE3, ACMPTGT,
	"CMPTGE",	LTYPE3, ACMPTGE,
	"CMPTUN",	LTYPE3, ACMPTUN,
	"DIVS",		LTYPE3, ADIVS,
	"DIVT",		LTYPE3, ADIVT,
	"MULS",		LTYPE3, AMULS,
	"MULT",		LTYPE3, AMULT,
	"SUBS",		LTYPE3, ASUBS,
	"SUBT",		LTYPE3, ASUBT,
	"FCMOVEQ",	LTYPE3, AFCMOVEQ,
	"FCMOVNE",	LTYPE3, AFCMOVNE,
	"FCMOVLT",	LTYPE3, AFCMOVLT,
	"FCMOVGE",	LTYPE3, AFCMOVGE,
	"FCMOVLE",	LTYPE3, AFCMOVLE,
	"FCMOVGT",	LTYPE3, AFCMOVGT,

	/* 4: integer load/store and reg->reg (incl special regs) */
	"MOVQ",		LTYPE4, AMOVQ,

	/* 5: integer load/store and reg->reg */
	"MOVL",		LTYPE5, AMOVL,
	"MOVQU",	LTYPE5, AMOVQU,
	"MOVB",		LTYPE5, AMOVB,
	"MOVBU",	LTYPE5, AMOVBU,
	"MOVW",		LTYPE5, AMOVW,
	"MOVWU",	LTYPE5, AMOVWU,
	"MOVLP",	LTYPE5, AMOVLP,
	"MOVQP",	LTYPE5, AMOVQP,

	/* 6: integer load/store (only) */
	"MOVA", 	LTYPE6, AMOVA,
	"MOVAH",	LTYPE6, AMOVAH,
	"MOVLL",	LTYPE6, AMOVLL,
	"MOVQL",	LTYPE6, AMOVQL,
	"MOVLC",	LTYPEK, AMOVLC,
	"MOVQC",	LTYPEK, AMOVQC,

	/* 7: floating load/store and reg->reg */
	"MOVS",		LTYPE7, AMOVS,
	"MOVT",		LTYPE7, AMOVT,

	/* 8,9: jumps */
	"JMP",		LTYPE8, AJMP,
	"JSR",		LTYPE8, AJSR,
	"RET",		LTYPE9, ARET,

	/* A: integer conditional branches */
	"BEQ",		LTYPEA, ABEQ,
	"BNE",		LTYPEA, ABNE,
	"BLT",		LTYPEA, ABLT,
	"BGE",		LTYPEA, ABGE,
	"BLE",		LTYPEA, ABLE,
	"BGT",		LTYPEA, ABGT,
	"BLBC",		LTYPEA, ABLBC,
	"BLBS",		LTYPEA, ABLBS,

	/* B: floating conditional branches */
	"FBEQ",		LTYPEB, AFBEQ,
	"FBNE",		LTYPEB, AFBNE,
	"FBLT",		LTYPEB, AFBLT,
	"FBGE",		LTYPEB, AFBGE,
	"FBLE",		LTYPEB, AFBLE,
	"FBGT",		LTYPEB, AFBGT,

	/* C-J: miscellaneous */
	"TRAPB",	LTYPEC, ATRAPB,
	"MB",		LTYPEC, AMB,
	"REI",		LTYPEC, AREI,
	"END",		LTYPEC, AEND,
	"FETCH",	LTYPED, AFETCH,
	"FETCHM",	LTYPED, AFETCHM,
	"CALL_PAL",	LTYPEE, ACALL_PAL,
	"TEXT",		LTYPEF, ATEXT,
	"GLOBL",	LTYPEF, AGLOBL,
	"DATA",		LTYPEG, ADATA,
	"WORD",		LTYPEH, AWORD,
	"NOP",		LTYPEI, ANOP,
	0
};

void
cinit(void)
{
	Sym *s;
	int i;

	nullgen.sym = S;
	nullgen.offset = 0;
	nullgen.type = D_NONE;
	nullgen.name = D_NONE;
	nullgen.reg = NREG;
	if(FPCHIP)
		nullgen.dval = 0;
	for(i=0; i<sizeof(nullgen.sval); i++)
		nullgen.sval[i] = 0;

	nerrors = 0;
	iostack = I;
	iofree = I;
	peekc = IGN;
	nhunk = 0;
	for(i=0; i<NHASH; i++)
		hash[i] = S;
	for(i=0; itab[i].name; i++) {
		s = slookup(itab[i].name);
		s->type = itab[i].type;
		s->value = itab[i].value;
	}

	ALLOCN(pathname, 0, 100);
	if(getwd(pathname, 99) == 0) {
		ALLOCN(pathname, 100, 900);
		if(getwd(pathname, 999) == 0)
			strcpy(pathname, "/???");
	}
}

void
syminit(Sym *s)
{

	s->type = LNAME;
	s->value = 0;
}

int
isreg(Gen *g)
{

	USED(g);
	return 1;
}

void
cclean(void)
{

	outcode(AEND, &nullgen, NREG, &nullgen);
	Bflush(&obuf);
}

void
zname(char *n, int t, int s)
{

	Bputc(&obuf, ANAME);
	Bputc(&obuf, t);	/* type */
	Bputc(&obuf, s);	/* sym */
	while(*n) {
		Bputc(&obuf, *n);
		n++;
	}
	Bputc(&obuf, 0);
}

void
zaddr(Gen *a, int s)
{
	vlong l;
	int i;
	char *n;
	Ieee e;

	Bputc(&obuf, a->type);
	Bputc(&obuf, a->reg);
	Bputc(&obuf, s);
	Bputc(&obuf, a->name);
	switch(a->type) {
	default:
		print("unknown type %d\n", a->type);
		exits("arg");

	case D_NONE:
	case D_REG:
	case D_FREG:
	case D_PREG:
	case D_FCREG:
	case D_PCC:
		break;

	case D_OREG:
	case D_CONST:
	case D_BRANCH:
		l = a->offset;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
		Bputc(&obuf, l>>32);
		Bputc(&obuf, l>>40);
		Bputc(&obuf, l>>48);
		Bputc(&obuf, l>>56);
		break;

	case D_SCONST:
		n = a->sval;
		for(i=0; i<NSNAME; i++) {
			Bputc(&obuf, *n);
			n++;
		}
		break;

	case D_FCONST:
		ieeedtod(&e, a->dval);
		Bputc(&obuf, e.l);
		Bputc(&obuf, e.l>>8);
		Bputc(&obuf, e.l>>16);
		Bputc(&obuf, e.l>>24);
		Bputc(&obuf, e.h);
		Bputc(&obuf, e.h>>8);
		Bputc(&obuf, e.h>>16);
		Bputc(&obuf, e.h>>24);
		break;
	}
}

void
outcode(int a, Gen *g1, int reg, Gen *g2)
{
	int sf, st, t;
	Sym *s;

	if(pass == 1)
		goto out;
jackpot:
	sf = 0;
	s = g1->sym;
	while(s != S) {
		sf = s->sym;
		if(sf < 0 || sf >= NSYM)
			sf = 0;
		t = g1->name;
		if(h[sf].type == t)
		if(h[sf].sym == s)
			break;
		zname(s->name, t, sym);
		s->sym = sym;
		h[sym].sym = s;
		h[sym].type = t;
		sf = sym;
		sym++;
		if(sym >= NSYM)
			sym = 1;
		break;
	}
	st = 0;
	s = g2->sym;
	while(s != S) {
		st = s->sym;
		if(st < 0 || st >= NSYM)
			st = 0;
		t = g2->name;
		if(h[st].type == t)
		if(h[st].sym == s)
			break;
		zname(s->name, t, sym);
		s->sym = sym;
		h[sym].sym = s;
		h[sym].type = t;
		st = sym;
		sym++;
		if(sym >= NSYM)
			sym = 1;
		if(st == sf)
			goto jackpot;
		break;
	}
	Bputc(&obuf, a);
	Bputc(&obuf, reg);
	Bputc(&obuf, lineno);
	Bputc(&obuf, lineno>>8);
	Bputc(&obuf, lineno>>16);
	Bputc(&obuf, lineno>>24);
	zaddr(g1, sf);
	zaddr(g2, st);

out:
	if(a != AGLOBL && a != ADATA)
		pc++;
}

void
outhist(void)
{
	Gen g;
	Hist *h;
	char *p, *q, *op;
	int n;

	g = nullgen;
	for(h = hist; h != H; h = h->link) {
		p = h->name;
		op = 0;
		if(p && p[0] != '/' && h->offset == 0 && pathname && pathname[0] == '/') {
			op = p;
			p = pathname;
		}
		while(p) {
			q = strchr(p, '/');
			if(q) {
				n = q-p;
				if(n == 0)
					n = 1;	/* leading "/" */
				q++;
			} else {
				n = strlen(p);
				q = 0;
			}
			if(n) {
				Bputc(&obuf, ANAME);
				Bputc(&obuf, D_FILE);	/* type */
				Bputc(&obuf, 1);	/* sym */
				Bputc(&obuf, '<');
				Bwrite(&obuf, p, n);
				Bputc(&obuf, 0);
			}
			p = q;
			if(p == 0 && op) {
				p = op;
				op = 0;
			}
		}
		g.offset = h->offset;

		Bputc(&obuf, AHISTORY);
		Bputc(&obuf, 0);
		Bputc(&obuf, h->line);
		Bputc(&obuf, h->line>>8);
		Bputc(&obuf, h->line>>16);
		Bputc(&obuf, h->line>>24);
		zaddr(&nullgen, 0);
		zaddr(&g, 0);
	}
}

#include "../cc/lexbody"
#include "../cc/macbody"
#include "../cc/compat"
