#include <ctype.h>
#define	EXTERN
#include "a.h"
#include "y.tab.h"

void
main(int argc, char *argv[])
{
	char *p;
	int nout, nproc, status, i, c;

	thechar = 'k';
	thestring = "sparc";
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
		setinclude(p);
		break;
	} ARGEND
	if(*argv == 0) {
		print("usage: %ca [-options] file.s\n", thechar);
		errorexit();
	}
	if(argc > 1 && systemtype(Windows)){
		print("can't assemble multiple files on windows\n");
		errorexit();
	}
	if(argc > 1 && !systemtype(Windows)) {
		nproc = 1;
		if(p = getenv("NPROC"))
			nproc = atol(p);	/* */
		c = 0;
		nout = 0;
		for(;;) {
			while(nout < nproc && argc > 0) {
				i = myfork();
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
					if(assemble(*argv))
						errorexit();
					exits(0);
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
	if(assemble(argv[0]))
		errorexit();
	exits(0);
}

int
assemble(char *file)
{
	char ofile[100], incfile[20], *p;
	int i, of;

	strcpy(ofile, file);
	p = utfrrune(ofile, pathchar());
	if(p) {
		include[0] = ofile;
		*p++ = 0;
	} else
		p = ofile;
	if(outfile == 0) {
		outfile = p;
		if(outfile){
			p = utfrrune(outfile, '.');
			if(p)
				if(p[1] == 's' && p[2] == 0)
					p[0] = 0;
			p = utfrune(outfile, 0);
			p[0] = '.';
			p[1] = thechar;
			p[2] = 0;
		} else
			outfile = "/dev/null";
	}
	p = getenv("INCLUDE");
	if(p) {
		setinclude(p);
	} else {
		if(systemtype(Plan9)) {
			sprint(incfile,"/%s/include", thestring);
			setinclude(strdup(incfile));
		}
	}

	of = mycreat(outfile, 0664);
	if(of < 0) {
		yyerror("%ca: cannot create %s", thechar, outfile);
		errorexit();
	}
	Binit(&obuf, of, OWRITE);

	pass = 1;
	pinit(file);
	for(i=0; i<nDlist; i++)
		dodefine(Dlist[i]);
	yyparse();
	if(nerrors) {
		cclean();
		return nerrors;
	}

	pass = 2;
	outhist();
	pinit(file);
	for(i=0; i<nDlist; i++)
		dodefine(Dlist[i]);
	yyparse();
	cclean();
	return nerrors;
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

	"FSR",		LFSR,	D_FSR,
	"CSR",		LFSR,	D_CSR,

	"FQ",		LFPQ,	D_FPQ,
	"CQ",		LFPQ,	D_CPQ,

	"Y",		LPSR,	D_Y,
	"PSR",		LPSR,	D_PSR,
	"WIM",		LPSR,	D_WIM,
	"TBR",		LPSR,	D_TBR,

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

	"C",		LC,	0,
	"C0",		LCREG,	0,
	"C1",		LCREG,	1,
	"C2",		LCREG,	2,
	"C3",		LCREG,	3,
	"C4",		LCREG,	4,
	"C5",		LCREG,	5,
	"C6",		LCREG,	6,
	"C7",		LCREG,	7,
	"C8",		LCREG,	8,
	"C9",		LCREG,	9,
	"C10",		LCREG,	10,
	"C11",		LCREG,	11,
	"C12",		LCREG,	12,
	"C13",		LCREG,	13,
	"C14",		LCREG,	14,
	"C15",		LCREG,	15,
	"C16",		LCREG,	16,
	"C17",		LCREG,	17,
	"C18",		LCREG,	18,
	"C19",		LCREG,	19,
	"C20",		LCREG,	20,
	"C21",		LCREG,	21,
	"C22",		LCREG,	22,
	"C23",		LCREG,	23,
	"C24",		LCREG,	24,
	"C25",		LCREG,	25,
	"C26",		LCREG,	26,
	"C27",		LCREG,	27,
	"C28",		LCREG,	28,
	"C29",		LCREG,	29,
	"C30",		LCREG,	30,
	"C31",		LCREG,	31,

	"F",		LF,	0,
	"F0",		LFREG,	0,
	"F2",		LFREG,	2,
	"F4",		LFREG,	4,
	"F6",		LFREG,	6,
	"F8",		LFREG,	8,
	"F10",		LFREG,	10,
	"F12",		LFREG,	12,
	"F14",		LFREG,	14,
	"F16",		LFREG,	16,
	"F18",		LFREG,	18,
	"F20",		LFREG,	20,
	"F22",		LFREG,	22,
	"F24",		LFREG,	24,
	"F26",		LFREG,	26,
	"F28",		LFREG,	28,
	"F30",		LFREG,	30,
	"F1",		LFREG,	1,
	"F3",		LFREG,	3,
	"F5",		LFREG,	5,
	"F7",		LFREG,	7,
	"F9",		LFREG,	9,
	"F11",		LFREG,	11,
	"F13",		LFREG,	13,
	"F15",		LFREG,	15,
	"F17",		LFREG,	17,
	"F19",		LFREG,	19,
	"F21",		LFREG,	21,
	"F23",		LFREG,	23,
	"F25",		LFREG,	25,
	"F27",		LFREG,	27,
	"F29",		LFREG,	29,
	"F31",		LFREG,	31,

	"ADD",		LADDW, AADD,
	"ADDCC",	LADDW, AADDCC,
	"ADDX",		LADDW, AADDX,
	"ADDXCC",	LADDW, AADDXCC,
	"AND",		LADDW, AAND,
	"ANDCC",	LADDW, AANDCC,
	"ANDN",		LADDW, AANDN,
	"ANDNCC",	LADDW, AANDNCC,
	"BA",		LBRA, ABA,
	"BCC",		LBRA, ABCC,
	"BCS",		LBRA, ABCS,
	"BE",		LBRA, ABE,
	"BG",		LBRA, ABG,
	"BGE",		LBRA, ABGE,
	"BGU",		LBRA, ABGU,
	"BL",		LBRA, ABL,
	"BLE",		LBRA, ABLE,
	"BLEU",		LBRA, ABLEU,
	"BN",		LBRA, ABN,
	"BNE",		LBRA, ABNE,
	"BNEG",		LBRA, ABNEG,
	"BPOS",		LBRA, ABPOS,
	"BVC",		LBRA, ABVC,
	"BVS",		LBRA, ABVS,
	"CB0",		LBRA, ACB0,
	"CB01",		LBRA, ACB01,
	"CB012",	LBRA, ACB012,
	"CB013",	LBRA, ACB013,
	"CB02",		LBRA, ACB02,
	"CB023",	LBRA, ACB023,
	"CB03",		LBRA, ACB03,
	"CB1",		LBRA, ACB1,
	"CB12",		LBRA, ACB12,
	"CB123",	LBRA, ACB123,
	"CB13",		LBRA, ACB13,
	"CB2",		LBRA, ACB2,
	"CB23",		LBRA, ACB23,
	"CB3",		LBRA, ACB3,
	"CBA",		LBRA, ACBA,
	"CBN",		LBRA, ACBN,
	"CMP",		LCMP, ACMP,
	"CPOP1",	LCPOP, ACPOP1,
	"CPOP2",	LCPOP, ACPOP2,
	"DATA",		LDATA, ADATA,
	"DIV",		LADDW, ADIV,
	"DIVL",		LADDW, ADIVL,
	"END",		LEND, AEND,
	"FABSD",	LFCONV, AFABSD,
	"FABSF",	LFCONV, AFABSF,
	"FABSX",	LFCONV, AFABSX,
	"FADDD",	LFADD, AFADDD,
	"FADDF",	LFADD, AFADDF,
	"FADDX",	LFADD, AFADDX,
	"FBA",		LBRA, AFBA,
	"FBE",		LBRA, AFBE,
	"FBG",		LBRA, AFBG,
	"FBGE",		LBRA, AFBGE,
	"FBL",		LBRA, AFBL,
	"FBLE",		LBRA, AFBLE,
	"FBLG",		LBRA, AFBLG,
	"FBN",		LBRA, AFBN,
	"FBNE",		LBRA, AFBNE,
	"FBO",		LBRA, AFBO,
	"FBU",		LBRA, AFBU,
	"FBUE",		LBRA, AFBUE,
	"FBUG",		LBRA, AFBUG,
	"FBUGE",	LBRA, AFBUGE,
	"FBUL",		LBRA, AFBUL,
	"FBULE",	LBRA, AFBULE,
	"FCMPD",	LFADD, AFCMPD,
	"FCMPED",	LFADD, AFCMPED,
	"FCMPEF",	LFADD, AFCMPEF,
	"FCMPEX",	LFADD, AFCMPEX,
	"FCMPF",	LFADD, AFCMPF,
	"FCMPX",	LFADD, AFCMPX,
	"FDIVD",	LFADD, AFDIVD,
	"FDIVF",	LFADD, AFDIVF,
	"FDIVX",	LFADD, AFDIVX,
	"FMOVD",	LFMOV, AFMOVD,
	"FMOVDF",	LFCONV, AFMOVDF,
	"FMOVDW",	LFCONV, AFMOVDW,
	"FMOVDX",	LFCONV, AFMOVDX,
	"FMOVF",	LFMOV, AFMOVF,
	"FMOVFD",	LFCONV, AFMOVFD,
	"FMOVFW",	LFCONV, AFMOVFW,
	"FMOVFX",	LFCONV, AFMOVFX,
	"FMOVWD",	LFCONV, AFMOVWD,
	"FMOVWF",	LFCONV, AFMOVWF,
	"FMOVWX",	LFCONV, AFMOVWX,
	"FMOVX",	LFCONV, AFMOVX,
	"FMOVXD",	LFCONV, AFMOVXD,
	"FMOVXF",	LFCONV, AFMOVXF,
	"FMOVXW",	LFCONV, AFMOVXW,
	"FMULD",	LFADD, AFMULD,
	"FMULF",	LFADD, AFMULF,
	"FMULX",	LFADD, AFMULX,
	"FNEGD",	LFCONV, AFNEGD,
	"FNEGF",	LFCONV, AFNEGF,
	"FNEGX",	LFCONV, AFNEGX,
	"FSQRTD",	LFCONV, AFSQRTD,
	"FSQRTF",	LFCONV, AFSQRTF,
	"FSQRTX",	LFCONV, AFSQRTX,
	"FSUBD",	LFADD, AFSUBD,
	"FSUBF",	LFADD, AFSUBF,
	"FSUBX",	LFADD, AFSUBX,
	"GLOBL",	LTEXT, AGLOBL,
	"IFLUSH",	LFLUSH, AIFLUSH,
	"JMPL",		LJMPL, AJMPL,
	"JMP",		LJMPL, AJMP,
	"MOD",		LADDW, AMOD,
	"MODL",		LADDW, AMODL,
	"MOVB",		LMOVB, AMOVB,
	"MOVBU",	LMOVB, AMOVBU,
	"MOVD",		LMOVD, AMOVD,
	"MOVH",		LMOVB, AMOVH,
	"MOVHU",	LMOVB, AMOVHU,
	"MOVW",		LMOVW, AMOVW,
	"MUL",		LADDW, AMUL,
	"MULSCC",	LADDW, AMULSCC,
	"NOP",		LNOP, ANOP,
	"OR",		LADDW, AOR,
	"ORCC",		LADDW, AORCC,
	"ORN",		LADDW, AORN,
	"ORNCC",	LADDW, AORNCC,
	"RESTORE",	LADDW, ARESTORE,
	"RETT",		LRETT, ARETT,
	"RETURN",	LRETRN, ARETURN,
	"SAVE",		LADDW, ASAVE,
	"SLL",		LADDW, ASLL,
	"SRA",		LADDW, ASRA,
	"SRL",		LADDW, ASRL,
	"SUB",		LADDW, ASUB,
	"SUBCC",	LADDW, ASUBCC,
	"SUBX",		LADDW, ASUBX,
	"SUBXCC",	LADDW, ASUBXCC,
	"SWAP",		LSWAP, ASWAP,
	"TA",		LTRAP, ATA,
	"TADDCC",	LADDW, ATADDCC,
	"TADDCCTV",	LADDW, ATADDCCTV,
	"TAS",		LSWAP, ATAS,
	"TCC",		LTRAP, ATCC,
	"TCS",		LTRAP, ATCS,
	"TE",		LTRAP, ATE,
	"TEXT",		LTEXT, ATEXT,
	"TG",		LTRAP, ATG,
	"TGE",		LTRAP, ATGE,
	"TGU",		LTRAP, ATGU,
	"TL",		LTRAP, ATL,
	"TLE",		LTRAP, ATLE,
	"TLEU",		LTRAP, ATLEU,
	"TN",		LTRAP, ATN,
	"TNE",		LTRAP, ATNE,
	"TNEG",		LTRAP, ATNEG,
	"TPOS",		LTRAP, ATPOS,
	"TSUBCC",	LADDW, ATSUBCC,
	"TSUBCCTV",	LADDW, ATSUBCCTV,
	"TVC",		LTRAP, ATVC,
	"TVS",		LTRAP, ATVS,
	"UNIMP",	LUNIMP, AUNIMP,
	"WORD",		LUNIMP, AWORD,
	"XNOR",		LADDW, AXNOR,
	"XNORCC",	LADDW, AXNORCC,
	"XOR",		LXORW, AXOR,
	"XORCC",	LADDW, AXORCC,

	"SCHED",	LSCHED, 0,
	"NOSCHED",	LSCHED, 0x80,

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
	nullgen.xreg = NREG;
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

	pathname = allocn(pathname, 0, 100);
	if(mygetwd(pathname, 99) == 0) {
		pathname = allocn(pathname, 100, 900);
		if(mygetwd(pathname, 999) == 0)
			strcpy(pathname, "/???");
	}
}

void
syminit(Sym *s)
{

	s->type = LNAME;
	s->value = 0;
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
	case D_CREG:
	case D_PREG:
		break;

	case D_OREG:
	case D_ASI:
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
	if(g1->xreg != NREG) {
		if(reg != NREG || g2->xreg != NREG)
			yyerror("bad addressing modes");
		reg = g1->xreg;
	} else
	if(g2->xreg != NREG) {
		if(reg != NREG)
			yyerror("bad addressing modes");
		reg = g2->xreg;
	}
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
	Bputc(&obuf, reg|nosched);
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
	char *p, *q, *op, c;
	int n;

	g = nullgen;
	c = pathchar();
	for(h = hist; h != H; h = h->link) {
		p = h->name;
		op = 0;
		/* on windows skip drive specifier in pathname */
		if(systemtype(Windows) && p && p[1] == ':'){
			p += 2;
			c = *p;
		}
		if(p && p[0] != c && h->offset == 0 && pathname){
			/* on windows skip drive specifier in pathname */
			if(systemtype(Windows) && pathname[1] == ':') {
				op = p;
				p = pathname+2;
				c = *p;
			} else if(pathname[0] == c){
				op = p;
				p = pathname;
			}
		}
		while(p) {
			q = strchr(p, c);
			if(q) {
				n = q-p;
				if(n == 0){
					n = 1;	/* leading "/" */
					*p = '/';	/* don't emit "\" on windows */
				}
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
