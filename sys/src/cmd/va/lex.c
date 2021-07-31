#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include "../vc/v.out.h"
#include "a.h"
#include "y.tab.h"

void
main(int argc, char *argv[])
{
	char ofile[100], incfile[20], *p;
	int nout, nproc, status, i, c, of;

	thechar = 'v';
	thestring = "mips";
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
			dodefine(p);
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
	strcpy(ofile, *argv);
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
	if(unix()) {
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
	yyparse();
	if(nerrors) {
		cclean();
		errorexit();
	}

	pass = 2;
	outhist();
	pinit(*argv);
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
	"HI",		LHI,	D_HI,
	"LO",		LLO,	D_LO,

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

	"M",		LM,	0,
	"M0",		LMREG,	0,
	"M1",		LMREG,	1,
	"M2",		LMREG,	2,
	"M3",		LMREG,	3,
	"M4",		LMREG,	4,
	"M5",		LMREG,	5,
	"M6",		LMREG,	6,
	"M7",		LMREG,	7,
	"M8",		LMREG,	8,
	"M9",		LMREG,	9,
	"M10",		LMREG,	10,
	"M11",		LMREG,	11,
	"M12",		LMREG,	12,
	"M13",		LMREG,	13,
	"M14",		LMREG,	14,
	"M15",		LMREG,	15,
	"M16",		LMREG,	16,
	"M17",		LMREG,	17,
	"M18",		LMREG,	18,
	"M19",		LMREG,	19,
	"M20",		LMREG,	20,
	"M21",		LMREG,	21,
	"M22",		LMREG,	22,
	"M23",		LMREG,	23,
	"M24",		LMREG,	24,
	"M25",		LMREG,	25,
	"M26",		LMREG,	26,
	"M27",		LMREG,	27,
	"M28",		LMREG,	28,
	"M29",		LMREG,	29,
	"M30",		LMREG,	30,
	"M31",		LMREG,	31,

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

	"FCR",		LFCR,	0,
	"FCR0",		LFCREG,	0,
	"FCR1",		LFCREG,	1,
	"FCR2",		LFCREG,	2,
	"FCR3",		LFCREG,	3,
	"FCR4",		LFCREG,	4,
	"FCR5",		LFCREG,	5,
	"FCR6",		LFCREG,	6,
	"FCR7",		LFCREG,	7,
	"FCR8",		LFCREG,	8,
	"FCR9",		LFCREG,	9,
	"FCR10",	LFCREG,	10,
	"FCR11",	LFCREG,	11,
	"FCR12",	LFCREG,	12,
	"FCR13",	LFCREG,	13,
	"FCR14",	LFCREG,	14,
	"FCR15",	LFCREG,	15,
	"FCR16",	LFCREG,	16,
	"FCR17",	LFCREG,	17,
	"FCR18",	LFCREG,	18,
	"FCR19",	LFCREG,	19,
	"FCR20",	LFCREG,	20,
	"FCR21",	LFCREG,	21,
	"FCR22",	LFCREG,	22,
	"FCR23",	LFCREG,	23,
	"FCR24",	LFCREG,	24,
	"FCR25",	LFCREG,	25,
	"FCR26",	LFCREG,	26,
	"FCR27",	LFCREG,	27,
	"FCR28",	LFCREG,	28,
	"FCR29",	LFCREG,	29,
	"FCR30",	LFCREG,	30,
	"FCR31",	LFCREG,	31,

	"ADD",		LTYPE1, AADD,
	"ADDU",		LTYPE1, AADDU,
	"SUB",		LTYPE1, ASUB,	/* converted to ADD(-) in loader */
	"SUBU",		LTYPE1, ASUBU,
	"SGT",		LTYPE1, ASGT,
	"SGTU",		LTYPE1, ASGTU,
	"AND",		LTYPE1, AAND,
	"OR",		LTYPE1, AOR,
	"XOR",		LTYPE1, AXOR,
	"SLL",		LTYPE1, ASLL,
	"SRL",		LTYPE1, ASRL,
	"SRA",		LTYPE1, ASRA,

	"NOR",		LTYPE2, ANOR,

	"MOVB",		LTYPE3, AMOVB,
	"MOVBU",	LTYPE3, AMOVBU,
	"MOVH",		LTYPE3, AMOVH,
	"MOVHU",	LTYPE3, AMOVHU,
	"MOVWL",	LTYPE3, AMOVWL,
	"MOVWR",	LTYPE3, AMOVWR,

	"BREAK",	LTYPEJ, ABREAK,		/* overloaded CACHE opcode */
	"END",		LTYPE4, AEND,
	"REM",		LTYPE6, AREM,
	"REMU",		LTYPE6, AREMU,
	"RET",		LTYPE4, ARET,
	"SYSCALL",	LTYPE4, ASYSCALL,
	"TLBP",		LTYPE4, ATLBP,
	"TLBR",		LTYPE4, ATLBR,
	"TLBWI",	LTYPE4, ATLBWI,
	"TLBWR",	LTYPE4, ATLBWR,

	"MOVW",		LTYPE5, AMOVW,
	"MOVD",		LTYPE5, AMOVD,
	"MOVF",		LTYPE5, AMOVF,

	"DIV",		LTYPE6, ADIV,
	"DIVU",		LTYPE6, ADIVU,
	"MUL",		LTYPE6, AMUL,
	"MULU",		LTYPE6, AMULU,

	"RFE",		LTYPE7, ARFE,
	"JMP",		LTYPE7, AJMP,

	"JAL",		LTYPE8, AJAL,

	"BEQ",		LTYPE9, ABEQ,
	"BNE",		LTYPE9, ABNE,

	"BGEZ",		LTYPEA, ABGEZ,
	"BGEZAL",	LTYPEA, ABGEZAL,
	"BGTZ",		LTYPEA, ABGTZ,
	"BLEZ",		LTYPEA, ABLEZ,
	"BLTZ",		LTYPEA, ABLTZ,
	"BLTZAL",	LTYPEA, ABLTZAL,

	"TEXT",		LTYPEB, ATEXT,
	"GLOBL",	LTYPEB, AGLOBL,

	"DATA",		LTYPEC, ADATA,

	"MOVDF",	LTYPE5, AMOVDF,
	"MOVDW",	LTYPE5, AMOVDW,
	"MOVFD",	LTYPE5, AMOVFD,
	"MOVFW",	LTYPE5, AMOVFW,
	"MOVWD",	LTYPE5, AMOVWD,
	"MOVWF",	LTYPE5, AMOVWF,

	"ABSD",		LTYPED, AABSD,
	"ABSF",		LTYPED, AABSF,
	"ABSW",		LTYPED, AABSW,
	"NEGD",		LTYPED, ANEGD,
	"NEGF",		LTYPED, ANEGF,
	"NEGW",		LTYPED, ANEGW,

	"CMPEQD",	LTYPEF, ACMPEQD,
	"CMPEQF",	LTYPEF, ACMPEQF,
	"CMPGED",	LTYPEF, ACMPGED,
	"CMPGEF",	LTYPEF, ACMPGEF,
	"CMPGTD",	LTYPEF, ACMPGTD,
	"CMPGTF",	LTYPEF, ACMPGTF,

	"ADDD",		LTYPEE, AADDD,
	"ADDF",		LTYPEE, AADDF,
	"ADDW",		LTYPEE, AADDW,
	"DIVD",		LTYPEE, ADIVD,
	"DIVF",		LTYPEE, ADIVF,
	"DIVW",		LTYPEE, ADIVW,
	"MULD",		LTYPEE, AMULD,
	"MULF",		LTYPEE, AMULF,
	"MULW",		LTYPEE, AMULW,
	"SUBD",		LTYPEE, ASUBD,
	"SUBF",		LTYPEE, ASUBF,
	"SUBW",		LTYPEE, ASUBW,

	"BFPT",		LTYPEG, ABFPT,
	"BFPF",		LTYPEG, ABFPF,

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
	long l;
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
	case D_MREG:
	case D_FCREG:
	case D_LO:
	case D_HI:
		break;

	case D_OREG:
	case D_CONST:
	case D_BRANCH:
		l = a->offset;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
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
	char name[NNAME], *p, *q;
	int n;

	g = nullgen;
	name[0] = '<';
	for(h = hist; h != H; h = h->link) {
		p = h->name;
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
			if(n >= NNAME-1)
				n = NNAME-2;
			if(n) {
				memmove(name+1, p, n);
				name[n+1] = 0;
				zname(name, D_FILE, 1);
			}
			p = q;
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
