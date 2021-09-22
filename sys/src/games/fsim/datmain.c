#include	"type.h"

void	datcount(char*, int, int, int, int, double, double);
void	datprint(char*, int, int, int, int, double, double);
void	doall(void (*)(char*, int, int, int, int, double, double));
void	datapt(void (*)(char*, int, int, int, int, double, double));
void	datils(void (*)(char*, int, int, int, int, double, double));
void	datvor(void (*)(char*, int, int, int, int, double, double));

int	cele(int);
int	cfreq(int, int);
ulong	clatlng(double);
Dapt*	datlook(char*);
void	dme(double, double, int, int, int);
int	acmp(void*, void*);

static	double	alat;		// side effect answers
static	double	alng;
static	int	napt;
static	int	ndat;
static	int	pass;

static char*
nn(char *name)
{
	static char t[100];
	sprint(t, "\"%s\",", name);
	return t;
}

static double
lt(ulong lat)
{
	double l;

	l = (double)lat/2147483648. * 180.;
	if(l >= 180)
		l -= 360;
	return l;
}

static double
lg(ulong lat)
{
	double l;

	l = (double)lat/2147483648. * 180.;
	if(l >= 180)
		l -= 360;
	return l;
}

static void
addf(char *t, char *f)
{
	if(*t != 0)
		strcat(t, "+");
	strcat(t, f);
}

static char*
fg(int flag)
{
	static char t[100];

	t[0] = 0;
	if(flag & VOR) addf(t, "VOR");
	if(flag & NDB) addf(t, "NDB");
	if(flag & ILS) addf(t, "ILS");
	if(flag & GS) addf(t, "GS");
	if(flag & OM) addf(t, "OM");
	if(flag & MM) addf(t, "MM");
	if(flag & IM) addf(t, "IM");
	if(flag & DME) addf(t, "DME");
	if(flag & APT) addf(t, "APT");
	if(flag & DATA) addf(t, "DATA");
	return t;
}

static int
fq(int flag, int freq)
{

	if(flag & (DME|VOR|ILS))
		freq += 10000;
	return freq;
}

void
datmain(void)
{
	Nav *p, *q;

	for(napt=0; dapt[napt].name != nil; napt++)
		;
	qsort(dapt, napt, sizeof(*dapt), acmp);

	pass = 0;
	ndat = 0;
	doall(datcount);
	ndat++;

	nav = malloc(ndat * sizeof(*nav));
	if(nav == nil) {
		fprint(2, "malloc nav\n");
		exits("malloc");
	}
	memset(nav, 0, ndat * sizeof(*nav));

	pass = 1;
	ndat = 0;
	doall(datprint);
	nav[ndat].name = nil;

	if(proxflag) {
		for(p=nav; p->name; p++) {
			if(p->freq == 0 || p->freq == 75)
				continue;
			plane.lat = p->lat;
			plane.lng = p->lng;
			plane.coslat = cos(plane.lat * C1);
			for(q=p+1; q->name; q++) {
				if(p->freq == q->freq && (p->flag&q->flag) != 0) {
					if(fdist(q) < MILE(20)) {
						fprint(2, "*** %s/%s ***\n", p->name, q->name);
						if((p->flag&DATA) == 0 && (q->flag&DATA) != 0)
							q->flag = 0;
						if((q->flag&DATA) == 0 && (p->flag&DATA) != 0)
							p->flag = 0;
						if((q->flag&DATA) != 0 && (p->flag&DATA) != 0)
							q->flag = 0;
					}
				}
			}
		}
		for(p=nav; p->name; p++) {
			if(p->flag & DATA)
				print("	%-10s %10.4f, %10.4f, %s, %d, %d,\n",
					nn(p->name),
					lt(p->lat), lg(p->lng),
					fg(p->flag), fq(p->flag, p->freq), p->z);
		}
		exits(0);
	}
}

int
acmp(void *aa, void *ab)
{
	Dapt *a, *b;

	a = aa;
	b = ab;
	return strcmp(a->name, b->name);
}

void
datcount(char*, int, int, int, int, double, double)
{
	ndat++;
}

void
datprint(char *name, int flag, int obs,
	int freq, int ele, double lat, double lng)
{
	Nav *n;

	n = nav + ndat;
	ndat++;

	n->name = name;
	n->flag = flag;
	n->obs = obs;
	n->z = ele;
	n->freq = freq;
	n->lat = clatlng(lat);
	n->lng = clatlng(lng);
}

void
doall(void (*f)(char*, int, int, int, int, double, double))
{
	datvor(f);
	datapt(f);
	datils(f);
}

void
datvor(void (*f)(char*, int, int, int, int, double, double))
{
	Dvor *p;

	for(p=dvor; p->name; p++) {
		f(
			p->name,
			p->flag,
			0,
			cfreq(p->freq, p->flag),
			cele(p->elev),
			p->lat,
			p->lng
		);
	}
}

void
datapt(void (*f)(char*, int, int, int, int, double, double))
{
	Dapt *p;
	int mv;

	for(p=dapt; p->name; p++) {
		mv = p->magv;
		if(mv < 0)
			mv = 3600 + mv;
		f(
			p->name,
			APT,
			mv,
			0,
			cele(p->elev),
			p->lat,
			p->lng
		);
	}
}

void
prepname(int alg, char *d, char *s)
{
	int c;

	switch(alg) {
	case 1:
		*d++ = 'K';
		break;
	}

	while(c = *s++) {
		if(c >= 'a' && c <= 'z')
			c += 'A'-'a';
		switch(alg) {
		default:
			*d++ = c;
			break;
		}
	}
	*d = 0;
}

void
datils(void (*f)(char*, int, int, int, int, double, double))
{
	Dils *p;
	Dapt *q;
	int h;
	char name[100];

	for(p=dils; p->name; p++) {
		prepname(1, name, p->name);
		q = datlook(name);
		if(q == nil) {
			prepname(2, name, p->name);
			q = datlook(name);
		}
		if(q == nil) {
			prepname(3, name, p->name);
			q = datlook(name);
		}
		if(q == nil) {
			if(pass == 0)
				fprint(2, "oops %s\n", p->name);
			continue;
		}
		h = p->hdg;
		if(p->freq) {
			dme(q->lat, q->lng, h, q->magv, 5);
			f(
				q->name,
				(p->dme == 5)? ILS+DME: ILS,
				p->hdg,
				cfreq(p->freq, VOR),
				cele(q->elev),
				alat,
				alng
			);
			if(p->dme != 0 && p->dme != 5) {
				dme(q->lat, q->lng, h, q->magv, p->dme);
				f(
					q->name,
					DME,
					0,
					cfreq(p->freq, VOR),
					cele(q->elev),
					alat,
					alng
				);
			}
		}

		h += 180;	// everything else is outbound
		if(p->gs) {
			/* should this be folded into ILS entry? */
			dme(q->lat, q->lng, h, q->magv, 5);
			f(
				q->name,
				GS,
				p->gs,
				cfreq(p->freq, VOR),
				cele(q->elev),
				alat,
				alng
			);
		}

		if((p->om && p->mm && p->om <= p->mm) ||
		   (p->om && p->im && p->om <= p->im) ||
		   (p->mm && p->im && p->mm <= p->im))
			fprint(2, "%s: marker\n", q->name);

		if(p->lom && !p->om && !p->mm)
			fprint(2, "%s: lom without om\n", q->name);

		if(p->om) {
			dme(q->lat, q->lng, h, q->magv, p->om+5);
			if(p->lom)
				f(
					q->name,
					(p->lom>0)? OM+NDB: NDB,
					0,
					cfreq(p->lom, NDB),
					cele(q->elev),
					alat,
					alng
				);
			else
				f(
					q->name,
					OM,
					0,
					75,
					cele(q->elev),
					alat,
					alng
				);
		}
		if(p->mm) {
			dme(q->lat, q->lng, h, q->magv, p->mm+5);
			f(
				q->name,
				MM,
				0,
				75,
				cele(q->elev),
				alat,
				alng
			);
		}
		if(p->im) {
			dme(q->lat, q->lng, h, q->magv, p->im+5);
			f(
				q->name,
				IM,
				0,
				75,
				cele(q->elev),
				alat,
				alng
			);
		}
	}
}

Dapt*
datlook(char *s)
{
	Dapt *p, *b;
	int i, m, n;

	b = dapt;
	n = napt;

loop:
	if(n <= 0)
		return nil;
	m = n / 2;
	p = b + m;
	i = strcmp(s, p->name);
	if(i <= 0) {
		if(i == 0)
			return p;
		n = m;
		goto loop;
	}
	b = p+1;
	n = n-m-1;
	goto loop;
}

int
cele(int n)
{

	if(n <= 0)
		return(1);
	return n;
}

int
cfreq(int freq, int flag)
{

	if(freq < 0)
		freq = -freq;
	if(flag & (DME|VOR|APT))
		return freq - 10000;
	if(flag & NDB)
		return freq;
	fprint(2, "cfreq: unk %d\n", flag);
	return 0;
}

ulong
clatlng(double ang)
{
	ulong ll;

	if(ang < 0)
		ang = 360. + ang;
	ang = ang/360.;
	while(ang >= 1.)
		ang -= 1.;
	while(ang < 0.)
		ang += 1.;
	ll = ang * 2147483648.;	// 2^31 BOTCH sb 2^32
	return ll+ll;
}

#define	C4	((180.*.1)/(CP*CR))

void
dme(double lat, double lng, int hdg, int mgv, int dist)
{
	double d, a;

	d = dist * C4;
	a = (hdg + mgv*.1) * (CP / 180.);
	alat = lat + d * cos(a);
	alng = lng - d * sin(a) / cos(lat*CP/180.);
}
