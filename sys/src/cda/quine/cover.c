#include <u.h>
#include <libc.h>
#include <stdio.h>

#define	PERLINE	8
#define	NIMP	500
#define	NDTC	(1000)
#define	NSET	12000
struct	imp
{
	long	val;
	long	mask;
} imp[NIMP], dtc[NDTC];

char	ven[NIMP][NSET];
char	vene[NIMP];
char	bvene[NIMP];
int	sflag;
int	opin;
int	warn;
int	nterm;
FILE	*fin;
int	nreduce;
int	nimp;
int	ndtc;
int	nset;
long	numb;
char	name[100];
int	peeks;
int	peekc;
#define	EON	1
#define	LINE	2
#define	OUT	3
#define	NUMB	4
#define	COLON	5
#define	PERC	6
#define	NAME	7
#define	ITER	8
#define EXTERN	9
#define	UNKN	10

/* function argument prototypes */
extern	void	reduce(int, int);
extern	int	rdata(void);
extern	int	symb(void);
extern	int	gchar(void);
extern	void	expect(char*);
extern	void	oterm(long, long, int);

main(int argc, char *argv[])
{
	int i, j, k;
	long mask, v;

	sflag = 0;
	warn = 1000;
	fin = stdin;
	while(argc > 1 && argv[1][0] == '-') {
		for(i=1; j = argv[1][i]; i++) {
			switch(j) {
			default:
				fprint(2, "flag -%c unlnown\n", j);
				continue;

			case 's':
				sflag++;
				continue;
			case 'w':
				warn = atoi(&argv[1][i+1]);
				break;
			}
			break;
		}
		argc--;
		argv++;
	}
	if(argc > 1) {
		if(argc > 2)
			fprint(2, "too many arguments ignored\n");
		fin = fopen(argv[1], "r");
		if(fin == 0) {
			fprint(2, "cannot open %s\n", argv[1]);
#ifdef PLAN9
			exits("can't open file");
#else
			exit(1);
#endif
		}
	}

start:
	if(rdata())
#ifdef PLAN9
		exits((char *) 0);
#else
		exit(0);
#endif
	nterm = 0;
	mask = 0;
	for(i=0; i<nimp; i++)
		mask |= imp[i].mask;

loop:
	nset = 0;
	for(v=0; v<=mask; v++) {
		for(j=0; j<ndtc; j++)
			if(dtc[j].val == (v & dtc[j].mask))
				goto newv;
		nreduce = 0;
		for(j=0; j<nimp; j++) {
			vene[j] = 0;
			if(imp[j].val == (v & imp[j].mask)) {
				vene[j] = 1;
				nreduce++;
			}
		}
		if(nreduce == 0)
			continue;
		for(j=0; j<nset; j++) {
			for(k=0; k<nimp; k++)
				if(vene[k] != ven[k][j])
					goto newj;
			goto newv;
		newj:;
		}
		if(nset >= NSET) {
			fprint(2, "NSET(%d) too small\n", NSET);
#ifdef PLAN9
			exits("NSET too small");
#else
			exit(1);
#endif
		}
		for(j=0; j<nimp; j++)
			ven[j][nset] = vene[j];
		nset++;
	newv:;
	}
/*
	Fprint(1, "nset = %d\n", nset);
	for(i=0; i<nimp; i++) {
		for(j=0; j<nset; j++)
			Fprint(1, "%d", ven[i][j]);
		Fprint(1, " %ld:%ld\n", imp[i].val, imp[j].val);
	}
 */
	nreduce = 0;
	for(i=0; i<nset; i++) {
		k = -1;
		for(j=0; j<nimp; j++)
		if(ven[j][i]) {
			if(k >= 0) {
				k = -1;
				break;
			}
			k = j;
		}
		if(k >= 0) {
			oterm(imp[k].val, imp[k].mask, ':');
			if(ndtc >= NDTC) {
				fprint(2, "NDTC(%d) too small\n", NDTC);
#ifdef PLAN9
				exits("NDTC too small");
#else
				exit(1);
#endif
			}
			dtc[ndtc].val = imp[k].val;
			dtc[ndtc].mask = imp[k].mask;
			ndtc++;
			nreduce++;
		}
	}
	if(nreduce)
		goto loop;
	nreduce = 0;
	for(i=0; i<nimp; i++) {
		vene[i] = 0;
		bvene[i] = 0;
		for(j=0; j<nset; j++)
		if(ven[i][j]) {
			vene[i] = 1;
			bvene[i] = 1;
			nreduce++;
			break;
		}
	}
	reduce(0, nreduce);
	for(i=0; i<nimp; i++) {
		if(bvene[i])
		oterm(imp[i].val, imp[i].mask, ':');
	}
	if(nterm % PERLINE)
		fprintf(stdout, "\n");
	if(nterm > warn)
		fprint(2, "warning opin %d: %d terms (>%d)\n",
			opin, nterm, warn);
	goto start;
}

void
reduce(int from, int count)
{
	int i, j;

start:
	if(from >= nimp)
		return;
	if(vene[from] == 0) {
		from++;
		goto start;
	}
	vene[from] = 0;
	for(i=0; i<nset; i++) {
		for(j=0; j<nimp; j++) {
			if(vene[j])
			if(ven[j][i])
				goto nexti;
		}
		vene[from] = 1;
		from++;
		goto start;
	nexti:;
	}
	if(count <= nreduce) {
		nreduce = count-1;
		for(i=0; i<nimp; i++)
			bvene[i] = vene[i];
	}
	if(sflag) {
		from++;
		count--;
		goto start;
	}
	reduce(from+1, count-1);
	vene[from] = 1;
	from++;
	goto start;
}

rdata(void)
{
	int s, c;
	long v;

loop:
	s = symb();
	if(s == EON)
		return(1);
 	if(s == EXTERN) {
 		fprintf(stdout, ".x");
 		while((c = gchar()) != '\n') putchar(c);
 		putchar(c);
 		goto loop;
 	}
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
	if(s != OUT)
		expect(".o");
	fflush(stdout);
	fprintf(stdout, ".o");
	opin = -1;
	for(;;) {
		s = symb();
		if(s == NUMB) {
			fprintf(stdout, " %ld", numb);
			if(opin == -1)
				opin = numb;
			continue;
		}
		if(s == NAME) {
			fprintf(stdout, " %s", name);
			if(opin == -1)
				opin = 0;
			continue;
		}
		break;
	}
	fprintf(stdout, "\n");
	if(s != LINE)
		expect("new line");
	nimp = 0;
	ndtc = 0;
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
			if(ndtc >= NDTC) {
				fprint(2, "buffer[DTC] too small\n");
#ifdef PLAN9
				exits("buffer[DTC] too small");
#else
				exit(1);
#endif
			}
			dtc[ndtc].val = v;
			dtc[ndtc].mask = numb;
			ndtc++;
			/*oterm(v, numb, '%');*/
			continue;
		}
		if(nimp >= NIMP) {
			fprint(2, "buffer[IMP] too small\n");
#ifdef PLAN9
			exits("buffer[IMP] too small");
#else
			exit(1);
#endif
		}
		imp[nimp].val = v;
		imp[nimp].mask = numb;
		nimp++;
	}
	peeks = s;
	return(0);
}

symb(void)
{
	int c;
	char *cp;

	if(peeks) {
		c = peeks;
		peeks = 0;
		return(c);
	}
	c = gchar();
	while(c == ' ')
		c = gchar();
	if(c <= 0) {
		peeks = EON;
		return(EON);
	}
	if(c == '\n')
		return(LINE);
	if(c == ':')
		return(COLON);
	if(c == '%')
		return(PERC);
	if(c == '.') {
		c = gchar();
		if(c == 'o')
			return(OUT);
		if(c == 'r')
			return ITER;
		if(c == 'x')
 			return EXTERN;
		return(UNKN);
	}
	numb = 1;
	cp = name;
	while((c >= '0' && c <= '9') ||
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		c == '-' || c == '_' || c == '+' || c == '[' || c == ']' ||
		c == '#' || c == '$' || c == '~' || c == '*' || c == '<' ||
		c == '>' || c == '@') {
			*cp++ = c;
			if(c < '0' || c > '9')
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
	return(UNKN);
}

gchar(void)
{
	int c;

	if(peekc) {
		c = peekc;
		peekc = 0;
		return(c);
	}
	c = fgetc(fin);
	return(c);
}

void
expect(char *s)
{

	fprint(2, "%s expected\n", s);
#ifdef PLAN9
	exits("syntax error");
#else
	exit(1);
#endif
}

void
oterm(long v, long m, int c)
{

	fprintf(stdout, " %ld%c%ld", v&m, c, m);
	nterm++;
	if(!(nterm%PERLINE))
		fprintf(stdout, "\n");
}
