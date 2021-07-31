#include	<u.h>
#include	<libc.h>
#include	<cda/fizz.h>

#define		RAD(x)	(drdiam(x)/2)

#define	NPKG	32

static int	drdiam(int);
static void	dorect(Rectangle);
static void	dogrid(Rectangle);
void	picpr(Package*);

int	bflag;
int	gflag;

char *	pkgnames[NPKG];
int	npkg;

void
main(int argc, char **argv)
{
	Board b;
	int n;
	char *fname;

	ARGBEGIN{
	case 'b':
		++bflag;	/* show bottom view */
		break;
	case 'g':
		++gflag;	/* show 100 mil grid */
		break;
	case 'p':
		if(npkg < NPKG)
			pkgnames[npkg++] = ARGF();
		break;
	default:
		fprint(2, "usage: %s [-b] [-p name] [files...]\n", argv0);
		exits("usage");
	}ARGEND
	f_init(&b);
	if(*argv)
		fname = *argv++;
	else
		fname = "/fd/0";
	do{
		if(n = f_crack(fname, &b)){
			fprint(2, "%s: %d errors\n", fname, n);
			exits("errors");
		}
	}while(fname = *argv++);	/* assign = */
	symtraverse(S_PACKAGE, picpr);
	exits(0);
}

static char *dir = "nesw";
static char *adj[] = { "above", "ljust", "below", "rjust" };

void
picpr(Package *p)
{
	int i, orient;
	Plane *pl;

	if(npkg){
		for(i=0; i<npkg; i++)
			if(strcmp(p->name, pkgnames[i]) == 0)
				break;
		if(i >= npkg)
			return;
	}

	fprint(1, ".sp 2i\n");
	fprint(1, ".PS 6.0\n");
	fprint(1, "boxwid=20\n");
	fprint(1, "boxht=20\n");
	fprint(1, "define pin X\n");
	fprint(1, "box invis \"\\(bu\" at $1,$2\n");
	fprint(1, "\"\\s-2$3\\s0\" $4 at last box.$5\n");
	fprint(1, "X\n");
	fprint(1, "line from -30,0 to 30,0\n");
	fprint(1, "line from 0,-30 to 0,30\n");
	if (!bflag)
		fprint(1, "\"%s\" at %d,%d\n",
		p->name, (p->r.min.x+p->r.max.x)/2, p->r.max.y+100);
	else
		fprint(1, "\"%s\" \"(bottom)\" at %d,%d\n",
		p->name, -(p->r.min.x+p->r.max.x)/2, p->r.max.y+100);
	dorect(p->r);
	if(gflag)
		dogrid(p->r);
	for(i = 0; i < p->npins; i++){
		orient = 3;	/* west for now */
		if (!bflag)
			fprint(1, "pin(%d,%d,%d,%s,%c)\n",
			p->pins[i].p.x, p->pins[i].p.y,
			i+p->pmin, adj[orient], dir[orient]);
		else
			fprint(1, "pin(%d,%d,%d,%s,%c)\n",
			-p->pins[i].p.x, p->pins[i].p.y,
			i+p->pmin, adj[orient], dir[orient]);
	}
	for(i=0; i < p->ndrills; i++){
		fprint(1, "circlerad=%d\n", RAD(p->drills[i].drill));
		if (!bflag)
			fprint(1, "circle at %d,%d\n",
			p->drills[i].p.x, p->drills[i].p.y);
		else
			fprint(1, "circle at %d,%d\n",
			-p->drills[i].p.x, p->drills[i].p.y);
	}
	for(i=0; i < p->ncutouts; i++){
		pl = &p->cutouts[i];
		dorect(pl->r);
	}
	fprint(1, ".PE\n");
}

static int
drdiam(int c)
{
	static int drills[] = {
		'A', 33,
		'B', 34,
		'C', 39,
		'D', 42,
		'E', 50,
		'F', 62,
		'G', 106,
		'H', 107,
		'I', 108,
		'J', 20,
		'K', 110,
		'L', 111,
		'M', 112,
		'N', 113,
		'O', 114,
		'P', 115,
		'Q', 116,
		'R', 117,
		'S', 118,
		'T', 119,
		'U', 100,
		'V', 20,
		'W', 122,
		'X', 123,
		'Y', 124,
		'Z', 125,
		0, 0
	};
	int i;
	for (i=0; drills[i]; i+=2)
		if (c == drills[i])
			return drills[i+1];
	return 0;
}

static void
dorect(Rectangle r)
{
	if (!bflag)
		fprint(1, "line from %d,%d to %d,%d to %d,%d to %d,%d to %d,%d\n",
		r.min.x, r.min.y, r.max.x, r.min.y,
		r.max.x, r.max.y, r.min.x, r.max.y,
		r.min.x, r.min.y);
	else
		fprint(1, "line from %d,%d to %d,%d to %d,%d to %d,%d to %d,%d\n",
		-r.min.x, r.min.y, -r.max.x, r.min.y,
		-r.max.x, r.max.y, -r.min.x, r.max.y,
		-r.min.x, r.min.y);
}

static int
roundout(int n, int r)
{
	int rn;

	rn = (n >= 0) ? n : -n;
	rn += r-1;
	rn /= r;
	rn *= r;
	return (n >= 0) ? rn : -rn;
}

static void
dogrid(Rectangle r)
{
	int t;

	r.min.x = roundout(r.min.x, 100);
	r.min.y = roundout(r.min.y, 100);
	r.max.x = roundout(r.max.x, 100);
	r.max.y = roundout(r.max.y, 100);
	if(bflag){
		r.min.x = -r.min.x;
		r.max.x = -r.max.x;
	}
	if(r.min.x >= r.max.x){
		t = r.min.x;
		r.min.x = r.max.x;
		r.max.x = t;
	}
	if(r.min.y >= r.max.y){
		t = r.min.y;
		r.min.y = r.max.y;
		r.max.y = t;
	}
	for(t=r.min.x; t<=r.max.x; t+=100)
		fprint(1, "line dotted 10 from %d,%d to %d,%d\n",
			t, r.min.y, t, r.max.y);
	for(t=r.min.y; t<=r.max.y; t+=100)
		fprint(1, "line dotted 10 from %d,%d to %d,%d\n",
			r.min.x, t, r.max.x, t);
}
