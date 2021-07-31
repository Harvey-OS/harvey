#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include "../6c/6.out.h"
#include "a.h"
#include "y.tab.h"

void
main(int argc, char *argv[])
{
	char *p;
	int nout, nproc, status, i, c;

	thechar = '6';
	thestring = "960";
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

	"R0",		LRREG,	D_R0+0,			/* 960 pfp, r0 */
	"R1",		LRREG,	D_R0+1,			/* 960 sp, r1 */
	"R2",		LRREG,	D_R0+2,			/* 960 rip, r2 */
	"R3",		LRREG,	D_R0+3,
	"R4",		LRREG,	D_R0+4,
	"R5",		LRREG,	D_R0+5,
	"R6",		LRREG,	D_R0+6,
	"R7",		LRREG,	D_R0+7,
	"R8",		LRREG,	D_R0+8,
	"R9",		LRREG,	D_R0+9,
	"R10",		LRREG,	D_R0+10,
	"R11",		LRREG,	D_R0+11,
	"R12",		LRREG,	D_R0+12,
	"R13",		LRREG,	D_R0+13,
	"R14",		LRREG,	D_R0+14,
	"R15",		LRREG,	D_R0+15,
	"R16",		LRREG,	D_R0+16,		/* g0 */
	"R17",		LRREG,	D_R0+17,
	"R18",		LRREG,	D_R0+18,
	"R19",		LRREG,	D_R0+19,
	"R20",		LRREG,	D_R0+20,
	"R21",		LRREG,	D_R0+21,
	"R22",		LRREG,	D_R0+22,
	"R23",		LRREG,	D_R0+23,
	"R24",		LRREG,	D_R0+24,
	"R25",		LRREG,	D_R0+25,
	"R26",		LRREG,	D_R0+26,
	"R27",		LRREG,	D_R0+27,
	"R28",		LRREG,	D_R0+28,		/* static register */
	"R29",		LRREG,	D_R0+29,		/* stack register */
	"R30",		LRREG,	D_R0+30,		/* link register */
	"R31",		LRREG,	D_R0+31,		/* 960 fp, g15 */

	"DATA",		LTYPE1,	ADATA,

	"ADJSP",	LTYPEX,	AADJSP,
	"SYSCALL",	LTYPEX,	ASYSCALL,
	"CALL",		LTYPEX, ACALL,
	"CALLS",	LTYPEX, ACALLS,

	"GLOBL",	LTYPEB,	AGLOBL,
	"TEXT",		LTYPEB,	ATEXT,
	"LONG",		LTYPES,	ALONG,
	"NOP",		LTYPES,	ANOP,
	"RTS",		LTYPEB,	ARTS,

	"ADDC",		LTYPEB, AADDC,
	"ADDI",		LTYPEB, AADDI,
	"ADDO",		LTYPEB, AADDO,
	"ALTERBIT",	LTYPEB, AALTERBIT,
	"AND",		LTYPEB, AAND,
	"ANDNOT",	LTYPEB, AANDNOT,
	"ATADD",	LTYPEB, AATADD,
	"ATMOD",	LTYPEB, AATMOD,
	"B",		LTYPED, AB,
	"BAL",		LTYPED, ABAL,
	"BBC",		LTYPED, ABBC,
	"BBS",		LTYPED, ABBS,
	"BE",		LTYPED, ABE,
	"BG",		LTYPED, ABG,
	"BGE",		LTYPED, ABGE,
	"BL",		LTYPED, ABL,
	"BLE",		LTYPED, ABLE,
	"BNE",		LTYPED, ABNE,
	"BNO",		LTYPED, ABNO,
	"BO",		LTYPED, ABO,
	"CHKBIT",	LTYPEB, ACHKBIT,
	"CLRBIT",	LTYPEB, ACLRBIT,
	"CMPDECI",	LTYPEB, ACMPDECI,
	"CMPDECO",	LTYPEB, ACMPDECO,
	"CMPI",		LTYPEB, ACMPI,
	"CMPIBE",	LTYPEB, ACMPIBE,
	"CMPIBG",	LTYPEB, ACMPIBG,
	"CMPIBGE",	LTYPEB, ACMPIBGE,
	"CMPIBL",	LTYPEB, ACMPIBL,
	"CMPIBLE",	LTYPEB, ACMPIBLE,
	"CMPIBNE",	LTYPEB, ACMPIBNE,
	"CMPIBNO",	LTYPEB, ACMPIBNO,
	"CMPIBO",	LTYPEB, ACMPIBO,
	"CMPINCI",	LTYPEB, ACMPINCI,
	"CMPINCO",	LTYPEB, ACMPINCO,
	"CMPO",		LTYPEB, ACMPO,
	"CMPOBE",	LTYPEB, ACMPOBE,
	"CMPOBG",	LTYPEB, ACMPOBG,
	"CMPOBGE",	LTYPEB, ACMPOBGE,
	"CMPOBL",	LTYPEB, ACMPOBL,
	"CMPOBLE",	LTYPEB, ACMPOBLE,
	"CMPOBNE",	LTYPEB, ACMPOBNE,
	"CONCMPI",	LTYPEB, ACONCMPI,
	"CONCMPO",	LTYPEB, ACONCMPO,
	"DADDC",	LTYPEB, ADADDC,
	"DIVI",		LTYPEB, ADIVI,
	"DIVO",		LTYPEB, ADIVO,
	"DMOVT",	LTYPEB, ADMOVT,
	"DSUBC",	LTYPEB, ADSUBC,
	"EDIV",		LTYPEB, AEDIV,
	"EMUL",		LTYPEB, AEMUL,
	"EXTRACT",	LTYPEB, AEXTRACT,
	"FAULTE",	LTYPEB, AFAULTE,
	"FAULTG",	LTYPEB, AFAULTG,
	"FAULTGE",	LTYPEB, AFAULTGE,
	"FAULTL",	LTYPEB, AFAULTL,
	"FAULTLE",	LTYPEB, AFAULTLE,
	"FAULTNE",	LTYPEB, AFAULTNE,
	"FAULTNO",	LTYPEB, AFAULTNO,
	"FAULTO",	LTYPEB, AFAULTO,
	"FLUSHREG",	LTYPEB, AFLUSHREG,
	"FMARK",	LTYPEB, AFMARK,
	"MARK",		LTYPEB, AMARK,
	"MODAC",	LTYPEB, AMODAC,
	"MODI",		LTYPEB, AMODI,
	"MODIFY",	LTYPEB, AMODIFY,
	"MODPC",	LTYPEB, AMODPC,
	"MODTC",	LTYPEB, AMODTC,
	"MOV",		LTYPEB, AMOV,
	"MOVA",		LTYPEB, AMOVA,
	"MOVIB",	LTYPEB, AMOVIB,
	"MOVIS",	LTYPEB, AMOVIS,
	"MOVOB",	LTYPEB, AMOVOB,
	"MOVOS",	LTYPEB, AMOVOS,
	"MOVQ",		LTYPEB, AMOVQ,
	"MOVT",		LTYPEB, AMOVT,
	"MOVV",		LTYPEB, AMOVV,
	"MULI",		LTYPEB, AMULI,
	"MULO",		LTYPEB, AMULO,
	"NAND",		LTYPEB, ANAND,
	"NOR",		LTYPEB, ANOR,
	"NOT",		LTYPEB, ANOT,
	"NOTAND",	LTYPEB, ANOTAND,
	"NOTBIT",	LTYPEB, ANOTBIT,
	"NOTOR",	LTYPEB, ANOTOR,
	"OR",		LTYPEB, AOR,
	"ORNOT",	LTYPEB, AORNOT,
	"REMI",		LTYPEB, AREMI,
	"REMO",		LTYPEB, AREMO,
	"RET",		LTYPEB, ARET,
	"ROTATE",	LTYPEB, AROTATE,
	"SCANBIT",	LTYPEB, ASCANBIT,
	"SCANBYTE",	LTYPEB, ASCANBYTE,
	"SETBIT",	LTYPEB, ASETBIT,
	"SHLI",		LTYPEB, ASHLI,
	"SHLO",		LTYPEB, ASHLO,
	"SHRDI",	LTYPEB, ASHRDI,
	"SHRI",		LTYPEB, ASHRI,
	"SHRO",		LTYPEB, ASHRO,
	"SPANBIT",	LTYPEB, ASPANBIT,
	"SUBC",		LTYPEB, ASUBC,
	"SUBI",		LTYPEB, ASUBI,
	"SUBO",		LTYPEB, ASUBO,
	"SYNCF",	LTYPEB, ASYNCF,
	"SYNMOV",	LTYPEB, ASYNMOV,
	"SYNMOVQ",	LTYPEB, ASYNMOVQ,
	"SYNMOVV",	LTYPEB, ASYNMOVV,
	"SYSCTL",	LTYPEB,	ASYSCTL,
	"END",		LTYPEB,	AEND,
	"TESTE",	LTYPED, ATESTE,
	"TESTG",	LTYPED, ATESTG,
	"TESTGE",	LTYPED, ATESTGE,
	"TESTL",	LTYPED, ATESTL,
	"TESTLE",	LTYPED, ATESTLE,
	"TESTNE",	LTYPED, ATESTNE,
	"TESTNO",	LTYPED, ATESTNO,
	"TESTO",	LTYPED, ATESTO,
	"XNOR",		LTYPEB, AXNOR,
	"XOR",		LTYPEB, AXOR,

	0
};

void
cinit(void)
{
	Sym *s;
	int i;

	nullgen.sym = S;
	nullgen.offset = 0;
	if(FPCHIP)
		nullgen.dval = 0;
	for(i=0; i<sizeof(nullgen.sval); i++)
		nullgen.sval[i] = 0;
	nullgen.type = D_NONE;
	nullgen.index = D_NONE;
	nullgen.scale = 0;

	nerrors = 0;
	iostack = I;
	iofree = I;
	peekc = IGN;
	nhunk = 0;
	for(i=0; i<NHASH; i++)
		hash[i] = S;
	for(i=0; itab[i].name; i++) {
		s = slookup(itab[i].name);
		if(s->type != LNAME)
			yyerror("double initialization %s", itab[i].name);
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
checkscale(int scale)
{

	switch(scale) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
		return;
	}
	yyerror("scale must be 1<<[01234]: %d", scale);
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
	Gen2 g2;

	g2.from = nullgen;
	g2.to = nullgen;
	outcode(AEND, &g2);
	Bflush(&obuf);
}

void
zname(char *n, int t, int s)
{

	Bputc(&obuf, ANAME);	/* as */
	Bputc(&obuf, t);		/* type */
	Bputc(&obuf, s);		/* sym */
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
	int i, t;
	char *n;
	Ieee e;

	t = 0;
	if(a->index != D_NONE || a->scale != 0)
		t |= T_INDEX;
	if(s != 0)
		t |= T_SYM;

	switch(a->type) {
	default:
		t |= T_TYPE;
	case D_NONE:
		if(a->offset != 0)
			t |= T_OFFSET;
		break;
	case D_FCONST:
		t |= T_FCONST;
		break;
	case D_SCONST:
		t |= T_SCONST;
		break;
	}
	Bputc(&obuf, t);

	if(t & T_INDEX) {	/* implies index, scale */
		Bputc(&obuf, a->index);
		Bputc(&obuf, a->scale);
	}
	if(t & T_OFFSET) {	/* implies offset */
		l = a->offset;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
	}
	if(t & T_SYM)		/* implies sym */
		Bputc(&obuf, s);
	if(t & T_FCONST) {
		ieeedtod(&e, a->dval);
		l = e.l;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
		l = e.h;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
		return;
	}
	if(t & T_SCONST) {
		n = a->sval;
		for(i=0; i<NSNAME; i++) {
			Bputc(&obuf, *n);
			n++;
		}
		return;
	}
	if(t & T_TYPE)
		Bputc(&obuf, a->type);
}

void
outcode(int a, Gen2 *g2)
{
	int sf, st, t;
	Sym *s;

	if(pass == 1)
		goto out;

jackpot:
	sf = 0;
	s = g2->from.sym;
	while(s != S) {
		sf = s->sym;
		if(sf < 0 || sf >= NSYM)
			sf = 0;
		t = g2->from.type;
		if(t == D_ADDR)
			t = g2->from.index;
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
	s = g2->to.sym;
	while(s != S) {
		st = s->sym;
		if(st < 0 || st >= NSYM)
			st = 0;
		t = g2->to.type;
		if(t == D_ADDR)
			t = g2->to.index;
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
	Bputc(&obuf, lineno);
	Bputc(&obuf, lineno>>8);
	Bputc(&obuf, lineno>>16);
	Bputc(&obuf, lineno>>24);
	switch(t = g2->type) {
	case D_NONE:
		t = 0;			/* 0 D_NONE */
		break;

	default:
		if(t < D_R0 || t >= D_R0+32) {
			yyerror("bad type in outcode: %d", t);
			t = D_R0;
		}
		t = (t - D_R0) + 1;	/* 1-32 D_R0+ */
		break;

	case D_CONST:
		t = g2->offset;
		if(t >= 32) {
			yyerror("bad offset in outcode", t);
			t = 0;
		}
		t = (t - 0) + 33;	/* 33-64 D_CONST+ */
		break;
	}
	Bputc(&obuf, t);
	zaddr(&g2->from, sf);
	zaddr(&g2->to, st);

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
		Bputc(&obuf, h->line);
		Bputc(&obuf, h->line>>8);
		Bputc(&obuf, h->line>>16);
		Bputc(&obuf, h->line>>24);
		Bputc(&obuf, 0);			/* reg */
		zaddr(&nullgen, 0);
		zaddr(&g, 0);
	}
}

void
praghjdicks(void)
{
	while(getnsc() != '\n')
		;
}

void
pragvararg(void)
{
	while(getnsc() != '\n')
		;
}

void
pragfpround(void)
{
	while(getnsc() != '\n')
		;
}

#include "../cc/lexbody"
#include "../cc/macbody"
#include "../cc/compat"
