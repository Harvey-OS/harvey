#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include "../9c/9.out.h"
#include "a.h"
#include "y.tab.h"

void
main(int argc, char *argv[])
{
	char *p;
	int nout, nproc, status, i, c;

	thechar = '9';
	thestring = "29000";
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

	"R",		LR,	0,
	"R0",		LREG,	0,
	"R1",		LREG,	1,
	"R131",		LREG,	131,
	"QREG",		LREG,	131,

	"ADDL",		LTYPE1, AADDL,
	"ADDUL",	LTYPE1, AADDUL,
	"ADDSL",	LTYPE1, AADDSL,
	"ADDCL",	LTYPE1, AADDCL,
	"ADDCUL",	LTYPE1, AADDCUL,
	"ADDCSL",	LTYPE1, AADDCSL,
	"SUBL",		LTYPE1, ASUBL,
	"SUBUL",	LTYPE1, ASUBUL,
	"SUBSL",	LTYPE1, ASUBSL,
	"SUBCL",	LTYPE1, ASUBCL,
	"SUBCUL",	LTYPE1, ASUBCUL,
	"SUBCSL",	LTYPE1, ASUBCSL,
	"ISUBL",	LTYPE1, AISUBL,
	"ISUBUL",	LTYPE1, AISUBUL,
	"ISUBSL",	LTYPE1, AISUBSL,
	"ISUBCL",	LTYPE1, AISUBCL,
	"ISUBCUL",	LTYPE1, AISUBCUL,
	"ISUBCSL",	LTYPE1, AISUBCSL,
	"ANDL",		LTYPE1, AANDL,
	"ANDNL",	LTYPE1, AANDNL,
	"NANDL",	LTYPE1, ANANDL,
	"NORL",		LTYPE1, ANORL,
	"ORL",		LTYPE1, AORL,
	"XNORL",	LTYPE1, AXNORL,
	"XORL",		LTYPE1, AXORL,
	"SRAL",		LTYPE1, ASRAL,
	"SRLL",		LTYPE1, ASRLL,
	"SLLL",		LTYPE1, ASLLL,
	"ASEQ",		LTYPE1, AASEQ,
	"ASGE",		LTYPE1, AASGE,
	"ASGEU",	LTYPE1, AASGEU,
	"ASGT",		LTYPE1, AASGT,
	"ASGTU",	LTYPE1, AASGTU,
	"ASLE",		LTYPE1, AASLE,
	"ASLEU",	LTYPE1, AASLEU,
	"ASLT",		LTYPE1, AASLT,
	"ASLTU",	LTYPE1, AASLTU,
	"ASNEQ",	LTYPE1, AASNEQ,
	"CPEQL",	LTYPE1, ACPEQL,
	"CPGEL",	LTYPE1, ACPGEL,
	"CPGEUL",	LTYPE1, ACPGEUL,
	"CPGTL",	LTYPE1, ACPGTL,
	"CPGTUL",	LTYPE1, ACPGTUL,
	"CPLEL",	LTYPE1, ACPLEL,
	"CPLEUL",	LTYPE1, ACPLEUL,
	"CPLTL",	LTYPE1, ACPLTL,
	"CPLTUL",	LTYPE1, ACPLTUL,
	"CPNEQL",	LTYPE1, ACPNEQL,
	"LOCKL",	LTYPE1, ALOCKL,
	"LOCKH",	LTYPE1, ALOCKH,
	"LOCKHU",	LTYPE1, ALOCKHU,
	"LOCKB",	LTYPE1, ALOCKB,
	"LOCKBU",	LTYPE1, ALOCKBU,
	"MOVL",		LTYPE3, AMOVL,
	"MOVH",		LTYPE3, AMOVH,
	"MOVHU",	LTYPE3, AMOVHU,
	"MOVB",		LTYPE3, AMOVB,
	"MOVBU",	LTYPE3, AMOVBU,
	"MOVF",		LTYPE3, AMOVF,
	"MOVD",		LTYPE3, AMOVD,
	"MOVFL",	LTYPE4, AMOVFL,
	"MOVDL",	LTYPE4, AMOVDL,
	"MOVLF",	LTYPE4, AMOVLF,
	"MOVLD",	LTYPE4, AMOVLD,
	"MOVFD",	LTYPE4, AMOVFD,
	"MOVDF",	LTYPE4, AMOVDF,
	"MTSR",		LTYPE9, AMTSR,
	"MFSR",		LTYPE10, AMFSR,
	"CALL",		LTYPE5, ACALL,
	"RET",		LTYPEG, ARET,
	"END",		LTYPEG, AEND,
	"JMP",		LTYPE5, AJMP,
	"JMPF",		LTYPE5, AJMPF,
	"JMPFDEC",	LTYPE5, AJMPFDEC,
	"JMPT",		LTYPE5, AJMPT,
	"DSTEPL",	LTYPE1, ADSTEPL,
	"DSTEP0L",	LTYPE1, ADSTEP0L,
	"DSTEPLL",	LTYPE1, ADSTEPLL,
	"DSTEPRL",	LTYPE1, ADSTEPRL,
	"DIVL",		LTYPE1, ADIVL,
	"DIVUL",	LTYPE1, ADIVUL,
	"MSTEPL",	LTYPE1, AMSTEPL,
	"MSTEPUL",	LTYPE1, AMSTEPUL,
	"MSTEPLL",	LTYPE1, AMSTEPLL,
	"MULL",		LTYPE1, AMULL,
	"MULUL",	LTYPE1, AMULUL,
	"MULML",	LTYPE1, AMULML,
	"MULMUL",	LTYPE1, AMULMUL,
	"ADDD",		LTYPE1, AADDD,
	"SUBD",		LTYPE1, ASUBD,
	"DIVD",		LTYPE1, ADIVD,
	"MULD",		LTYPE1, AMULD,
	"SQRTD",	LTYPE1, ASQRTD,
	"EQD",		LTYPE1, AEQD,
	"GED",		LTYPE1, AGED,
	"GTD",		LTYPE1, AGTD,
	"ADDF",		LTYPE1, AADDF,
	"SUBF",		LTYPE1, ASUBF,
	"DIVF",		LTYPE1, ADIVF,
	"MULF",		LTYPE1, AMULF,
	"SQRTF",	LTYPE1, ASQRTF,
	"EQF",		LTYPE1, AEQF,
	"GEF",		LTYPE1, AGEF,
	"GTF",		LTYPE1, AGTF,
	"CLZ",		LTYPE1, ACLZ,
	"CPBYTE",	LTYPE1, ACPBYTE,
	"CLASS",	LTYPE1, ACLASS,
	"EMULATE",	LTYPEA, AEMULATE,
	"EXBYTE",	LTYPE1, AEXBYTE,
	"EXHW",		LTYPE1, AEXHW,
	"EXHWS",	LTYPE1, AEXHWS,
	"EXTRACT",	LTYPE1, AEXTRACT,
	"HALT",		LTYPE1, AHALT,
	"INBYTE",	LTYPE1, AINBYTE,
	"INHW",		LTYPE1, AINHW,
	"IRETINV",	LTYPE8, AIRETINV,
	"INV",		LTYPE8, AINV,
	"IRET",		LTYPEG, AIRET,
	"LOADM",	LTYPE6, ALOADM,
	"LOADSET",	LTYPE6, ALOADSET,
	"SETIP",	LTYPE1, ASETIP,
	"STOREM",	LTYPE7, ASTOREM,

	"TEXT",		LTYPEB, ATEXT,
	"GLOBL",	LTYPEB, AGLOBL,
	"DATA",		LTYPEC, ADATA,
	"WORD",		LTYPEH, AWORD,
	"NOP",		LTYPEI, ANOP,
	"DELAY",	LTYPEI, ADELAY,

	"SCHED",	LSCHED, 0,
	"NOSCHED",	LSCHED, 1,
	0
};

void
cinit(void)
{
	Sym *s;
	char buf[32];
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

	for(i = 64; i < 256; i++){
		sprint(buf, "R%d", i);
		s = slookup(buf);
		s->type = LREG;
		s->value = i;
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
		break;

	case D_OREG:
	case D_CONST:
	case D_OCONST:
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
	Bputc(&obuf, reg|nosched);
	Bputc(&obuf, lineno);
	Bputc(&obuf, lineno>>8);
	Bputc(&obuf, lineno>>16);
	Bputc(&obuf, lineno>>24);
	zaddr(g1, sf);
	zaddr(g2, st);

out:
	if(a != AGLOBL && a != ADATA && a != ANOSCHED)
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
