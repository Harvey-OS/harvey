#include	<u.h>
#include	<libc.h>
#include	<stdio.h>

/*
 * looks for multiplexor-like
 * hazards of the form Ax+Bx?~
 * and adds the term +AB
 */
#define	PERLINE	8
#define	NIMP	40
int	rflg;
int	modflg;
struct	imp
{
	long	val;
	long	mask;
} imp[NIMP];
int	nwork;
char	work[20][20];
char	opname[20];
int	opin;

int	nimp;
int	inimp;
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

extern	void	otermnl(void);
extern	void	oterm(long, long, char);
extern	int	isnumber(char*);
extern	void	expect(char*);
extern	int	gchar(void);
extern	int	symb(void);
extern	int	rdata(void);
extern	int	cover(long, long, long, long);
extern	void	addimp(long, long);

void
main(int argc, char *argv[])
{
	int i, j;
	long b;

	nwork = 0;
	for(i=1; i<argc; i++) {
		if(*argv[i] == '-') {
			argv[i]++;
			if(*argv[i] == 'r') {
				rflg = 1;
				continue;
			}
			fprint(2, "unknown option -%c\n", *argv[i]);
			continue;
		}
		else  {
			strcpy(work[nwork],argv[i]);
			nwork++;
			continue;
		}
	}

start:
	if(rdata())
#ifdef PLAN9
		exits((char *) 0);
#else
		exit(0);
#endif

loop:
	inimp = nimp;
	modflg = 0;
	for(i=0; i<inimp; i++)
	for(j=i+1; j<inimp; j++) {
		b = imp[i].val^imp[j].val;
		b &= imp[i].mask&imp[j].mask;
		if(b == 0)
			continue;
		if((b & (b-1)) != 0)
			continue;
/*
		Fprint(1, "haz w %ld:%ld and %ld:%ld\n",
			imp[i].val, imp[i].mask,
			imp[j].val, imp[j].mask);
 */
		addimp((imp[i].val|imp[j].val) & ~b,
			(imp[i].mask|imp[j].mask) & ~b);
	}
	if(modflg && rflg)
		goto loop;
	for(i=0; i<nimp; i++)
		oterm(imp[i].val, imp[i].mask, ':');
	otermnl();
	goto start;
}

void
addimp(long v, long m)
{
	int i;

	if(nwork > 0) {
		for(i=0; i<nwork; i++)
			if(strcmp(work[i], opname) == 0)
				goto start;
		return;
	}

start:
	for(i=0; i<nimp; i++)
		if(cover(imp[i].val, imp[i].mask, v, m))
			return;

loop:
	for(i=inimp; i<nimp; i++) {
		if(cover(v, m, imp[i].val, imp[i].mask)) {
/*
			fprintf(stdout, "%ld:%ld >> %ld:%ld\n",
				v, m, imp[i].val, imp[i].mask);
 */
			for(i++; i<nimp; i++) {
				imp[i-1].val = imp[i].val;
				imp[i-1].mask = imp[i].mask;
			}
			modflg = 1;
			nimp--;
			goto loop;
		}
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
	imp[nimp].mask = m;
	nimp++;
/*
	fprintf(stdout, "%ld:%ld added\n", v, m);
 */
	modflg = 1;
}

int
cover(long v1, long m1, long v2, long m2)
{

	if((m2&m1) != m1)
		return 0;
	if((v2&m1) != v1)
		return 0;
	return(1);
}

int
rdata(void)
{
	int s, c, i;
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
	fprintf(stdout, ".o");
	opin = -1;
	for(;;) {
		s = symb();
		if(s == NUMB) {
			fprintf(stdout, " %ld", numb);
			if(opin == -1) {
				opin = numb;
				sprint(opname, "%d", numb);
			}
			continue;
		}
		if(s == NAME) {
			fprintf(stdout, " %s", name);
			if(opin == -1) {
				opin = 0;
				strcpy(opname, name);
				if(opname[i = (strlen(opname) - 1)] == '~') {
					opname[i--] = 0;
					if(opname[i] == '#') 
						opname[i] = 0;
				}
				else if(opname[i] == '#') {
					opname[i--] = 0;
					if(opname[i] == '~') 
						opname[i] = 0;
				}
			}
			continue;
		}
		break;
	}
	fprintf(stdout, "\n");
	if(s != LINE)
		expect("new line");
	nimp = 0;
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
			oterm(v, numb, '%');
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
	otermnl();
	peeks = s;
	return(0);
}

int
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
	return UNKN;
}

int
gchar(void)
{
	int c;

	if(peekc) {
		c = peekc;
		peekc = 0;
		return(c);
	}
	c = getchar();
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

int
isnumber(char *s)
{

	for(; *s; s++)
		if(*s < '0' || *s > '9')
			return 0;
	return 1;
}

int	ntout = { 0 };

void
oterm(long v, long m, char c)
{

	fprintf(stdout, " %ld%c%ld", v&m, c, m);
	ntout++;
	if(ntout >= PERLINE)
		otermnl();
}

void
otermnl(void)
{

	if(ntout > 0) {
		fprintf(stdout, "\n");
		ntout = 0;
	}
}
