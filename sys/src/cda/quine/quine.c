#include <u.h>
#include <libc.h>
#include <stdio.h>

#define	PERLINE	8
#ifdef X386
#define NTMP	2000000
#else
#define	NTMP	10000000
#endif
#define	NDC	100000
#define	MARK	0x80000000L

long	itmp[NTMP];
long	dctmp[NDC];
long	mask;
int	warn;
int	qflag;
int	nterm;
int	ndc;
int	itmpp;
FILE 	*stin = stdin;

long	numb;
char	name[100];
char	cclass[128];
int	peeks;
int	peekc;
int	opeekc;
int	opeeks;
long	onumb;
enum
{
	EON	= 1,
	LINE,
	OUT,
	NUMB,
	COLON,
	PERC,
	NAME,
	ITER,
	EXTERN,
	UNKN,
};

/* function prototypes */
extern	int	main(int, char*[]);
long*	bsrch(long v, long *lb, long *hb, long msk);
extern	void	doexit(int s);
extern	int	rdata(void);
extern	void	buildclass(void);
extern	int	symb(void);
extern	int	gchar(void);
extern	void	expect(char*);
extern	void	doimp(long* , long* , long, long);
extern	void	doimpq(long* , long* , long, long);
extern	void	oterm(long, long, int);
extern	void	pimp(long*, long*, long, long, int);

void
pimp(long *start, long *end, long msk, long lvl, int n)
{
	fprint(2, "dc mask = 0x%lx level = 0x%lx num terms %d\n",
		msk, lvl, n);
	for(; start != end; start++)
		fprint(2, "	0x%lx %c\n",
			(*start & ~MARK), (*start & MARK)? 'X': ' ');
}

int
main(int argc, char *argv[])
{
	int i, j;

	warn = 1000;
	buildclass();
	while(argc > 1 && argv[1][0] == '-') {
		for(i=1; j = argv[1][i]; i++) {
			switch(j) {
			default:
				fprint(2, "unknown flag -%c\n", j);
				continue;

			case 'q':
				qflag = 1;
				break;

			case 'w':
				warn = atoi(&argv[1][i+1]);
				break;
			}
			break;
		}
		argc--;
		argv++;
	}
	if(argc > 2)
		fprint(2, "Too many arguments -- ignored\n");
	if(argc > 1) {
		stin = fopen(argv[1], "r");
		if(stin == 0) {
			fprint(2, "cannot open %s\n", argv[1]);
			doexit(1);
		}
	}

readmore:
	nterm = 0;
	itmpp = 0;
	peekc = opeekc;
	peeks = opeeks;
	numb = onumb;
	if(rdata())
		doexit(0);
	opeeks = peeks;
	opeekc = peekc;
	onumb = numb;
	if(qflag)
		doimpq(itmp, &itmp[itmpp], mask, 1L);
	else
		doimp(itmp, &itmp[itmpp], mask, 1L);
	if(nterm % PERLINE)
		fprintf(stdout, "\n");
	goto readmore;
}

void
doimpq(long *start, long *end, long mask, long lvl)
{
	long v, l, msk;
	long *p, *a, *b;
	int f;

/*	pimp(start, end, msk, lvl, end-start); /* */
	for(msk = mask; msk;  msk &= ~l) {
		l = msk & -msk;
		f = 0;
		if(l < lvl) {
			for(a=start; a<end; a++) {
				v = *a;
				if(v & l)
					continue;
				v &= mask;
				b = bsrch(v|l, a+1, end, mask);
				if(b == 0)
					continue;
				*a |= MARK;
				*b |= MARK;
			}
			continue;
		}
		p = end;
		for(a=start; a<end; a++) {
			v = *a;
			if(v & l)
				continue;
			v &= mask;
			b = bsrch(v|l, a+1, end, mask);
			if(b == 0)
				continue;
			if((*a & MARK) && (*b & MARK)) {
				*p++ = v|MARK;
				continue;
			}
			f = 1;
			if(p < &itmp[NTMP]) *p++ = v;
			else {
				fprint(2, "NTMP to small\n");
#ifdef PLAN9
				exits("quine: NTMP too small");
#else
				exit(1);
#endif
			}
			*a |= MARK;
			*b |= MARK;
		}
		if(f)
			doimpq(end, p, mask & ~l, l<<1);
	}
	for(a=start; a<end; a++)
		if(!(*a & MARK))
			oterm(*a, mask & ~msk, (ndc==-1)? '%': ':');
}

void
doimp(long *start, long *end, long mask, long lvl)
{
	long v, l, msk;
	long *p, *a, *b;

/*	pimp(start, end, msk, lvl, end-start); /* */
	for(msk = mask; msk;  msk &= ~l) {
		l = msk & -msk;
		if(l < lvl) {
			for(a=start; a<end; a++) {
				v = *a;
				if(v & l)
					continue;
				v &= mask;
				b = bsrch(v|l, a+1, end, mask);
				if(b == 0)
					continue;
				*a |= MARK;
				*b |= MARK;
			}
			continue;
		}
		p = end;
		for(a=start; a<end; a++) {
			v = *a;
			if(v & l)
				continue;
			v &= mask;
			b = bsrch(v|l, a+1, end, mask);
			if(b == 0)
				continue;
			if(p < &itmp[NTMP]) *p++ = v;
			else {
				fprint(2, "NTMP to small\n");
#ifdef PLAN9
				exits("quine: NTMP too small");
#else
				exit(1);
#endif
			}
			*a |= MARK;
			*b |= MARK;
		}
		if(p != end)
			doimp(end, p, mask & ~l, l<<1);
	}
	for(a=start; a<end; a++)
		if(!(*a & MARK))
			oterm(*a, mask & ~msk, (ndc==-1)? '%': ':');
}

void
doexit(int s)
{
#ifdef PLAN9
	exits(s ? "quine: abnormal exit" : 0);
#else
	exit(s);
#endif
}

long*
bsrch(long v, long *lb, long *hb, long msk)
{
	long v1, *mb;

loop:
	v1 = hb-lb;
	if(v1 <= 3) {
		while(lb < hb) {
			if(v == (*lb & msk))
				return lb;
			lb++;
		}
		return 0;
	}
	mb = lb + v1/2;
	v1 = (*mb & msk) - v;
	if(v1 > 0) {
		hb = mb;
		goto loop;
	}
	if(v1 < 0) {
		lb = mb+1;
		goto loop;
	}
	return mb;
}

int
rdata(void)
{
	int s, c;
	long v;

	if(ndc < 0)
		ndc = 0;
	if(ndc) {
		for(s=0; s<ndc; s++) {
			itmp[itmpp++] =  dctmp[s];
			if(itmpp >= NTMP) {
				fprint(2, "rdata: no room\n");
#ifdef PLAN9
				exits("rdata no room");
#else
				exit(1);
#endif
			}
		}
		ndc = -1;
		return 0;
	}
	mask = ~0L;

loop:
	s = symb();
	if(s == EON)
		return 1;
	if(s == ITER) {
		fprintf(stdout, ".r");
		for(;;) {
			c = gchar();
			if(c < 0)
				goto loop;
			putchar(c);
			if(c == '\n')
				goto loop;
		}
	}
 	if(s == EXTERN) {
 		fprintf(stdout, ".x");
 		while((c = gchar()) != '\n') putchar(c);
 		putchar(c);
 		goto loop;
 	}
	if(s != OUT)
		expect(".o");
	fflush(stdout);
	fprintf(stdout, ".o");
	for(;;) {
		s = symb();
		if(s == NUMB) {
			fprintf(stdout, " %ld", numb);
			continue;
		}
		if(s == NAME) {
			fprintf(stdout, " %s", name);
			continue;
		}
		break;
	}
	fprintf(stdout, "\n");
	if(s != LINE)
		expect("new line");
	for(;;) {
		s = symb();
		if(s == LINE)
			continue;
		if(s != NUMB)
			break;
		v = numb;
		c = symb();
		if(c != COLON && c != PERC)
			expect(":");
		s = symb();
		if(s != NUMB)
			expect("number");
		if(c == PERC) {
			if(ndc >= NDC) {
				fprint(2, "NDC(%d) too small\n", NDC);
				doexit(1);
			}
			dctmp[ndc++] = v;
		}
		if(mask == ~0L)
			mask = numb;
		else
		if(mask != numb) {
			fprint(2, "whoops, masks different\n");
#ifdef PLAN9
			exits("whoooops, masks different");
#else
			exit(1);
#endif
		}
		v &= mask;
		if((itmpp == 0) || (itmp[itmpp-1] < v)) {
			itmp[itmpp++] = v;
			if(itmpp >= NTMP) {
				fprint(2, "rdata: no room\n");
#ifdef PLAN9
				exits("rdata no room");
#else
				exit(1);
#endif
			}
			continue;
		}
		if(itmp[itmpp-1] > v) {
			fprint(2, "input not sorted\n");
#ifdef PLAN9
				exits("input not sorted");
#else
				exit(1);
#endif
		}
	}
	peeks = s;
	return 0;
}

void
buildclass(void)
{
	int c;

	memset(cclass, 0, sizeof(cclass));
	for(c='0'; c<='9'; c++)
		cclass[c] = 2;
	for(c='a'; c<='z'; c++)
		cclass[c] = 1;
	for(c='A'; c<='Z'; c++)
		cclass[c] = 1;
	cclass['_'] = 1;
	cclass['-'] = 1;
	cclass['+'] = 1;
	cclass['['] = 1;
	cclass[']'] = 1;
	cclass['#'] = 1;
	cclass['$'] = 1;
	cclass['~'] = 1;
	cclass['*'] = 1;
	cclass['<'] = 1;
	cclass['>'] = 1;
	cclass['@'] = 1;
}

int
symb(void)
{
	int c;
	char *cp;

	if(peeks) {
		c = peeks;
		peeks = 0;
		return c;
	}
	c = gchar();
	while(c == ' ')
		c = gchar();
	if(c <= 0) {
		peeks = EON;
		return EON;
	}
	if(c == '\n')
		return LINE;
	if(c == ':')
		return COLON;
	if(c == '%')
		return PERC;
	if(c == '.') {
		c = gchar();
		if(c == 'o')
			return OUT;
		if(c == 'r')
			return ITER;
 		if(c == 'x')
 			return EXTERN;
		return UNKN;
	}
	numb = 1;
	cp = name;
	while(cclass[c]) {
		*cp++ = c;
		if(cclass[c] != 2)
			numb = 0;
		c = gchar();
	}
	if(cp != name) {
		peekc = c;
		*cp = 0;
		if(numb) {
			numb = atol(name);
			return NUMB;
		}
		return NAME;
	}
	return UNKN;
}

gchar(void)
{
	int c;

	if(peekc) {
		c = peekc;
		peekc = 0;
		return c;
	}
	c = fgetc(stin);
	return c;
}

void
expect(char *s)
{

	fprint(2, "%s expected\n", s);
	doexit(1);
}

void
oterm(long v, long m, int c)
{

	fprintf(stdout, " %ld%c%ld", v&m, c, m);
	nterm++;
	if(!(nterm%PERLINE))
		fprintf(stdout, "\n");
}
