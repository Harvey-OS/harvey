#include <u.h>
#include <libc.h>
#include <cda/fizz.h>

typedef struct Line {
	float length;
	struct Line *connect;
	int inf;
	Wire *wire;
} Line;

struct Family {
	char *name;
	float device_cap;
	float rise_time;
} families[] = {
	{"DEFAULT", 3.5, 6.0}, /* default position 0 is sacred */
	{"H", 3.5, 3.0}, {"L", 4.0, 20.0}, {"N", 3.5, 10.0},
	{"LS", 4.0, 6.0}, {"ALS", 4.0, 6.0}, , {"S", 4.0, 3.0},
	{"F", 4.0, 2.0},
	{"ACT", 4.0, 2.0}, {"BCT", 4.0, 2.0}, {"HC", 9.0, 8.0}, {"C", 4.0, 20.0},
	{0, 0.0, 0.0}
};

#define square(x) ((x) * (x))
#define max(x, y) ((x > y) ? (x) : (y))
#define AWG(g) (0.324883 * exp(-0.115947 * (g)))

void prundef(Chip *c);
void prwires(Signal *s);
void prsignal(Signal *s);
void calclength(Signal *s);
void insertpins(Signal *s);
void pinstat(void);
void scstat(Chip *c);
void scmap(void);
void cksignal(Signal *s);
float chase(Line *l, int level);
float device_cap(Signal *s);
float get_rise(char *);
void f_major(char *, ...);
void f_minor(char *, ...);

/* velocity of light in freespace in mils per picosecond (YES!) */
#define C 11.8029

int dolength = 0;
int quiet = 0;
float Z0 = 55.0;
float epsilon = 5.0;
float width = 0.00630452; /* AWG(34) */
float C0, L0;
float konstant = 2.0;
Board b;

void
main(int argc, char **argv)
{
	int n;
	extern long nph;
	int doundef = 0;
	int dow = 0;
	int dosignal = 0;
	float ratio;
	extern int optind;
	extern char *optarg;
	extern Signal *maxsig;

	while((n = getopt(argc, argv, "lqsuvz:adkw:c:")) != -1)
		switch(n)
		{
		case 'k':	konstant = atof(optarg); break;
		case 'a':	width = AWG(atof(optarg)); break;
		case 'd':	epsilon = atof(optarg); break;
		case 'l':	dolength = 1; break;
		case 'q':	quiet = 1; break;
		case 's':	dosignal = 1; break;
		case 'u':	doundef = 1; break;
		case 'v':	dow = 1; break;
		case 'w':	width = atof(optarg); break;
		case 'z':	Z0 = atof(optarg); break;
		case '?':	break;
		}
	fizzinit();
	f_init(&b);
	if(optind == argc)
		argv[--optind] = "/dev/stdin";
	for(; optind < argc; optind++)
		if(n = f_crack(argv[optind], &b)){
			fprint(1, "%s: %d errors\n", argv[optind], n);
			exit(1);
		}
	if(b.name)
		fprint(1, "Board %s:\n", b.name);
	else
		fprint(1, "Warning: no board name\n");
	if(n = fizzplace()){
		fprint(1, "%d chips unplaced\n", n);
		if(doundef)
			symtraverse(S_CHIP, prundef);
		exit(1);
	}
	cutout(&b);
	fizzplane(&b);
	if (fizzprewrap())
		exit(1);
	if (dow) symtraverse(S_SIGNAL, prwires);
	symtraverse(S_SIGNAL, calclength);
	if (dosignal) symtraverse(S_SIGNAL, prsignal);
	pininit();
	symtraverse(S_SIGNAL, insertpins);
/*
 * ratio is 2 * h / d from Shibata & Ryuiti;
 * L0 is from Terman's classic, but neglects skin effect (in microhenries / ft)
 */
	ratio = exp((Z0 - 17.08) / 34.83);
	L0 = 0.1404 * log10(2.0 * ratio) * 83.3333;
	C0 = L0 / square(Z0);
fprint(2, "Capacitance=%f, inductance=%f, ns/ft=%f\n", C0, L0, 12.0*sqrt(C0*L0));
	symtraverse(S_SIGNAL, cksignal);
	exit(0);
}

void
prundef(Chip *c)
{
	if(c->flags&UNPLACED)
		fprint(1, "%s\n", c->name);
}

void
prwires(Signal *s)
{
	int i, wc;
	Wire *w;
	fprint(1, "Signal %s\n", s->name);
	for (w = s->wires, wc = 1; w; w = w->next, wc++) {
		fprint(1, "\tWire %d:\n", wc);
		for(i = 0; i < w->ninf; i++)
			fprint(1, "\t\tW[%d] %4d/%4d\n", i, w->inflections[i].p.x, w->inflections[i].p.y);
	}
}

float Euclidian(Point p1, Point p2)
{
	return(sqrt(square(p1.x - p2.x) + square(p1.y - p2.y)));
}

void
calclength(Signal *s)
{
	int i, wc;
	float length = 0.0;
	Point lastPt;
	Wire *w;
	for (w = s->wires, wc = 1; w; w = w->next, wc++) {
		lastPt = w->inflections[0].p;
		for(i = 1; i < w->ninf; i++) {
				length += Euclidian(lastPt, w->inflections[i].p);
				lastPt = w->inflections[i].p;
		}
		w->length = length;
		if (dolength) fprint(1, "%s.%d: %f\n", s->name, wc, length);
	}
}

int
pteq(Point p1, Point p2)
{
	return(p1.x == p2.x && p1.y == p2.y);
}

void
prsignal(Signal *s)
{
	int i, j, pin, wc;
	Wire *w;
	Chip *c;

	if((s->type != NORMSIG) || (s->n == 0))
		return;
	fprint(1, "Signal %s:\n", s->name);
	for (w = s->wires, wc=1; w; w = w->next, wc++) {
		fprint(1, "\tWire %d\n", wc);
		for(i = 0; i < w->ninf; i++)
			for(j=0; j<s->n; j++)
				if (pteq(w->inflections[i].p, s->pins[j].p))
					if ((c = (Chip *) symlook(s->coords[j].chip, S_CHIP, (void *) 0)) == 0)
						f_major("chip %s not found", s->coords[j].chip);
					else
						fprint(1, "\t\t%C, %c\n", s->coords[j], c->type->tt[s->coords[j].pin-1]);
	}
}

void
insertpins(Signal *s)
{
	int i, j;
	Wire *w;
	Chip *c;
	Line *l, *ll;

	if((s->type != NORMSIG) || (s->n == 0))
		return;
	for (w = s->wires; w; w = w->next)
	    for(i = 0; i < w->ninf; i++)
		for(j=0; j<s->n; j++)
		    if (pteq(w->inflections[i].p, s->pins[j].p))
			if ((c = (Chip *) symlook(s->coords[j].chip, S_CHIP, (void *) 0)) == 0)
				f_major("chip %s not found", s->coords[j].chip);
			else {
				l = (Line *) f_malloc(sizeof(Line));
				l -> wire = w;
				l -> inf = i;
				if (ll = (Line *) pinlook(XY(s->pins[j].p.x, s->pins[j].p.y), 0)) {
				    l -> connect = ll -> connect;
				    ll -> connect = l;
				}
				else
				    pinlook(XY(s->pins[j].p.x, s->pins[j].p.y), (long) l); /* sinful... */
			}
}

int
isdriver(char tt)
{
	return(((tt >= '0' && tt <= '6') || (tt == 'D') || (tt == 'd')) ? 1 : 0);
}

void
cksignal(Signal *s)
{
	int i, j, pin, wc;
	float length, cap, rise_time, prop_delay, ok_length, delay_per;
	Wire *w;
	Chip *c;
	Line *l;

	if((s->type != NORMSIG) || (s->n == 0))
		return;
	cap = device_cap(s);
	for (w = s->wires; w; w = w->next)
	    for(i = 0; i < w->ninf; i++)
		for(j=0; j<s->n; j++)
		    if (pteq(w->inflections[i].p, s->pins[j].p))
			if ((c = (Chip *) symlook(s->coords[j].chip, S_CHIP, (void *) 0)) == 0)
				f_major("chip %s not found", s->coords[j].chip);
			else
			if (isdriver(c->type->tt[s->coords[j].pin-1])) {
				rise_time = get_rise(c->type->family);
				l = (Line *) pinlook(XY(s->pins[j].p.x, s->pins[j].p.y), 0);
				length = chase(l, 0);
				prop_delay = sqrt(L0 * length * (C0 * length + cap));
				delay_per = sqrt(L0 * (C0 + cap / length));
				ok_length = (rise_time * 1000.0) / (konstant * delay_per);
				if (length > ok_length)
fprint(1, "%s (%C): %f mils > %f mils, z = %f\n", s->name, s->coords[j], length, ok_length, sqrt((L0 * length) / (C0 * length + cap)));
/***fprint(1, "%s : %C = %f > %f, %f, %f, %f = %f\n", s -> name, s->coords[j], length, ok_length, L0 * length, C0 * length, cap, prop_delay); ***/
			}
}

float chase(Line *l, int level)
{
	Line *connecting;
	Wire *w;
	int ip;
	float l_length, r_length, max_length;
	Point lastPt;
	if (!l) return(0.0);
	if (l -> length > 0.0) return(l -> length);
	max_length = 0.0;
	for(; l; l = l->connect) {
		l_length = r_length = 0.0;
		w = l->wire;
		lastPt = w->inflections[0].p;
		for(ip=0; ip<w->ninf; ip++) {
			if (ip <= l->inf)
				l_length += Euclidian(lastPt, w->inflections[ip].p);
			else
				r_length += Euclidian(lastPt, w->inflections[ip].p);
			lastPt = w->inflections[ip].p;
		}
		l -> length = max(l_length, r_length);
		l_length = r_length = 0.0;
		for(ip=0; ip<w->ninf; ip++)
		    if (ip != l->inf)
			for (connecting = (Line *) pinlook(XY(w->inflections[ip].p.x, w->inflections[ip].p.y), 0);
			connecting; connecting = connecting -> connect)
			    if (connecting -> wire != w)
			        r_length = max(r_length, chase(connecting, level+1));
		l -> length += r_length;
		max_length = max(max_length, l -> length);
	}
	return(max_length);
}

float
device_cap(Signal *s)
{
	int i, j;
	char *family;
	float cap, family_cap;
	Chip *c;
	cap = 0.0;
	for(j=0; j<s->n; j++)
		if ((c = (Chip *) symlook(s->coords[j].chip, S_CHIP, (void *) 0)) == 0)
			f_major("chip %s not found", s->coords[j].chip);
		else {
			family = c -> type -> family;
			if (!family) {
				if (!quiet) f_minor("chip %s doesn't have a family", s->coords[j].chip);
				c -> type -> family = families[0].name;
				cap += families[0].device_cap;
			}
			else {
				family_cap = 0.0;
				for(i=0; families[i].name; i++)
					if (strcmp(family, families[i].name) == 0)
						family_cap = families[i].device_cap;
				if (family_cap < 1.0)
					f_minor("family %s not found for chip %s", family, s->coords[j].chip);
				else cap += family_cap;
			}
		}
	return(cap);
}

float
get_rise(char *family)
{
	int i;
	for(i=0; families[i].name; i++)
		if (strcmp(family, families[i].name) == 0)
			return(families[i].rise_time);
	f_major("rise time not found for %s", family);
	return(families[0].rise_time);
}
