#include <u.h>
#include <libc.h>
#include <stdio.h>

enum {
	DOTO = 1,
	NAME,
	NUMBER,
	ALPHA,		/* okay, so it's mixing tokens with character classes.. */
	DIGIT,
	DFF,
	INV,
	TFF,
	BURIED,
	ENABLE,
	PVAL,
	QVAL,
	CKEN,
	RESET,
};

typedef struct Imp Imp;
typedef struct Func Func;
typedef struct Sym Sym;

struct Imp {
	int val;
	int mask;
};

struct Func {
	Sym **dep;
	int ndep;
	Imp *imp;
	int nimp;
	int inverted;
};

struct Sym {
	char *id;
	char used;		/* true if referenced elsewhere */
	char internal;		/* true if it doesn't drive a pin */
	char input;		/* true if value comes from a pin */
	char output;		/* true if it drives a pin */
	char toggle;		/* true if we toggle */
	Func *exp;
	Func *clk;
	Func *en;
	Func *cken;
	Func *reset;		/* clear function */
};

#define NID	1000
#define NCHAR	10000
#define NIMP	10000
#define NDEP	10000
#define NFUNC	1000

Sym idbuf[NID],*idp=idbuf;
char charbuf[NCHAR],*charp=charbuf;
Imp impbuf[NIMP],*impp=impbuf;
Sym *depbuf[NDEP],**depp=depbuf;
Func funcbuf[NFUNC],*funcp=funcbuf;

int peekt;
int peekc;
int numb;
Sym *symb;
int lineno=1;
FILE *stin;
FILE *stout1,*stout2;	/* for buffering trick */
FILE *netfp;
char ctype[128];
int gen;
int dflag;

void
setclass(char *s, int c)
{
	while (*s)
		ctype[*s++] = c;
}

void
doclass(void)
{
	setclass("abcdefghijklmnopqrstuvwxyz_-",ALPHA);
	setclass("ABCDEFGHIJKLMNOPQRSTUVWXYZ",ALPHA);
	setclass("0123456789",DIGIT);
	setclass(".",'.');
	setclass("@",'@');
	setclass("\n",'\n');
	setclass(" ",' ');
	setclass("\t",'\t');
}

int
getch(void)
{
	int c;
	if (c = peekc)
		peekc = 0;
	else
		c = fgetc(stin);
	return c;
}

void
spewn(int i)
{
	fprintf(stderr,"%d ",i);
	fflush(stderr);
}

void
spew(char *s)
{
	fprintf(stderr,s);
	fflush(stderr);
}

void
warn(char *s)
{
	fprintf(stderr,"line %d: warning: %s\n",lineno,s);
	fflush(stderr);
}

void
panic(char *s)
{
	fprintf(stderr,"line %d: %s\n",lineno,s);
	exits("xnf panic");
}

int
tok(void)
{
	int c,i,t;
	char *p;
	Sym *s;
	if (peekt) {
		i = peekt;
		peekt = 0;
		return i;
	}
	switch ((t = ctype[c=getch()])) {
	case ALPHA:
		for (p = charp; t == ALPHA || t == DIGIT; t = ctype[c=getch()])
			*p++ = c;
		peekc = c;
		*p++ = 0;
		for (s = idbuf; s->id; s++)
			if (strcmp(s->id,charp) == 0) {
				symb = s;
				return NAME;
			}
		symb = idp;
		idp->id = charp;
		idp++;
		if (idp >= &idbuf[NID])
			panic("author error: NID too small");
		charp = p;
		if (charp >= &charbuf[NCHAR])
			panic("author error: NCHAR too small");
		return NAME;
	case DIGIT:
		for (i = c-'0'; ctype[c=getch()] == DIGIT;)
			i = i*10 + c-'0';
		peekc = c;
		numb = i;
		return NUMBER;
	case '.':
		if (getch() == 'o')
			return DOTO;
		else {	/* skip dot anything else */
			while (getch() != '\n')
				;
			lineno++;
			return tok();
		}
	case '@':
		switch (getch()) {
		case 'd':
			return DFF;
		case 'i':
			return INV;
		case 'c':
			return RESET;
		case 't':
			return TFF;
		case 'b':
			return BURIED;
		case 'e':
			return ENABLE;
		case 'g':
			return CKEN;
		case 'P':
			return PVAL;	/* use pin value */
		case 'Q':
			warn("<-Q unimplemented");
			return QVAL;	/* use internal value */
		}
	case '\n':
		lineno++;
	case ' ':
	case '\t':
		return tok();
		break;
	default:
		return c;
		}
}

void
readin(void)
{
	int t;
	Sym *s;
	while ((t = tok()) != -1) {
		if (t != DOTO) {
			panic("expected .o");
		}
		if (tok() != NAME)
			panic("expected an ID");
		s = symb;
		s->output = 1;
		switch (t = tok()) {
		case DFF:
			s->clk = funcp;
			break;
		case TFF:
			s->clk = funcp;
			s->toggle = 1;
			break;
		case BURIED:
			s->internal = 1;
			s->exp = funcp;
			break;
		case ENABLE:
			s->en = funcp;
			break;
		case RESET:
			s->reset = funcp;
			break;
		case PVAL:
			break;
		case QVAL:
			break;
		case CKEN:
			s->cken = funcp;
			break;
		default:
			s->exp = funcp;
			peekt = t;
		}
		if ((t = tok()) == INV)
			funcp->inverted = 1;
		else
			peekt = t;
		funcp->dep = depp;
again:
		while ((t = tok()) == NAME) {
			*depp++ = symb;
			symb->used++;
			symb->input = 1;		/* provisional */
			funcp->ndep++;
			if (depp >= &depbuf[NDEP])
				panic("author error: NDEP too small");
		}
		if (t == PVAL || t == QVAL)
			goto again;	/* unimplemented P&Q */
		peekt = t;
		funcp->imp = impp;
		while ((t = tok()) == NUMBER) {
			impp->val = numb;
			if (tok() != ':')
				panic("expected a ':'");
			if (tok() != NUMBER)
				panic("expected a number");
			impp->mask = numb;
			impp++;
			if (impp >= &impbuf[NIMP])
				panic("author error: NIMP too small");
			funcp->nimp++;
		}
		peekt = t;
		funcp++;
		if (funcp >= &funcbuf[NFUNC])
			panic("author error: NFUNC too small");
	}
	for (s = idbuf; s < idp; s++)		/* cull for guys that have to come from without */
		if (s->internal)
			s->input = s->output = 0;
}

void
outtermd(Sym *d[], Imp f)
{
	int i,mask;
	for (i = 0, mask = 1; f.mask; i++, mask <<= 1) {
		if (mask & f.mask)
			fprintf(stdout,"%s%s",(mask&f.val) ? "" : "!",d[i]->id);
		f.mask &= ~mask;
	}
}

void
outfuncd(Func *f)
{
	int i;
	if (f->ndep == 0)
		fprintf(stdout,"%d",f->nimp);
	for (i = 0; i < f->nimp; i++) {
		outtermd(f->dep,f->imp[i]);
		if (i+1 < f->nimp)
			fprintf(stdout," | ");
	}
}

void
outsymd(Sym *s)
{
	if (s->exp == 0) {
		fprintf(stdout,"%s: input\n",s->id);
		return;
	}
	if (s->internal)
		fprintf(stdout,"int");
	fprintf(stdout,s->id);
	if (s->en) {
		fprintf(stdout,"(");
		outfuncd(s->en);
		fprintf(stdout,")");
	}
	if (s->clk) {
		fprintf(stdout,":");
		outfuncd(s->clk);
	}
	fprintf(stdout," %s= ",s->exp->inverted ? "~" : "");
	outfuncd(s->exp);
	fprintf(stdout,"\n");
fflush(stdout);
}

char *
sufname(Sym *s)
{
	static char sbuf[40];
	sprint(sbuf,"%s_%s",s->id,s->input ? "B" : "");		/* can't use real names */
	return sbuf;
}

char *
outterm(Sym *d[], Imp f, int xor)
{
	int i,j,mask;
	static char tbuf[40];
	mask = f.mask;
	if (mask == (mask & -mask)) {
		for (i = 0; mask != 1; i++, mask >>= 1)
			;
		sprint(tbuf,"%s%s",sufname(d[i]),(xor^(f.val!=0)) ? "" : ",, INV");
	}
	else {
		gen++;
		sprint(tbuf,"G%d",gen);
		fprintf(stout2,"SYM, G%d, %sAND\n",gen,xor?"N":"");
		fprintf(stout2,"PIN, O, O, G%d\n",gen);
		for (i = 0, j = 1, mask = 1; f.mask; i++, mask <<= 1) {
			if ((mask & f.mask) == 0)
				continue;
			fprintf(stout2,"PIN, %d, I, %s%s\n",j,sufname(d[i]),(f.val&mask) ? "" : ",, INV");
			f.mask &= ~mask;
			j++;
		}
		fprintf(stout2,"END\n");
	}
	return tbuf;
}

char *
outfunc(Func *f, int xor)
{
	int i;
	static char fbuf[40];
	xor ^= f->inverted;
	if (f == 0)
		return "";
	if (f->ndep == 0)
		sprint(fbuf,"%d",f->nimp ^ xor);	/* nimp is 0 or 1 here */
	else if (f->nimp == 1)
		sprint(fbuf,"%s",outterm(f->dep,f->imp[0],xor));
	else {
		gen++;
		sprint(fbuf,"G%d",gen);
		fprintf(stout1,"SYM, G%d, %sOR\n",gen,xor ? "N" : "");
		fprintf(stout1,"PIN, O, O, G%d\n",gen);
		for (i = 0; i < f->nimp; i++)
			fprintf(stout1,"PIN, %d, I, %s\n",i+1,outterm(f->dep,f->imp[i],0));
		fprintf(stout1,"END\n");
	}
	return fbuf;
}

void
outsym(Sym *s)
{
	if (s->used == 0 && s->internal)
		return;
	if (s->input || s->toggle && s->output) {
		char *bname="IBUF";
		if (strcmp("aclk",s->id) == 0)
			bname = "ACLK";
		else if (strcmp("gclk",s->id) == 0)
			bname = "GCLK";
		fprintf(stdout,"SYM, %s_B, %s\n",s->id,bname);
		fprintf(stdout,"PIN, O, O, %s_B\n",s->id);
		fprintf(stdout,"PIN, I, I, %s_\n",s->id);
		fprintf(stdout,"END\n");
	}
	if (s->output) {
		fprintf(stdout,"EXT, %s_, O\n",s->id);
		fprintf(netfp,"Net\t%s_\t%s_\tO\n",s->id,s->id);
	}
	else if (s->input) {
		fprintf(stdout,"EXT, %s_, I\n",s->id);
		fprintf(netfp,"Net\t%s_\t%s_\tI\n",s->id,s->id);
	}
	if (s->exp == 0)
		goto skip;
	if (s->clk) {
		fprintf(stdout,"SYM, %s, %sFF%s\n",s->id,s->internal?"D":"OUT",s->output?"Z":"");
		fprintf(stdout,"PIN, %s, O, %s_\n",s->output?"O":"Q",s->id);
		if (s->toggle)
			fprintf(stdout,"PIN, D, I, %s_X\n",s->id);
		else
			fprintf(stdout,"PIN, D, I, %s\n",outfunc(s->exp,0));
		if (s->cken) {
			if (s->output) {
				fprintf(stderr,"error: output %s can't have a clock enable\n",s->id);
				exits("bad clock enable");
			}
			else
				fprintf(stdout,"PIN, CE, I, %s\n",outfunc(s->cken,0));
		}
		else if (s->internal)
			fprintf(stdout,"PIN, CE, I, 1\n");
		fprintf(stdout,"PIN, C, I, %s\n",outfunc(s->clk,0));
		if (s->output)
			fprintf(stdout,"PIN, T, I, %s\n",(s->en) ? outfunc(s->en,1) : "0");
		else if (s->reset)
			fprintf(stdout,"PIN, RD, I, %s\n",outfunc(s->reset,0));
		else
			fprintf(stdout,"PIN, RD, I, 0\n");
	}
	else {
		fprintf(stdout,"SYM, %s_, %sBUF%s\n",s->id,s->internal?"":"O",s->en?"Z":"");
		fprintf(stdout,"PIN, O, O, %s_\n",s->id);
		fprintf(stdout,"PIN, I, I, %s\n",outfunc(s->exp,0));
		if (s->en)
			fprintf(stdout,"PIN, T, I, %s\n",outfunc(s->en,1));
	}
	fprintf(stdout,"END\n");
	if (s->toggle) {	/* create xor goo for toggle ff */
		fprintf(stdout,"SYM, %s_X, XOR\n",s->id);
		fprintf(stdout,"PIN, O, O, %s_X\n",s->id);
		fprintf(stdout,"PIN, 1, I, %s\n",outfunc(s->exp,0));
		fprintf(stdout,"PIN, 2, I, %s_%s\n",s->id,s->output?"B":"");
		fprintf(stdout,"END\n");
	}
skip:
	fflush(stdout);
	fflush(stout1);
	fflush(stout2);
}

char *boilerplate = "LCANET, 1\n\
USER,\n\
PROG, MIN2XNF\n\
PWR, 0, GND, 0\n\
PWR, 1, VCC, 1\n\
PART, 3064PG132\n";

void
dumpout(void)
{
	Sym *s;
	if (!dflag)
		fprintf(stdout,boilerplate);
	for (s = idbuf; s < idp; s++)
		if (dflag)
			outsymd(s);
		else
			outsym(s);
	if (!dflag)
		fprintf(stdout,"EOF\n");
}

void
main(int argc, char *argv[])
{
	int i;
	static char *p,netfile[40];
	doclass();
	stout1 = fdopen(dup(1,-1), "w");
	stout2 = fdopen(dup(1,-1), "w");
	if (argc == 1)
		readin();
	else
		for (i = 1; i < argc; i++) {
			if (strcmp(argv[i],"-d") == 0) {
				dflag = 1;
				continue;
			}
			if ((stin = fopen(argv[i],"r")) == 0) {
				fprintf(stderr,"can't open %s\n",argv[i]);
				exits("can't open file");
			}
			if (netfile[0] == 0) {
				strcpy(netfile,argv[i]);
				for (p = netfile; *p != '.' && *p != 0; p++)
					;
				sprint(p,".netpin");
				if ((netfp = fopen(netfile,"w")) == 0) {
					fprintf(stderr,"can't open %s\n",netfile);
					exits("can't create file");
				}
			}
			readin();
		}
	dumpout();
	exits((char *) 0);
}
