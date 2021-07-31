#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include "../zc/z.out.h"
#include "a.h"
#include "y.tab.h"

void
main(int argc, char *argv[])
{
	char ofile[100], incfile[20], *p;
	int nout, nproc, status, i, c, of;

	thechar = 'z';
	thestring = "hobbit";
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
	".B",		LWID,	W_B,
	".UB",		LWID,	W_UB,
	".BU",		LWID,	W_UB,
	".H",		LWID,	W_H,
	".UH",		LWID,	W_UH,
	".HU",		LWID,	W_UH,
	".W",		LWID,	W_W,
	".F",		LWID,	W_F,
	".D",		LWID,	W_D,
	".E",		LWID,	W_E,

	".DW",		LDQWID,	W_B,
	".QW",		LDQWID,	W_W,

	"SP",		LSP,	D_AUTO,
	"SB",		LSB,	D_EXTERN,
	"FP",		LFP,	D_PARAM,
	"PC",		LPC,	D_BRANCH,

	"R",		LR,	0,

	"ADD",		LTYPE1, AADD,
	"ADD3",		LTYPE1, AADD3,
	"ADDI",		LTYPE1, AADDI,
	"AND",		LTYPE1, AAND,
	"AND3",		LTYPE1, AAND3,
	"ANDI",		LTYPE1, AANDI,
	"CALL",		LTYPE3, ACALL,
	"CATCH",	LTYPE3, ACATCH,
	"CMPEQ",	LTYPE1, ACMPEQ,
	"CMPGT",	LTYPE1, ACMPGT,
	"CMPHI",	LTYPE1, ACMPHI,
	"CPU",		LTYPE2, ACPU,
	"CRET",		LTYPE2, ACRET,
	"DATA",		LTYPED, ADATA,
	"DIV",		LTYPE1, ADIV,
	"DIV3",		LTYPE1, ADIV3,
	"DQM",		LTYPE1, ADQM,
	"END",		LTYPE2, AEND,
	"ENTER",	LTYPE3, AENTER,
	"FADD",		LTYPE1, AFADD,
	"FADD3",	LTYPE1, AFADD3,
	"FCLASS",	LTYPE1, AFCLASS,
	"FCMPEQ",	LTYPE1, AFCMPEQ,
	"FCMPEQN",	LTYPE1, AFCMPEQN,
	"FCMPGE",	LTYPE1, AFCMPGE,
	"FCMPGT",	LTYPE1, AFCMPGT,
	"FCMPN",	LTYPE1, AFCMPN,
	"FDIV",		LTYPE1, AFDIV,
	"FDIV3",	LTYPE1, AFDIV3,
	"FLOGB",	LTYPE1, AFLOGB,
	"FLUSHD",	LTYPE2, AFLUSHD,
	"FLUSHDCE",	LTYPE3, AFLUSHDCE,
	"FLUSHI",	LTYPE2, AFLUSHI,
	"FLUSHP",	LTYPE2, AFLUSHP,
	"FLUSHPBE",	LTYPE3, AFLUSHPBE,
	"FLUSHPTE",	LTYPE3, AFLUSHPTE,
	"FMOV",		LTYPE1, AFMOV,
	"FMUL",		LTYPE1, AFMUL,
	"FMUL3",	LTYPE1, AFMUL3,
	"FNEXT",	LTYPE1, AFNEXT,
	"FREM",		LTYPE1, AFREM,
	"FSCALB",	LTYPE1, AFSCALB,
	"FSQRT",	LTYPE1, AFSQRT,
	"FSUB",		LTYPE1, AFSUB,
	"FSUB3",	LTYPE1, AFSUB3,
	"GLOBL",	LTYPE1, AGLOBL,
	"JMP",		LTYPE5, AJMP,
	"JMPF",		LTYPE5, AJMPF,
	"JMPFN",	LTYPE5, AJMPFN,
	"JMPFY",	LTYPE5, AJMPFY,
	"JMPT",		LTYPE5, AJMPT,
	"JMPTN",	LTYPE5, AJMPTN,
	"JMPTY",	LTYPE5, AJMPTY,
	"KCALL",	LTYPE3, AKCALL,
	"KRET",		LTYPE2, AKRET,
	"LDRAA",	LTYPE5, ALDRAA,
	"LONG",		LTYPE3, ALONG,
	"MOV",		LTYPE7, AMOV,
	"MOVA",		LTYPE1, AMOVA,
	"MUL",		LTYPE1, AMUL,
	"MUL3",		LTYPE1, AMUL3,
	"NAME",		LTYPE1, ANAME,
	"NOP",		LTYPE2, ANOP,
	"OR",		LTYPE1, AOR,
	"OR3",		LTYPE1, AOR3,
	"ORI",		LTYPE1, AORI,
	"POPN",		LTYPE3, APOPN,
	"REM",		LTYPE1, AREM,
	"REM3",		LTYPE1, AREM3,
	"RETURN",	LTYPE6, ARETURN,
	"SHL",		LTYPE1, ASHL,
	"SHL3",		LTYPE1, ASHL3,
	"SHR",		LTYPE1, ASHR,
	"SHR3",		LTYPE1, ASHR3,
	"SUB",		LTYPE1, ASUB,
	"SUB3",		LTYPE1, ASUB3,
	"TADD",		LTYPE1, ATADD,
	"TESTC",	LTYPE2, ATESTC,
	"TESTV",	LTYPE2, ATESTV,
	"TEXT",		LTYPET, ATEXT,
	"TSUB",		LTYPE1, ATSUB,
	"UDIV",		LTYPE1, AUDIV,
	"UREM",		LTYPE1, AUREM,
	"USHR",		LTYPE1, AUSHR,
	"USHR3",	LTYPE1, AUSHR3,
	"WORD",		LTYPE3, AWORD,
	"XOR",		LTYPE1, AXOR,
	"XOR3",		LTYPE1, AXOR3,
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
	nullgen.width = W_NONE;
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
	char *p;

	s->type = LNAME;
	s->value = 0;
	if(symb[0] == 'R') {
		for(p = symb+1; *p; p++)
			if(!isdigit(*p))
				return;
		s->type = LREG;
		s->value = atol(symb+1);
	}
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

	outcode(AEND, &nullgen, &nullgen);
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
	int i, t;
	char *n;
	Ieee e;

	t = a->type;
	Bputc(&obuf, t);
	Bputc(&obuf, a->width);
	Bputc(&obuf, s);
	switch(t & ~D_INDIR) {
	default:
		print("unknown type %d\n", a->type);
		exits("arg");

	case D_NONE:
		break;

	case D_CPU:
	case D_CONST:
	case D_BRANCH:
	case D_ADDR:
	case D_REG:
	case D_EXTERN:
	case D_STATIC:
	case D_AUTO:
	case D_PARAM:
		l = a->offset;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
		break;

	case D_SCONST:
		n = a->sval;
		l = a->width;
		Bputc(&obuf, l);
		for(i=0; i<l; i++) {
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
outcode(int a, Gen *g1, Gen *g2)
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
		t = g1->type & ~D_INDIR;
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
		t = g2->type & ~D_INDIR;
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
