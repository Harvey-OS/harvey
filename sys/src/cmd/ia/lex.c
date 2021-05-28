#include <ctype.h>
#define	EXTERN
#include "a.h"
#include "y.tab.h"

void
main(int argc, char *argv[])
{
	char *p;
	int nout, nproc, status, i, c;

	thechar = 'i';
	p = strrchr(argv[0], '/');
	if(p == nil)
		p = argv[0];
	else
		p++;
	if(*p == 'j')
		thechar = 'j';
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
	if(debug['j'])
		thechar = 'j';
	thestring = (thechar == 'j'? "riscv64" : "riscv");
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
	if(thechar == 'j')
		dodefine("XLEN=8");
	else
		dodefine("XLEN=4");
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
	if(thechar == 'j')
		dodefine("XLEN=8");
	else
		dodefine("XLEN=4");
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

	"F",		FR,	0,
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

	"CSR",		LCTL,	0,

	"ADD",		LADD,	AADD,
	"SLL",		LADD,	ASLL,
	"SLT",		LADD,	ASLT,
	"SLTU",		LADD,	ASLTU,
	"XOR",		LADD,	AXOR,
	"SRL",		LADD,	ASRL,
	"OR",		LADD,	AOR,
	"AND",		LADD,	AAND,
	"SRA",		LADD,	ASRA,
	"ADDW",		LADD,	AADDW,
	"SLLW",		LADD,	ASLLW,
	"SRLW",		LADD,	ASRLW,
	"SRAW",		LADD,	ASRAW,
	"SUB",		LADD,	ASUB,
	"SUBW",		LADD,	ASUBW,

	"MUL",		LMUL,	AMUL,
	"MULH",		LMUL,	AMULH,
	"MULHSU",	LMUL,	AMULHSU,
	"MULHU",	LMUL,	AMULHU,
	"DIV",		LMUL,	ADIV,
	"DIVU",		LMUL,	ADIVU,
	"REM",		LMUL,	AREM,
	"REMU",		LMUL,	AREMU,
	"MULW",		LMUL,	AMULW,
	"DIVW",		LMUL,	ADIVW,
	"DIVUW",	LMUL,	ADIVUW,
	"REMW",		LMUL,	AREMW,
	"REMUW",	LMUL,	AREMUW,

	"BEQ",		LBEQ,	ABEQ,
	"BNE",		LBEQ,	ABNE,
	"BLT",		LBEQ,	ABLT,
	"BGE",		LBEQ,	ABGE,
	"BLTU",		LBEQ,	ABLTU,
	"BGEU",		LBEQ,	ABGEU,
	"BGT",		LBEQ,	ABGT,
	"BGTU",		LBEQ,	ABGTU,
	"BLE",		LBEQ,	ABLE,
	"BLEU",		LBEQ,	ABLEU,

	"JMP",		LBR,	AJMP,

	"RET",		LBRET,	ARET,
	"FENCE_I",	LBRET,	AFENCE_I,
	"NOP",		LBRET,	ANOP,
	"END",		LBRET,	AEND,

	"JAL",		LCALL,	AJAL,
	"JALR",		LCALL,	AJAL,

	"MOVB",		LMOVB,	AMOVB,
	"MOVH",		LMOVB,	AMOVH,

	"MOVF",		LMOVF,	AMOVF,
	"MOVD",		LMOVF,	AMOVD,

	"MOVBU",	LMOVBU,	AMOVBU,
	"MOVHU",	LMOVBU,	AMOVHU,

	"MOVW",		LMOVW,	AMOVW,

	"MOVFD",	LFLT2,	AMOVFD,
	"MOVDF",	LFLT2,	AMOVDF,
	"MOVWF",	LFLT2,	AMOVWF,
	"MOVUF",	LFLT2,	AMOVUF,
	"MOVFW",	LFLT2,	AMOVFW,
	"MOVWD",	LFLT2,	AMOVWD,
	"MOVUD",	LFLT2,	AMOVUD,
	"MOVDW",	LFLT2,	AMOVDW,
	"ADDF",		LFLT3,	AADDF,
	"ADDD",		LFLT3,	AADDD,
	"SUBF",		LFLT3,	ASUBF,
	"SUBD",		LFLT3,	ASUBD,
	"MULF",		LFLT3,	AMULF,
	"MULD",		LFLT3,	AMULD,
	"DIVF",		LFLT3,	ADIVF,
	"DIVD",		LFLT3,	ADIVD,
	"CMPLTF",	LFLT3,	ACMPLTF,
	"CMPLTD",	LFLT3,	ACMPLTD,
	"CMPEQF",	LFLT3,	ACMPEQF,
	"CMPEQD",	LFLT3,	ACMPEQD,
	"CMPLEF",	LFLT3,	ACMPLEF,
	"CMPLED",	LFLT3,	ACMPLED,

	"LUI",		LLUI,	ALUI,

	"SYS",		LSYS,	ASYS,
	"ECALL",	LSYS0,	0,
	"EBREAK",	LSYS0,	1,
	"CSRRW",	LCSR,	ACSRRW,
	"CSRRS",	LCSR,	ACSRRS,
	"CSRRC",	LCSR,	ACSRRC,

	"SWAP_W",	LSWAP,	ASWAP_W,
	"SWAP_D",	LSWAP,	ASWAP_D,
	"LR_W",		LSWAP,	ALR_W,
	"LR_D",		LSWAP,	ALR_D,
	"SC_W",		LSWAP,	ASC_W,
	"SC_D",		LSWAP,	ASC_D,

	"AMO_W",	LAMO,	AAMO_W,
	"AMO_D",	LAMO,	AAMO_D,

	"DATA",		LDATA, 		ADATA,
	"GLOBL",	LTEXT, 		AGLOBL,
	"TEXT",		LTEXT, 		ATEXT,
	"WORD",		LWORD,		AWORD,
	"DWORD",	LWORD,		ADWORD,

	"MOV",		LMOVW,		AMOV,
	"MOVWU",	LMOVBU,		AMOVWU,

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
	vlong v;
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
		break;

	case D_CTLREG:
	case D_OREG:
	case D_CONST:
	case D_BRANCH:
		l = a->offset;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
		break;

	case D_VCONST:
		v = a->vval;
		Bputc(&obuf, v);
		Bputc(&obuf, v>>8);
		Bputc(&obuf, v>>16);
		Bputc(&obuf, v>>24);
		Bputc(&obuf, v>>32);
		Bputc(&obuf, v>>40);
		Bputc(&obuf, v>>48);
		Bputc(&obuf, v>>56);
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
