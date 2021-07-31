#include <u.h>
#include <libc.h>
#include <bio.h>

#define	NDIM	10
#define	NTAB	601

char*	dfile	= "/lib/units";
char*	unames[NDIM];

typedef	struct	Unit	Unit;
typedef	struct	Table	Table;
typedef	struct	Prefix	Prefix;
struct	Unit
{
	double	factor;
	char	dim[NDIM];
};
struct Table
{
	Unit;
	char*	name;
};
struct	Prefix
{
	double	factor;
	char	*pname;
};

Table	table[NTAB];
char	names[NTAB*10];
Prefix	prefix[];
Biobuf	*inp;
Biobuf	bin;
int	fperrc;
int	peekc;
int	dumpflg;

int	main(int argc, char *argv[]);
void	units(Unit *up);
int	pu(int u, int i, int f);
int	convr(Unit *up);
int	lookup(char *name, Unit *up, int den, int c);
void	init(void);
double	getflt(void);
int	get(void);
Table*	hash(char *name);
void	fperr(void);

int
main(int argc, char *argv[])
{
	int i;
	char *file;
	Unit u1, u2;
	double f;

	if(argc>1 && *argv[1]=='-') {
		argc--;
		argv++;
		dumpflg++;
	}
	file = dfile;
	if(argc > 1)
		file = argv[1];
	if((inp = Bopen(file, 0)) == 0) {
		fprint(2, "cannot open %s\n", file);
		exits("open");
	}
	init();
	Binit(&bin, 0, OREAD);
	inp = &bin;

loop:
	fperrc = 0;
	print("you have: ");
	if(convr(&u1))
		goto loop;
	if(fperrc)
		goto fp;

loop1:
	print("you want: ");
	if(convr(&u2))
		goto loop1;
	for(i=0; i<NDIM; i++)
		if(u1.dim[i] != u2.dim[i])
			goto conform;
	f = u1.factor/u2.factor;
	if(fperrc)
		goto fp;
	print("\t* %g\n", f);
	print("\t/ %g\n", 1/f);
	goto loop;

conform:
	if(fperrc)
		goto fp;
	print("conformability\n");
	units(&u1);
	units(&u2);
	goto loop;

fp:
	print("underflow or overflow\n");
	goto loop;
}

void
units(Unit *up)
{
	Unit *p;
	int f, i;

	p = up;
	print("\t%g ", p->factor);
	f = 0;
	for(i=0; i<NDIM; i++)
		f |= pu(p->dim[i], i, f);
	if(f & 1) {
		print("%c", '/');
		f = 0;
		for(i=0; i<NDIM; i++)
			f |= pu(-p->dim[i], i, f);
	}
	print("\n");
}

int
pu(int u, int i, int f)
{

	if(u > 0) {
		if(f & 2)
			print("-");
		if(unames[i])
			print("%s", unames[i]);
		else
			print("*%c*", i+'a');
		if(u > 1)
			print("%c", u+'0');
		return 2;
	}
	if(u < 0)
		return 1;
	return 0;
}

int
convr(Unit *up)
{
	Unit *p;
	char name[20], *cp;
	int den, err, c;

	p = up;
	for(c=0; c<NDIM; c++)
		p->dim[c] = 0;
	p->factor = getflt();
	if(p->factor == 0)
		p->factor = 1;
	err = 0;
	den = 0;
	cp = name;

loop:
	switch(c = get()) {

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '-':
	case '/':
	case ' ':
	case '\t':
	case '\n':
		if(cp != name) {
			*cp = 0;
			cp = name;
			err |= lookup(cp, p, den, c);
		}
		if(c == '/')
			den++;
		if(c == '\n')
			return err;
		goto loop;
	}
	*cp++ = c;
	goto loop;
}

int
lookup(char *name, Unit *up, int den, int c)
{
	Unit *p;
	Table *q;
	int i;
	char *cp1, *cp2;
	double e;

	p = up;
	e = 1;

loop:
	q = hash(name);
	if(q->name) {
		l1:
		if(den) {
			p->factor /= q->factor*e;
			for(i=0; i<NDIM; i++)
				p->dim[i] -= q->dim[i];
		} else {
			p->factor *= q->factor*e;
			for(i=0; i<NDIM; i++)
				p->dim[i] += q->dim[i];
		}
		if(c >= '2' && c <= '9') {
			c--;
			goto l1;
		}
		return 0;
	}
	for(i=0; cp1 = prefix[i].pname; i++) {
		cp2 = name;
		while(*cp1 == *cp2++)
			if(*cp1++ == 0) {
				cp1--;
				break;
			}
		if(*cp1 == 0) {
			e *= prefix[i].factor;
			name = cp2-1;
			goto loop;
		}
	}
	for(cp1 = name; *cp1; cp1++)
		;
	if(cp1 > name+1 && *--cp1 == 's') {
		*cp1 = 0;
		goto loop;
	}
	print("cannot recognize %s\n", name);
	return 1;
}

void
init(void)
{
	char *cp, *np;
	Table *tp, *lp;
	int c, i, f, t;

	cp = names;
	for(i=0; i<NDIM; i++) {
		np = cp;
		*cp++ = '*';
		*cp++ = i+'a';
		*cp++ = '*';
		*cp++ = 0;
		lp = hash(np);
		lp->name = np;
		lp->factor = 1;
		lp->dim[i] = 1;
	}
	lp = hash("");
	lp->name = cp-1;
	lp->factor = 1;

l0:
	c = get();
	if(c == 0) {
		print("%d units; %d bytes\n\n", i, cp-names);
		if(dumpflg)
		for(tp = &table[0]; tp < &table[NTAB]; tp++) {
			if(tp->name == 0)
				continue;
			print("%s", tp->name);
			units(tp);
		}
		Bclose(inp);
		inp = 0;
		return;
	}
	if(c == '/') {
		while(c != '\n' && c != 0)
			c = get();
		goto l0;
	}
	if(c == '\n')
		goto l0;
	np = cp;
	while(c != ' ' && c != '\t') {
		*cp++ = c;
		c = get();
		if(c == 0)
			goto l0;
		if(c == '\n') {
			*cp++ = 0;
			tp = hash(np);
			if(tp->name)
				goto redef;
			tp->name = np;
			tp->factor = lp->factor;
			for(c=0; c<NDIM; c++)
				tp->dim[c] = lp->dim[c];
			i++;
			goto l0;
		}
	}
	*cp++ = 0;
	lp = hash(np);
	if(lp->name)
		goto redef;
	convr(lp);
	lp->name = np;
	f = 0;
	i++;
	if(lp->factor != 1)
		goto l0;
	for(c=0; c<NDIM; c++) {
		t = lp->dim[c];
		if(t>1 || (f>0 && t!=0))
			goto l0;
		if(f==0 && t==1) {
			if(unames[c])
				goto l0;
			f = c+1;
		}
	}
	if(f > 0)
		unames[f-1] = np;
	goto l0;

redef:
	print("redefinition %s\n", np);
	goto l0;
}

double
getflt(void)
{
	int c, i, dp, f;
	double d, e;

	d = 0;
	dp = 0;
	do
		c = get();
	while(c == ' ' || c == '\t');

l1:
	if(c >= '0' && c <= '9') {
		d = d*10 + c-'0';
		if(dp)
			dp++;
		c = get();
		goto l1;
	}
	if(c == '.') {
		dp++;
		c = get();
		goto l1;
	}
	if(dp)
		dp--;
	if(c == '+' || c == '-') {
		f = 0;
		if(c == '-')
			f++;
		i = 0;
		c = get();
		while(c >= '0' && c <= '9') {
			i = i*10 + c-'0';
			c = get();
		}
		if(f)
			i = -i;
		dp -= i;
	}
	e = 1;
	i = dp;
	if(i < 0)
		i = -i;
	while(i--)
		e *= 10;
	if(dp < 0)
		d *= e; else
		d /= e;
	if(c == '|')
		return d / getflt();
	peekc = c;
	return d;
}

int
get(void)
{
	int c;

	if(c = peekc) {
		peekc = 0;
		return c;
	}
	c = Bgetc(inp);
	if(c < 0) {
		if(Bfildes(inp) == 0) {
			print("\n");
			exits(0);
		}
		return 0;
	}
	return c;
}

Table*
hash(char *name)
{
	Table *tp;
	char *np;
	int h;

	h = 0;
	np = name;
	while(*np)
		h = h*57 + *np++ - '0';
	if(h < 0)
		h = -h;
	h %= NTAB;
	tp = &table[h];

l0:
	if(tp->name == 0)
		return tp;
	if(strcmp(name, tp->name) == 0)
		return tp;
	tp++;
	if(tp >= &table[NTAB])
		tp = table;
	goto l0;
}

void
fperr(void)
{

	fperrc++;
}

Prefix	prefix[] =
{
	1e-18,	"atto",
	1e-15,	"femto",
	1e-12,	"pico",
	1e-9,	"nano",
	1e-6,	"micro",
	1e-3,	"milli",
	1e-2,	"centi",
	1e-1,	"deci",
	1e1,	"deka",
	1e2,	"hecta",
	1e2,	"hecto",
	1e3,	"kilo",
	1e6,	"mega",
	1e6,	"meg",
	1e9,	"giga",
	1e12,	"tera",
	0,	0
};
