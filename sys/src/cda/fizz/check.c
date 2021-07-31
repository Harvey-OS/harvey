#include <u.h>
#include	<libc.h>
#include	<cda/fizz.h>

void prundef(Chip *c);
void chkpins(Chip *c);
void setup(void);
void prpkg(Package *p);
void prchip(Chip *c);
static int pcmp(Pin *a, Pin *b);
void chkpkgpins(Package *p);
Board b;

void
main(int argc, char **argv)
{
	int n;
	extern long nph;
	int doundef = 0;
	int dow = 0;
	char *chname = 0;
	char *pkname = 0;
	extern int optind;
	extern char *optarg;
	extern Signal *maxsig;

	while((n = getopt(argc, argv, "uwp:c:")) != -1)
		switch(n)
		{
		case 'u':	doundef = 1; break;
		case 'w':	dow = 1; break;
		case 'c':	chname = optarg; break;
		case 'p':	pkname = optarg; break;
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
	if(pkname)
		prpkg((Package *)symlook(pkname, S_PACKAGE, (void *)0));
	if(chname)
		prchip((Chip *)symlook(chname, S_CHIP, (void *)0));
	if(dow){
		if(fizzprewrap())
			exit(1);
		symtraverse(S_SIGNAL, netlen);
		if(maxsig && ((maxsig->type & VSIG) != VSIG) && (maxsig->n >= MAXNET)){
			fprint(1, "net %s is too big (%d>=%d)\n", maxsig->name,
				maxsig->n, MAXNET);
			exit(1);
		}
		setup();
		symtraverse(S_CHIP, chkpins);
	}
	symtraverse(S_PACKAGE, chkpkgpins);
	exit(0);
}

void
prundef(Chip *c)
{
	if(c->flags&UNPLACED)
		fprint(1, "%s\n", c->name);
}

void
chkpins(Chip *c)
{
	register i, j;
	long bestd;
	short bpin;

	if(c->type->tt[0] == 0)
		return;
	for(i = 0; i < c->npins; i++)
		if(j = pinlook(XY(c->pins[i].p.x, c->pins[i].p.y), 0))
			if(c->type->tt[i] != ttnames[j-1]){
				nnprep(b.v[j-1].pins, b.v[j-1].npins);
				nn(c->pins[i].p, &bestd, &bpin);
				if(bestd)
					fprint(1, "*****check nn ERK!! get help %ld : %d\n", bestd, bpin);
				fprint(1, "%s.%d (tt=%c) coincides with %s.%d at %p\n",
					c->name, i+c->pmin, c->type->tt[i],
					b.v[j-1].name, bpin, c->pins[i].p);
			}
}

void
setup(void)
{
	register i, j;
	register Pin *p;

	pininit();
	for(i = 0; i < 6; i++){
		if(b.v[i].npins == 0) continue;
		for(j = b.v[i].npins, p = b.v[i].pins; j > 0; j--, p++)
			pinlook(XY(p->p.x, p->p.y), i+1);
	}
}

void
prchip(Chip *c)
{
	register i;

	if(c == 0)
		return;
	fprint(1, "Chip %s: type='%s' pt=%p r=%r pins=%d drills=%d\n", c->name,
		c->typename, c->pt, c->r, c->npins, c->ndrills);
	for(i = 0; i < c->npins; i++)
		fprint(1, "[%d] %p.%c %c\n", i+c->pmin, c->pins[i].p,
			c->pins[i].drill, c->type->tt[0]? c->type->tt[i]:'?');
	for(i = 0; i < c->ndrills; i++)
		fprint(1, "D[%d] %p.%c\n", i+c->pmin, c->drills[i].p,
			c->drills[i].drill);
}

void
prpkg(Package *p)
{
	register i;

	if(p == 0)
		return;
	fprint(1, "Package %s: pins=%d,%d r=%r pins=%d drills=%d\n", p->name,
		p->pmin, p->pmax, p->r, p->npins, p->ndrills);
	for(i = 0; i < p->npins; i++)
		fprint(1, "[%d] %p.%c\n", i+p->pmin, p->pins[i].p,
			p->pins[i].drill);
	for(i = 0; i < p->ndrills; i++)
		fprint(1, "D[%d] %p.%c\n", i+p->pmin, p->drills[i].p,
			p->drills[i].drill);
}

static
pcmp(Pin *a, Pin *b)
{
	register k;

	if(k = a->p.x - b->p.x)
		return(k);
	return(a->p.y - b->p.y);
}

void
chkpkgpins(Package *p)
{
	register Pin *p1, *p2;

if(p->npins <= 0) return;
	qsort((char *)p->pins, p->npins, sizeof(Pin), pcmp);
	for(p2 = p->pins+p->npins-1, p1 = p2-1; p1 >= p->pins; p1--, p2--)
		if(pcmp(p1, p2) == 0)
			fprint(1, "Package %s: point %p occurs more than once\n",
				p->name, p1->p);
}
