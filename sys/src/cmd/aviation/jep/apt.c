#include	<u.h>
#include	<libc.h>
#include	<bio.h>

#define	N(a)	(((a)>='0'&&(a)<='9')?'#':(a))
#define	H(a,b)	((N(a)<<8)|(b))
#define	G(p)	H(p[2][0],p[2][1])

typedef	struct	Db	Db;
struct	Db
{
	char*	name;
	int	nfield;
	int	nrec;
	char**	data;
};
typedef	struct	Etab	Etab;
struct	Etab
{
	char	name[NAMELEN+1];
	char*	dir;
};

Etab*	etab;
int	netab;
int	maxetab;
Biobuf*	bout;
char*	jn;
int	rflag;

extern	Db*	readdb(char*);
extern	void*	mal(ulong);
extern	void	sortby(Db*, int, int, int);
extern	void	unique(Db*);
extern	void	spitout(Db*, char**);
extern	double	dolat(char*);
extern	void	dumpdb(Db*, int);
extern	void	dumpdc(Db*, Db*, int);
extern	void	getdir(char*, char*);
extern	char**	lookup(Db*, char*);
extern	int	typeign(char**);
extern	int	typesid(char**);
extern	int	typeap(char**);
extern	int	typeapr(char**);
extern	char*	exist(char*);
extern	int	casestrcmp(char*, char*);

void
main(int argc, char *argv[])
{
	Db *dba, *dbb;
	char **dp, *t1, file[100];
	int i, aflag, cflag;
	Biobuf biobuf;

	bout = &biobuf;
	Binit(bout, 1, OWRITE);

/*
 * types
 *	#1 ils
 *	#2 radar
 *	#3 vor
 *	#4 tacan
 *	#5 copter
 *	#6 ndb
 *	#7 circling
 *	#8 radar
 *	#9 rnav
 *	#a cat 2 ils
 *	#b cat 2 ils
 *	#c cat 3 ils
 *	#d loc
 *	#e loc bcrs
 *	#f lda
 *	#g sdf
 *	#h mls
 *	#j visual
 *	#k vicinity chart
 *	#m gps
 *
 *	aa airport
 *	ap airport
 *	a  temp airport chart
 *	ad temp airport chart
 *	b  class b
 *	j2 descent step
 *	eo engine out
 *	g  departure
 *	mg operational restrictions
 *	n  noise abatement
 *	p  taxi routes
 *	r  parking
 *	s  guide in system
 *	st bird concentration plus
 *	tt vfr arrival
 *	vf vfr departure
 */

	t1 = "ap";
	jn = "1";
	aflag = 0;
	cflag = 0;

	ARGBEGIN {
	default:
		fprint(2, "usage: a.out\n");
		exits(0);
	case 't':	// type of chart
		t1 = ARGF();
		break;
	case 'r':	// remote airports ok
		rflag = 1;
		break;
	case 'a':	// dist air? files
		aflag = 1;
		break;
	case 'b':	// $air files
		aflag = 2;
		break;
	case 'c':	// bystate sorted approach plates
		cflag = 1;
		break;
	case 'j':	// jep cd number '1' is us, '2' is europe
		jn = ARGF();
		break;
	} ARGEND

	if(argc > 0)
		jn = argv[0];
	sprint(file, "a%s.stropria", jn);
	dba = readdb(file);
	if(dba == nil) {
		fprint(2, "apt: cant open %s: %r\n", file);
		exits("open");
	}

	if(aflag) {
		dumpdb(dba, aflag);
		exits(0);
	}

	sprint(file, "a%s.strach", jn);
	dbb = readdb(file);
	if(dbb == nil) {
		fprint(2, "apt: cant open %s: %r\n", file);
		exits("open");
	}

	getdir("/n/jep%s/charts/eff_new", jn);
	getdir("/n/jep%s/charts", jn);

	if(cflag) {
		dumpdc(dba, dbb, cflag);
		exits(0);
	}

	sortby(dba, 0, 0, 0);	// tla for binary lookup
	sortby(dbb, 0, 1, 1);	// tla, approach for binary lookup
	unique(dbb);

	for(i=0; i<dbb->nrec; i++) {
		dp = dbb->data + i*dbb->nfield;
		if(G(dp) == H(t1[0],t1[1])) {
			spitout(dba, dp);
			continue;
		}
	}

	exits(0);
}

int
etcmp(void *va, void *vb)
{
	Etab *a, *b;

	a = (Etab*)va;
	b = (Etab*)vb;
	return strcmp(a->name, b->name);
}

int
casestrcmp(char *a, char *b)
{
	int ca, cb;

	for(;;) {
		ca = *a++;
		cb = *b++;
		if(ca >= 'A' && ca <= 'Z')
			ca += 'a' - 'A';
		if(cb >= 'A' && cb <= 'Z')
			cb += 'a' - 'A';
		if(ca != cb) {
			if(ca > cb)
				return 1;
			return -1;
		}
		if(ca == 0)
			return 0;
	}
}

void
lcase(char *a)
{
	int c;

	while(c = *a) {
		if(c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
		*a++ = c;
	}
}

void
getdir(char *fmt, char *jn)
{
	int f, n;
	Dir statb;
	Etab *e;
	char file[100], *g, *p;

	sprint(file, fmt, jn);
	g = strdup(file);
	f = open(file, OREAD);
	if(f < 0) {
		fprint(2, "apt: cant open %s: %r\n", file);
		exits("getdir");
	}

	n = netab;
	for(;;) {
		if(dirread(f, &statb, sizeof(statb)) != sizeof(statb))
			break;
		lcase(statb.name);
		p = strstr(statb.name, ".tcl");
		if(p == nil)
			continue;
		*p = 0;
		if(exist(statb.name))
			continue;
		if(n >= maxetab) {
			maxetab += 10000;
			if(etab == nil)
				etab = malloc(maxetab*sizeof(*etab));
			else
				etab = realloc(etab, maxetab*sizeof(*etab));
		}
		e = etab + n;
		memcpy(e->name, statb.name, sizeof(statb.name));
		e->dir = g;
		n++;
	}
	close(f);

	netab = n;
	qsort(etab, netab, sizeof(*etab), etcmp);
}

char*
exist(char *nam)
{
	Etab *p, *pm;
	int c, m, t;

	p = etab;
	c = netab;

loop:
	if(c <= 0)
		return nil;
	m = c/2;
	pm = p + m;
	t = strcmp(nam, pm->name);
	if(t <= 0) {
		if(t == 0)
			return pm->dir;
		c = m;
		goto loop;
	}
	p = pm + 1;
	c = c-m-1;
	goto loop;
}

void
spitout(Db *d, char **r)
{
	char **p, **pm;
	int c, m, t;

	p = d->data;
	c = d->nrec;

loop:
	if(c <= 0) {
		fprint(2, "spitout: not found %s\n", r[0]);
		return;
	}
	m = c/2;
	pm = p + m*d->nfield;
	t = strcmp(r[0], pm[0]);
//print("cmp %s %s %d\n", pm[0], r[0], t);
	if(t <= 0) {
		if(t == 0) {
			if(exist(r[1]))
				Bprint(bout, "\t\"%s\",%.4f,%.4f,0,0,\n",
					r[1], dolat(pm[3]), dolat(pm[4]));
			return;
		}
		c = m;
		goto loop;
	}
	p = pm + d->nfield;
	c = c-m-1;
	goto loop;
}

void*
mal(ulong n)
{
	void *v;

	v = malloc(n);
	if(v == nil) {
		fprint(2, "out of mem\n");
		exits("mem");
	}
	return v;
}

double
dolat(char *s)
{
	ulong n;
	int a1, a2, a3, c;
	double f;
	char *q;

	q = s;
	n = 0;
	while(c = *s++)
		if(c >= '0' && c <= '9')
			n = n*10 + c-'0';
	a1 = n%10000;
	n = n/10000;
	a2 = n%100;
	a3 = n/100;
	f = a1/360000. + a2/60. + a3/1.;

	c = *q;
	if(c == 's' || c == 'e')
		f = -f;

	return f;
}

Db*
readdb(char *file)
{
	Biobuf *b;
	char *p, *q, **dp;
	int nfield, nrec, i, j;
	Db *d;

	b = Bopen(file, OREAD);
	if(b == nil)
		return nil;
	p = Brdline(b, '\n');
	if(p == nil)
		return nil;
	p[Blinelen(b)-1] = 0;
	for(nfield=0; p; nfield++) {
		p = strchr(p, ':');
		if(p != nil)
			p++;
	}

	p = Brdline(b, '\n');
	for(nrec=0; p; nrec++)
		p = Brdline(b, '\n');

	d = mal(sizeof(*d));
	d->name = strdup(file);
	d->nrec = nrec;
	d->nfield = nfield;
	d->data = mal(nrec*nfield*sizeof(*d->data));
	dp = d->data;

	Bseek(b, 0, 0);
	Brdline(b, '\n');
	for(i=0; i<nrec; i++) {
		p = Brdline(b, '\n');
		if(p == nil) {
			fprint(2, "phase error\n");
			return nil;
		}
		p[Blinelen(b)-1] = 0;
		for(j=0; j<nfield; j++) {
			q = p;
			p = strchr(p, ':');
			if(p != nil)
				*p++ = 0;
			*dp = "";
			if(*q != 0)
				*dp = strdup(q);
			dp++;
		}
	}
	Bterm(b);

	return d;
}

static	int	fno[3];

static	int
fcmp(void *va, void *vb)
{
	char **a, **b;
	int i, n;

	a = va;
	b = vb;
	for(i=0; i<nelem(fno); i++) {
		n = strcmp(a[fno[i]], b[fno[i]]);
		if(n)
			return n;
	}
	return 0;
}

void
sortby(Db *d, int f0, int f1, int f2)
{
	fno[0] = f0;
	fno[1] = f1;
	fno[2] = f2;
	qsort(d->data, d->nrec, d->nfield*sizeof(*d->data), fcmp);
}

void
unique(Db *d)
{
	char **dp0, **dp;
	int i, n;

	dp0 = nil;
	n = 0;
	for(i=0; i<d->nrec; i++) {
		dp = d->data + i*d->nfield;
		// skip charts that are not for this airport
		if(!rflag && memcmp(dp[0], dp[1], strlen(dp[0])) != 0) {
			continue;
		}
		if(dp0 != nil) {
			// skip duplicated charts
			if(strcmp(dp0[0], dp[0]) == 0)
			if(strcmp(dp0[1], dp[1]) == 0)
				continue;
			dp0 += d->nfield;
		} else
			dp0 = d->data;
		memmove(dp0, dp, d->nfield*sizeof(*dp0));
		n++;
	}
	d->nrec = n;
}

void
dumpdb(Db *d, int flag)
{
	char **dp;
	int i;
	char str[100];

	for(i=0; i<d->nrec; i++)
	switch(flag) {
	case 1:	/* dist air? files */
		dp = d->data + i*d->nfield;
		if(strcmp(dp[10], "usa") == 0) {
//			Bprint(bout, "%s,3,0,0,%.6f,%.6f,\n",
//				dp[0], dolat(dp[3]), dolat(dp[4]));
//			if(strlen(dp[0]) == 4 && *dp[0] == 'k')
//				Bprint(bout, "%s,3,0,0,%.6f,%.6f,\n",
//					dp[0]+1, dolat(dp[3]), dolat(dp[4]));
			break;
		}
		if(strcmp(dp[10], "can") == 0) {
			Bprint(bout, "%s,3,0,0,%.6f,%.6f,\n",
				dp[0], dolat(dp[3]), dolat(dp[4]));
			if(strlen(dp[0]) == 4 && *dp[0] == 'c')
				Bprint(bout, "%s,3,0,0,%.6f,%.6f,\n",
					dp[0]+1, dolat(dp[3]), dolat(dp[4]));
			break;
		}
		Bprint(bout, "%s,3,0,0,%.6f,%.6f,\n",
			dp[0], dolat(dp[3]), dolat(dp[4]));
		break;
	case 2:	/* $air file */
		dp = d->data + i*d->nfield;
		sprint(str, "%s", dp[8]);
		if(*dp[9])
			sprint(strchr(str, 0), ", %s", dp[9]);
		if(*dp[10] && strcmp(dp[10], "usa") != 0)
			sprint(strchr(str, 0), ", %s", dp[10]);
		if(strcmp(dp[10], "usa") == 0) {
//			Bprint(bout, "%-4s %10.6f %11.6f %6ld %s (%s)\n",
//				dp[0], dolat(dp[3]), dolat(dp[4]),
//				atol(dp[6]), str, dp[7]);
//			if(strlen(dp[0]) == 4 && *dp[0] == 'k')
//				Bprint(bout, "%-4s %10.6f %11.6f %6ld %s, %s (%s)\n",
//					dp[0]+1, dolat(dp[3]), dolat(dp[4]),
//					atol(dp[6]), dp[8], dp[9], dp[7]);
			break;
		}
		if(strcmp(dp[10], "can") == 0) {
			Bprint(bout, "%-4s %10.6f %11.6f %6ld %s (%s)\n",
				dp[0], dolat(dp[3]), dolat(dp[4]),
				atol(dp[6]), str, dp[7]);
			if(strlen(dp[0]) == 4 && *dp[0] == 'c')
				Bprint(bout, "%-4s %10.6f %11.6f %6ld %s, %s (%s)\n",
					dp[0]+1, dolat(dp[3]), dolat(dp[4]),
					atol(dp[6]), dp[8], dp[9], dp[7]);
			break;
		}
		Bprint(bout, "%-4s %10.6f %11.6f %6ld %s (%s)\n",
			dp[0], dolat(dp[3]), dolat(dp[4]),
			atol(dp[6]), str, dp[7]);
		break;
	}
}

int
typeign(char **p)
{

	switch(G(p)) {
	case H('a',0):		// temp chart
	case H('a','d'):	// temp chart
	case H('b',0):		// class b
	case H('e','o'):	// engine out
	case H('m','g'):	// operational restrictions
	case H('n',0):		// noise
	case H('p',0):		// taxi
	case H('r',0):		// parking
	case H('s',0):		// guide in system
	case H('s','t'):	// bird concentration +
	case H('t','t'):	// vfr arrival
	case H('v','f'):	// vfr departure

	case H('#','7'):	// circling
	case H('#','5'):	// copter
	case H('#','n'):	// fms (flight management system)
	case H('#','h'):	// mls
		return 1;
	}
	return 0;
}
int
typesid(char **p)
{
	switch(G(p)) {
	case H('g',0):		// departure
	case H('j',0):		// arrival
		return !typeign(p);
	}
	return 0;
}
int
typeap(char **p)
{
	switch(G(p)) {
	case H('a','a'):	// airport
	case H('a','p'):	// airport
		return 1;
	}
	return 0;
}
int
typeapr(char **p)
{
	switch(G(p)) {
	case H('#','1'):	// ils
	case H('#','2'):	// radar
	case H('#','3'):	// vor
	case H('#','4'):	// tacan
	case H('#','6'):	// ndb
	case H('#','8'):	// radar
	case H('#','9'):	// rnav
	case H('#','a'):	// cat 2 ils
	case H('#','b'):	// cat 2 ils
	case H('#','c'):	// cat 3 ils
	case H('#','d'):	// loc
	case H('#','e'):	// loc bcrs
	case H('#','f'):	// lda
	case H('#','g'):	// sdf
	case H('#','j'):	// visual
	case H('#','k'):	// vicinity chart
	case H('#','l'):	// rnav
	case H('#','m'):	// gps
	case H('j','2'):	// descent steps
		return 1;
	}
	return 0;
}

void
aprpr(char **da, char **dp)
{
	char *dir;

	dir = exist(dp[1]);
	if(dir == nil)
		Bprint(bout, "# ERR-EXIST 8.out %s.tcl # %s", dp[1], da[8]);
	else
		Bprint(bout, "8.out %s/%s.tcl # %s", dir, dp[1], da[8]);
	if(strcmp(da[9], "") != 0)
		Bprint(bout, ", %s", da[9]);
	if(strcmp(da[10], "") != 0)
		Bprint(bout, ", %s", da[10]);
	Bprint(bout, "\n");
}

void
dumpdc(Db *a, Db *p, int flag)
{
	char **da, **dp, **dp0, **mdp;
	int i, any;

	sortby(a, 10, 9, 8);	// country, state, city
	sortby(p, 0, 1, 1);	// tla, approach for binary lookup
	unique(p);

	mdp = p->data + p->nrec*p->nfield;

	for(i=0; i<a->nrec; i++)
	switch(flag) {
	case 1:	/* list of plates */
		da = a->data + i*a->nfield;
		dp0 = lookup(p, da[0]);
		if(dp0 == nil) {
			Bprint(bout, "# ERR-NIL %s,%s,%s\n", da[8], da[9], da[10]);
			break;
		}

		/*
		 * pass 1 - print sids and stars
		 * print first approach
		 */
		for(dp=dp0; dp<mdp; dp+=p->nfield) {
			if(strcmp(da[0], dp[0]))
				break;
			if(!typesid(dp) && !typeapr(dp))
				continue;
			aprpr(da, dp);
			if(typeapr(dp))
				break;
		}

		/*
		 * pass 2 - print airport diagrams
		 */
		for(dp=dp0; dp<mdp; dp+=p->nfield) {
			if(strcmp(da[0], dp[0]))
				break;
			if(!typeap(dp))
				continue;
			aprpr(da, dp);
		}

		/*
		 * pass 3 - print subsequent approach
		 */
		any = 0;
		for(dp=dp0; dp<mdp; dp+=p->nfield) {
			if(strcmp(da[0], dp[0]))
				break;
			if(!typeapr(dp))
				continue;
			if(any == 0) {
				any = 1;
				continue;
			}
			aprpr(da, dp);
		}

		/*
		 * pass 4 - anything we dont understand
		 */
		for(dp=dp0; dp<mdp; dp+=p->nfield) {
			if(strcmp(da[0], dp[0]))
				break;
			if(!typeap(dp))
			if(!typeapr(dp))
			if(!typesid(dp))
			if(!typeign(dp)) {
				Bprint(bout, "# ERR-UNKN-%s ", dp[2]);
				aprpr(da, dp);
			}
		}
		break;
	}
}

char**
lookup(Db *d, char *s)
{
	int c, m, t;
	char **p, **b;

	c = d->nrec;
	b = d->data;

loop:
	if(c <= 0)
		return nil;
	m = c/2;
	p = b + m*d->nfield;
	t = strcmp(s, p[0]);
	if(t <= 0) {
		if(t) {
			c = m;
			goto loop;
		}
		while(p > d->data) {
			b = p - d->nfield;
			t = strcmp(s, b[0]);
			if(t)
				break;
			p = b;
		}
		return p;
	}
	b = p+d->nfield;
	c = c-m-1;
	goto loop;
}
