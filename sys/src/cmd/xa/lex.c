#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include "../xc/x.out.h"
#include "a.h"
#include "y.tab.h"

void
main(int argc, char *argv[])
{
	char ofile[100], incfile[20], *p;
	int nout, nproc, status, i, c, of;

	thechar = 'x';
	thestring = "3210";
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
	nosched = 0;
	pinit(*argv);
	for(i=0; i<nDlist; i++)
		dodefine(Dlist[i]);
	yyparse();
	if(nerrors) {
		cclean();
		errorexit();
	}

	pass = 2;
	nosched = 0;
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

	"PCSH",		LPCSH,	D_BRANCH,	/* redo */

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

	"C",		LC,	0,
	"C0",		LCREG,	0,
	"C8",		LCREG,	8,
	"C10",		LCREG,	10,
	"C12",		LCREG,	12,
	"C14",		LCREG,	14,
	"C15",		LCREG,	15,

	"F",		LF,	0,
	"F0",		LFREG,	0,
	"F1",		LFREG,	1,
	"F2",		LFREG,	2,
	"F3",		LFREG,	3,

	"BMOVW",	LBMOVW, ABMOVW,
	"MOVW",		LMOVW, AMOVW,
	"MOVH",		LMOVH, AMOVH,
	"MOVB",		LMOVH, AMOVB,
	"MOVHU",	LMOVHU, AMOVHU,
	"MOVBU",	LMOVHU, AMOVBU,
	"MOVHB",	LMOVHU, AMOVHB,

	"ADD",		LADD, AADD,
	"ADDH",		LADD, AADDH,

	"ADDCR",	LOP, AADDCR,
	"ADDCRH",	LOP, AADDCRH,
	"AND",		LOP, AAND,
	"ANDH",		LOP, AANDH,
	"ANDN",		LOP, AANDN,
	"ANDNH",	LOP, AANDNH,
	"OR",		LOP, AOR,
	"ORH",		LOP, AORH,
	"XOR",		LOP, AXOR,
	"XORH",		LOP, AXORH,
	"MUL",		LOP, AMUL,
	"DIV",		LOP, ADIV,
	"DIVL",		LOP, ADIVL,
	"MOD",		LOP, AMOD,
	"MODL",		LOP, AMODL,
	"SUB",		LOP, ASUB,
	"SUBH",		LOP, ASUBH,
	"SUBR",		LOP, ASUBR,
	"SUBRH",	LOP, ASUBRH,

	"BIT",		LCMP, ABIT,
	"BITH",		LCMP, ABITH,
	"CMP",		LCMP, ACMP,
	"CMPH",		LCMP, ACMPH,

	"ROL",		LRO, AROL,
	"ROLH",		LRO, AROLH,
	"ROR",		LRO, AROR,
	"RORH",		LRO, ARORH,

	"SLL",		LSH, ASLL,
	"SLLH",		LSH, ASLLH,
	"SRA",		LSH, ASRA,
	"SRAH",		LSH, ASRAH,
	"SRL",		LSH, ASRL,
	"SRLH",		LSH, ASRLH,

	"FADD",		LFADD, AFADD,
	"FADDN",	LFADD, AFADDN,
	"FADDT",	LFADDT, AFADDT,
	"FADDTN",	LFADDT, AFADDTN,
	"FSUB",		LFADD, AFSUB,
	"FSUBN",	LFADD, AFSUBN,
	"FSUBT",	LFADDT, AFSUBT,
	"FSUBTN",	LFADDT, AFSUBTN,
	"FMADD",	LFMADD, AFMADD,
	"FMADDN",	LFMADD, AFMADDN,
	"FMADDT",	LFMADDT, AFMADDT,
	"FMADDTN",	LFMADDT, AFMADDTN,
	"FMSUB",	LFMADD, AFMSUB,
	"FMSUBN",	LFMADD, AFMSUBN,
	"FMSUBT",	LFMADDT, AFMSUBT,
	"FMSUBTN",	LFMADDT, AFMSUBTN,

	"FMUL",		LFADD, AFMUL,
	"FMULN",	LFADD, AFMULN,
	"FMULT",	LFADDT, AFMULT,
	"FMULTN",	LFADDT, AFMULTN,

	"FDIV",		LFDIV, AFDIV,
	"FMOVF",	LFMOV, AFMOVF,
	"FMOVFN",	LFMOV, AFMOVFN,
	"FMOVFB",	LFMOV, AFMOVFB,
	"FMOVFW",	LFMOV, AFMOVFW,
	"FMOVFH",	LFMOV, AFMOVFH,
	"FMOVBF",	LFMOV, AFMOVBF,
	"FMOVWF",	LFMOV, AFMOVWF,
	"FMOVHF",	LFMOV, AFMOVHF,
	"FIEEE",	LFOP, AFIEEE,
	"FROUND",	LFOP, AFRND,
	"FSEED",	LFOP, AFSEED,
	"FIFEQ",	LFOP, AFIFEQ,
	"FIFGT",	LFOP, AFIFGT,
	"FIFLT",	LFOP, AFIFLT,

	"FDSP",		LFDSP, AFDSP,

	"DO",		LDO, ADO,
	"DOLOCK",	LDO, ADOLOCK,
	"DOEND",	LDOEND, ADOEND,
	"CALL",		LCALL, ACALL,
	"DBRA",		LDBRA, ADBRA,
	"IRET",		LIRET, AIRET,
	"JMP",		LJMP, AJMP,
	"BRA",		LBRA, ABRA,
	"RETURN",	LRETRN, ARETURN,
	"SFTRST",	LSFTRST, ASFTRST,
	"WAITI",	LWAITI, AWAITI,

	"TRUE",		LCC, CCTRUE,
	"FALSE",	LCC, CCFALSE,
	"CC",		LCC, CCCC,
	"CS",		LCC, CCCS,
	"EQ",		LCC, CCEQ,
	"GE",		LCC, CCGE,
	"GT",		LCC, CCGT,
	"HI",		LCC, CCHI,
	"LE",		LCC, CCLE,
	"LS",		LCC, CCLS,
	"LT",		LCC, CCLT,
	"MI",		LCC, CCMI,
	"NE",		LCC, CCNE,
	"OC",		LCC, CCOC,
	"OS",		LCC, CCOS,
	"PL",		LCC, CCPL,
	"FNE",		LCC, CCFNE,
	"FEQ",		LCC, CCFEQ,
	"FGE",		LCC, CCFGE,
	"FLT",		LCC, CCFLT,
	"FOC",		LCC, CCFOC,
	"FOS",		LCC, CCFOS,
	"FUC",		LCC, CCFUC,
	"FUS",		LCC, CCFUS,
	"FGT",		LCC, CCFGT,
	"FLE",		LCC, CCFLE,
	"IBE",		LCC, CCIBE,
	"IBF",		LCC, CCIBF,
	"OBE",		LCC, CCOBE,
	"OBF",		LCC, CCOBF,
	"SYC",		LCC, CCSYC,
	"SYS",		LCC, CCSYS,
	"FBC",		LCC, CCFBC,
	"FBS",		LCC, CCFBS,
	"IR0C",		LCC, CCIR0C,
	"IR0S",		LCC, CCIR0S,
	"IR1C",		LCC, CCIR1C,
	"IR1S",		LCC, CCIR1S,

	"TEXT",		LTEXT, ATEXT,
	"GLOBL",	LTEXT, AGLOBL,
	"DATA",		LDATA, ADATA,
	"END",		LEND, AEND,
	"NOP",		LNOP, ANOP,
	"WORD",		LWORD, AWORD,
	"LONG",		LWORD, AWORD,
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

void
cclean(void)
{

	outcode(AEND, 0, &nullgen, NREG, &nullgen);
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
	Dsp e;

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
	case D_INC:
	case D_DEC:
	case D_INDREG:
		break;

	case D_INCREG:
		Bputc(&obuf, a->offset);	/* second register */
		break;

	case D_NAME:
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
	case D_AFCONST:
		e = dtodsp(a->dval);
		Bputc(&obuf, e);
		Bputc(&obuf, e>>8);
		Bputc(&obuf, e>>16);
		Bputc(&obuf, e>>24);
		break;
	}
}

int
outsim(Gen *g)
{
	Sym *s;
	int sno, t;

	s = g->sym;
	if(s == S)
		return 0;
	sno = s->sym;
	if(sno < 0 || sno >= NSYM)
		sno = 0;
	t = g->name;
	if(h[sno].type == t && h[sno].sym == s)
		return sno;
	zname(s->name, t, sym);
	s->sym = sym;
	h[sym].sym = s;
	h[sym].type = t;
	sno = sym;
	sym++;
	if(sym >= NSYM)
		sym = 1;
	return sno;
}

/*
 * opcode condition operand operand operand
 */
void
outcode(int a, int cc, Gen *g1, int reg, Gen *g2)
{
	int sf, st;

	if(a != AGLOBL && a != ADATA)
		pc++;
	if(pass == 1)
		return;

	sf = outsim(g1);
	st = outsim(g2);
	if(sf == st && sf != 0)
		outsim(g1);

	Bputc(&obuf, a);
	Bputc(&obuf, reg | nosched);
	Bputc(&obuf, cc);
	Bputc(&obuf, lineno);
	Bputc(&obuf, lineno>>8);
	Bputc(&obuf, lineno>>16);
	Bputc(&obuf, lineno>>24);
	zaddr(g1, sf);
	zaddr(g2, st);
}

void
outfcode(int a, Gen *g1, Gen *g2, Gen *g3, int reg, Gen *g4)
{
	int s1, s2, s3, s4, n; 

	if(a != AGLOBL && a != ADATA)
		pc++;
	if(pass == 1)
		return;
	for(;;){
		s1 = outsim(g1);
		s2 = outsim(g2);
		s3 = outsim(g3);
		s4 = outsim(g4);
		if(!s1 || s1 != s2 && s1 != s3 && s1 != s4
		&& !s2 || s2 != s3 && s2 != s4
		&& !s3 || s3 != s4)
			break;
	}
	n = 3;
	if(g3->type == D_NONE){
		n = 2;
		if(g2->type == D_NONE)
			n = 1;
	}
	Bputc(&obuf, a);
	Bputc(&obuf, reg | nosched);
	Bputc(&obuf, n << 6);
	Bputc(&obuf, lineno);
	Bputc(&obuf, lineno>>8);
	Bputc(&obuf, lineno>>16);
	Bputc(&obuf, lineno>>24);
	zaddr(g1, s1);
	if(n > 1)
		zaddr(g2, s2);
	if(n > 2)
		zaddr(g3, s3);
	zaddr(g4, s4);
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
		Bputc(&obuf, 0);
		Bputc(&obuf, h->line);
		Bputc(&obuf, h->line>>8);
		Bputc(&obuf, h->line>>16);
		Bputc(&obuf, h->line>>24);
		zaddr(&nullgen, 0);
		zaddr(&g, 0);
	}
}

Dsp
dtodsp(double d)
{
	double in;
	long v;
	int exp, neg;

	neg = 0;
	if(d < 0.0){
		neg = 1;
		d = -d;
	}

	/*
	 * frexp returns with d in [1/2, 1)
	 * we want Mbits bits in the mantissa,
	 * plus a bit to make d in [1, 2)
	 */
	d = frexp(d, &exp);
	exp--;
	d = modf(d * (1 << (Dspbits + 1)), &in);

	v = in;
	if(d >= 0.5)
		v++;
	if(v >= (1 << (Dspbits + 1))){
		v >>= 1;
		exp++;
	}
	if(neg){
		v = -v;
		/*
		 * normalize for hidden 0 bit
		 */
		if(v & (1 << Dspbits)){
			v <<= 1;
			exp--;
		}
	}
	if(v == 0 || exp < -(Dspbias-1))	/* underflow */
		return 0;
	if(exp > Dspexp - Dspbias){		/* overflow */
		if(neg)
			v = 0;
		else
			v = ~0;
		exp = Dspexp - Dspbias;
	}
	v &= Dspmask;
	v = (v << 8) | ((exp + Dspbias) & Dspexp);
	if(neg)
		v |= 0x80000000;
	return v;
}

/*
 * crap for lexbody
 */
typedef struct {
	long	l;
	long	h;
} Ieee;

#include "../cc/lexbody"
#include "../cc/macbody"
