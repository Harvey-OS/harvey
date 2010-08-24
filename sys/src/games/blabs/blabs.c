#include	<u.h>
#include	<libc.h>
#include	<draw.h>

#define	NPJW		48
#define	NSPLIT		6
#define	NDOT		14
#define	MAXTRACKS	128
#define	MINV		6
#define	PDUP		1		/* useful for debugging */

#define	nels(a)		(sizeof(a) / sizeof(a[0]))
#define	imin(a,b)	(((a) <= (b)) ? (a) : (b))
#define	imax(a,b)	(((a) >= (b)) ? (a) : (b))
#define	sqr(x)		((x) * (x))

typedef struct vector	vector;
struct vector
{
	double	x;
	double	y;
};

typedef struct dot	Dot;
struct dot
{
	Point	pos;
	Point	ivel;
	vector	vel;
	double	mass;
	int	charge;
	int	height;	/* precalculated for speed */
	int	width;	/* precalculated for speed */
	int	facei;
	int	spin;
	Image	*face;
	Image	*faces[4];
	Image	*mask;
	Image	*masks[4];
	Image	*save;
	int	ntracks;
	Point	track[MAXTRACKS];
	int	next_clear;
	int	next_write;
};

Dot	dot[NDOT];
Dot	*edot		= &dot[NDOT];
int	total_spin	= 3;
vector	no_gravity;
vector	gravity		=
{
	0.0,
	1.0,
};

/* static Image	*track; */
static Image	*im;
static int	track_length;
static int	track_width;
static int	iflag;
static double	k_floor		= 0.9;
Image *screen;

#include "andrew.bits"
#include "bart.bits"
#include "bwk.bits"
#include "dmr.bits"
#include "doug.bits"
#include "gerard.bits"
#include "howard.bits"
#include "ken.bits"
#include "philw.bits"
#include "pjw.bits"
#include "presotto.bits"
#include "rob.bits"
#include "sean.bits"
#include "td.bits"

Image*
eallocimage(Display *d, Rectangle r, ulong chan, int repl, int col)
{
	Image *i;
	i = allocimage(d, r, chan, repl, col);
	if(i == nil) {
		fprint(2, "cannot allocate image\n");
		exits("allocimage");
	}
	return i;
}

/*
 * p1 dot p2
 *	inner product
 */
static
double
dotprod(vector *p1, vector *p2)
{
	return p1->x * p2->x + p1->y * p2->y;
}

/*
 * r = p1 + p2
 */
static
void
vecaddpt(vector *r, vector *p1, vector *p2)
{
	r->x = p1->x + p2->x;
	r->y = p1->y + p2->y;
}

/*
 * r = p1 - p2
 */
static
void
vecsub(vector *r, vector *p1, vector *p2)
{
	r->x = p1->x - p2->x;
	r->y = p1->y - p2->y;
}

/*
 * r = v * s
 *	multiplication of a vector by a scalar
 */
static
void
scalmult(vector *r, vector *v, double s)
{
	r->x = v->x * s;
	r->y = v->y * s;
}

static
double
separation(Dot *p, Dot *q)
{
	return sqrt((double)(sqr(q->pos.x - p->pos.x) + sqr(q->pos.y - p->pos.y)));
}

static
int
close_enough(Dot *p, Dot *q, double *s)
{
	int	sepx;
	int	width;
	int	sepy;

	sepx = p->pos.x - q->pos.x;
	width = p->width;

	if (sepx < -width || sepx > width)
		return 0;

	sepy = p->pos.y - q->pos.y;

	if (sepy < -width || sepy > width)
		return 0;

	if ((*s = separation(p, q)) > (double)width)
		return 0;

	return 1;
}

static
void
ptov(vector *v, Point *p)
{
	v->x = (double)p->x;
	v->y = (double)p->y;
}

static
void
vtop(Point *p, vector *v)
{
	p->x = (int)(v->x + 0.5);
	p->y = (int)(v->y + 0.5);
}

static
void
exchange_spin(Dot *p, Dot *q)
{
	int	tspin;

	if (p->spin == 0 && q->spin == 0)
		return;

	tspin = p->spin + q->spin;

	p->spin = imax(nrand(imin(3, tspin + 1)), tspin + 1 - 3);

	if (p->spin == 0)
	{
		p->face = p->faces[0];
		p->mask = p->masks[0];
	}

	q->spin = tspin - p->spin;

	if (q->spin == 0)
	{
		q->face = q->faces[0];
		q->mask = q->masks[0];
	}
}

static
void
exchange_charge(Dot *p, Dot *q)
{
	if (p->charge == 0 && q->charge == 0)
	{
		switch (nrand(16))
		{
		case 0:
			p->charge = 1;
			q->charge = -1;
			break;

		case 1:
			p->charge = -1;
			q->charge = 1;
			break;
		}
	}
}

/*
 * The aim is to completely determine the final
 * velocities of the two interacting particles as
 * a function of their initial velocities, masses
 * and point of collision.  We start with the two
 * quantities that we know are conserved in
 * elastic collisions -- momentum and kinetic energy.
 * 
 * Let the masses of the particles be m1 and m2;
 * their initial velocities V1 and V2 (vector quantities
 * represented by upper case variables, scalars lower
 * case); their final velocities V1' and V2'.
 * 
 * Conservation of momentum gives us:
 * 
 * 	(1)	m1 * V1 + m2 * V2	= m1 * V1' + m2 * V2'
 * 
 * and conservation of kinetic energy gives:
 * 
 * 	(2)	1/2 * m1 * |V1| * |V1| + 1/2 * m2 * |V2| * |V2|	=
 * 			1/2 * m1 * |V1'| * |V1'| + 1/2 * m2 * |V2'| * |V2'|
 * 
 * Then, decomposing (1) into its 2D scalar components:
 * 
 * 	(1a)	m1 * v1x + m2 * v2x	= m1 * v1x' + m2 * v2x'
 * 	(1b)	m1 * v1y + m2 * v2y	= m1 * v1y' + m2 * v2y'
 * 
 * and simplifying and expanding (2):
 * 
 * 	(2a)	m1 * (v1x * v1x + v1y * v1y) +
 * 		m2 * (v2x * v2x + v2y * v2y)
 * 					=
 * 		m1 * (v1x' * v1x' + v1y' * v1y') +
 * 		m2 * (v2x' * v2x' + v2y' * v2y')
 * 
 * We know m1, m2, V1 and V2 which leaves four unknowns:
 * 
 * 	v1x', v1y', v2x' and v2y'
 * 
 * but we have just three equations:
 * 
 * 	(1a), (1b) and (2a)
 * 
 * To remove this extra degree of freedom we add the assumption that
 * the forces during the collision act instantaneously and exactly
 * along the (displacement) vector joining the centres of the two objects.
 * 
 * And unfortunately, I've forgotten the extra equation that this
 * adds to the system(!), but it turns out that this extra constraint
 * does allow us to solve the augmented system of equations.
 * (I guess I could reverse engineer it from the code ...)
 */
static
void
rebound(Dot *p, Dot *q, double s)
{
	vector	pposn;
	vector	qposn;
	vector	pvel;
	vector	qvel;
	vector	newqp;	/* vector difference of the positions */
	vector	newqpu;	/* unit vector parallel to newqp */
	vector	newqv;	/*   "        "      "  "   velocities */
	double	projp;	/* projection of p's velocity in newqp direction */
	double	projq;	/*     "       " q's velocity in newqp direction */
	double	pmass;	/* mass of p */
	double	qmass;	/* mass of q */
	double	tmass;	/* sum of mass of p and q */
	double	dmass;	/* difference of mass of p and q */
	double	newp;
	double	newq;

	if (s == 0.0)
		return;

	ptov(&pposn, &p->pos);
	ptov(&qposn, &q->pos);
	pvel = p->vel;
	qvel = q->vel;

	/*
	 * Transform so that p is stationary at (0,0,0);
	 */
	vecsub(&newqp, &qposn, &pposn);
	vecsub(&newqv, &qvel, &pvel);

	scalmult(&newqpu, &newqp, -1.0 / s);

	if (dotprod(&newqv, &newqpu) <= 0.0)
		return;

	projp = dotprod(&pvel, &newqpu);
	projq = dotprod(&qvel, &newqpu);
	pmass = p->mass;
	qmass = q->mass;
	tmass = pmass + qmass;
	dmass = pmass - qmass;
	newp = (2.0 * qmass * projq + dmass * projp) / tmass;
	newq = (2.0 * pmass * projp - dmass * projq) / tmass;
	scalmult(&newqp, &newqpu, projp - newp);
	scalmult(&newqv, &newqpu, projq - newq);
	vecsub(&pvel, &pvel, &newqp);
	vecsub(&qvel, &qvel, &newqv);

	p->vel = pvel;
	vtop(&p->ivel, &p->vel);
	q->vel = qvel;
	vtop(&q->ivel, &q->vel);

	exchange_spin(p, q);

	if (iflag)
		exchange_charge(p, q);
}

static
int
rrand(int lo, int hi)
{
	return lo + nrand(hi + 1 - lo);
}

static
void
drawdot(Dot *d)
{
	Rectangle r;
	char buf[10];

	r = d->face->r;
	r = rectsubpt(r, d->face->r.min);
	r = rectaddpt(r, d->pos);
	r = rectaddpt(r, screen->r.min);
	
	draw(screen, r, d->face, d->mask, d->face->r.min);

	if(PDUP > 1) {	/* assume debugging */
		sprint(buf, "(%d,%d)", d->pos.x, d->pos.y);
		string(screen, r.min, display->black, ZP, font, buf);
	}
}

static
void
undraw(Dot *d)
{
	Rectangle r;
	r = d->face->r;
	r = rectsubpt(r, d->face->r.min);
	r = rectaddpt(r, d->pos);
	r = rectaddpt(r, screen->r.min);
	
	draw(screen, r, display->black, d->mask, d->face->r.min);

/*
	if (track_width > 0)
	{
		if (d->ntracks >= track_length)
		{
			bitblt(&screen, d->track[d->next_clear], track, track->r, D ^ ~S);
			d->next_clear = (d->next_clear + 1) % track_length;
			d->ntracks--;
		}

		d->track[d->next_write] = Pt(d->pos.x + d->width / 2 - track_width / 2, d->pos.y + d->height / 2 - track_width / 2);
		bitblt(&screen, d->track[d->next_write], track, track->r, D ^ ~S);
		d->next_write = (d->next_write + 1) % track_length;
		d->ntracks++;
	}
*/
}

static void
copy(Image *a, Image *b)
{
	if(PDUP==1 || eqrect(a->r, b->r))
		draw(a, a->r, b, nil, b->r.min);
	else {
		int x, y;
		for(x = a->r.min.x; x < a->r.max.x; x++)
			for(y = a->r.min.y; y < a->r.max.y; y++)
				draw(a, Rect(x,y,x+1,y+1), b, nil, Pt(x/PDUP,y/PDUP));
	}
}

static void
transpose(Image *b)
{
	int x, y;

	for(x = b->r.min.x; x < b->r.max.x; x++)
		for(y = b->r.min.y; y < b->r.max.y; y++)
			draw(im, Rect(y,x,y+1,x+1), b, nil, Pt(x,y));

	draw(b, b->r, im, nil, ZP);
}

static void
reflect1_lr(Image *b)
{
	int x;
	for(x = 0; x < PDUP*NPJW; x++)
		draw(im, Rect(x, 0, x, PDUP*NPJW), b, nil, Pt(b->r.max.x-x,0));
	draw(b, b->r, im, nil, ZP);
}

static void
reflect1_ud(Image *b)
{
	int y;
	for(y = 0; y < PDUP*NPJW; y++)
		draw(im, Rect(0, y, PDUP*NPJW, y), b, nil, Pt(0,b->r.max.y-y));
	draw(b, b->r, im, nil, ZP);
}

static
void
rotate_clockwise(Image *b)
{
	transpose(b);
	reflect1_lr(b);
}

static
void
spin(Dot *d)
{
	int	i;

	if (d->spin > 0)
	{
		i = (d->facei + d->spin) % nels(d->faces);
		d->face = d->faces[i];
		d->mask = d->masks[i];
		d->facei = i;
	}
	sleep(0);
}

static
void
restart(Dot *d)
{
	static int	beam;

	d->charge = 0;

	d->pos.x = 0;
	d->vel.x = 20.0 + (double)rrand(-2, 2);

	if (beam ^= 1)
	{
		d->pos.y = screen->r.max.y - d->height;
		d->vel.y = -20.0 + (double)rrand(-2, 2);
	}
	else
	{
		d->pos.y = 0;
		d->vel.y = 20.0 + (double)rrand(-2, 2);
	}

	vtop(&d->ivel, &d->vel);
}

static
void
upd(Dot *d)
{
	Dot	*dd;

	spin(d);

	/*
	 * Bounce off others?
	 */
	for (dd = d + 1; dd != edot; dd++)
	{
		double	s;

		if (close_enough(d, dd, &s))
			rebound(d, dd, s);
	}

	if (iflag)
	{
		/*
		 * Going off-screen?
		 */
		if
		(
			d->pos.x + d->width < 0
			||
			d->pos.x >= Dx(screen->r)
			||
			d->pos.y + d->height < 0
			||
			d->pos.y >= Dy(screen->r)
		)
			restart(d);

		/*
		 * Charge.
		 */
		if (d->charge != 0)
		{
			/*
			 * TODO: This is wrong -- should
			 * generate closed paths.  Can't
			 * simulate effect of B field just
			 * by adding in an orthogonal
			 * velocity component... (sigh)
			 */
			vector	f;
			double	s;

			f.x = -d->vel.y;
			f.y = d->vel.x;

			if ((s = sqrt(sqr(f.x) + sqr(f.y))) != 0.0)
			{
				scalmult(&f, &f, -((double)d->charge) / s);

				vecaddpt(&d->vel, &f, &d->vel);
				vtop(&d->ivel, &d->vel);
			}
		}
	}
	else
	{
		/*
		 * Bounce off left or right border?
		 */
		if (d->pos.x < 0)
		{
			d->vel.x = fabs(d->vel.x);
			vtop(&d->ivel, &d->vel);
		}
		else if (d->pos.x + d->width >= Dx(screen->r))
		{
			d->vel.x = -fabs(d->vel.x);
			vtop(&d->ivel, &d->vel);
		}

		/*
		 * Bounce off top or bottom border?
		 * (bottom is slightly inelastic)
		 */
		if (d->pos.y < 0)
		{
			d->vel.y = fabs(d->vel.y);
			vtop(&d->ivel, &d->vel);
		}
		else if (d->pos.y + d->height >= Dy(screen->r))
		{
			if (gravity.y == 0.0)
				d->vel.y = -fabs(d->vel.y);
			else if (d->ivel.y >= -MINV && d->ivel.y <= MINV)
			{
				/*
				 * y-velocity is too small --
				 * give it a random kick.
				 */
				d->vel.y = (double)-rrand(30, 40);
				d->vel.x = (double)rrand(-10, 10);
			}
			else
				d->vel.y = -fabs(d->vel.y) * k_floor;
			vtop(&d->ivel, &d->vel);
		}
	}

	if (gravity.x != 0.0 || gravity.y != 0.0)
	{
		vecaddpt(&d->vel, &gravity, &d->vel);
		vtop(&d->ivel, &d->vel);
	}

	d->pos = addpt(d->pos, d->ivel);

	drawdot(d);
}

static
void
setup(Dot *d, char *who, uchar *face, int n_els)
{
	int	i, j, k, n;
	int	repl;
	uchar	mask;
	int 	nbits, bits;
	uchar	tmpface[NPJW*NPJW];
	uchar	tmpmask[NPJW*NPJW];
	static	Image	*im;	/* not the global */
	static	Image	*imask;

	if(im == nil)
	{
		im = eallocimage(display, Rect(0,0,NPJW,NPJW), CMAP8, 0, DNofill);
		imask = eallocimage(display, Rect(0,0,NPJW,NPJW), CMAP8, 0, DNofill);
	}

	repl = (NPJW*NPJW)/n_els;
	if(repl > 8) {
		fprint(2, "can't happen --rsc\n");
		exits("repl");
	}
	nbits = 8/repl;
	mask = (1<<(nbits))-1;

	if(0) print("converting %s... n_els=%d repl=%d mask=%x nbits=%d...\n", 
		who, n_els, repl, mask, nbits);
	n = 0;
	for (i = 0; i < n_els; i++)
	{
		for(j = repl; j--; ) {
			bits = (face[i] >> (j*nbits)) & mask;
			tmpface[n] = 0;
			tmpmask[n] = 0;
			for(k = 0; k < repl; k++)
			{
				tmpface[n] |= (mask-bits) << (k*nbits);
				tmpmask[n] |= (bits==mask ? 0 : mask) << (k*nbits);
			}
			n++;
		}

	}

	if(n != sizeof tmpface) {
		fprint(2, "can't happen2 --rsc\n");
		exits("n!=tmpface");
	}

	loadimage(im, im->r, tmpface, n);
	loadimage(imask, imask->r, tmpmask, n);

	for (i = 0; i < nels(d->faces); i++)
	{
		d->faces[i] = eallocimage(display, Rect(0,0,PDUP*NPJW, PDUP*NPJW),
			screen->chan, 0, DNofill);
		d->masks[i] = eallocimage(display, Rect(0,0,PDUP*NPJW, PDUP*NPJW),
			GREY1, 0, DNofill);

		switch (i) {
		case 0:
			copy(d->faces[i], im);
			copy(d->masks[i], imask);
			break;

		case 1:
			copy(d->faces[i], im);
			copy(d->masks[i], imask);
			rotate_clockwise(d->faces[i]);
			rotate_clockwise(d->masks[i]);
			break;

		default:
			copy(d->faces[i], d->faces[i-2]);
			copy(d->masks[i], d->masks[i-2]);
			reflect1_lr(d->faces[i]);
			reflect1_ud(d->faces[i]);
			reflect1_lr(d->masks[i]);
			reflect1_ud(d->masks[i]);
			break;
		}
	}
	d->face = d->faces[0];
	d->mask = d->masks[0];

	d->height = Dy(im->r);
	d->width = Dx(im->r);

	d->mass = 1.0;

	d->spin = nrand(imin(3, total_spin + 1));
	total_spin -= d->spin;

	d->pos.x = nrand(screen->r.max.x - d->width);
	d->pos.y = nrand(screen->r.max.y - d->height);

	d->vel.x = (double)rrand(-20, 20);
	d->vel.y = (double)rrand(-20, 20);
	vtop(&d->ivel, &d->vel);

	drawdot(d);
}

int
msec(void)
{
	static int fd;
	int n;
	char buf[64];

	if(fd <= 0)
		fd = open("/dev/msec", OREAD);
	if(fd < 0)
		return 0;
	if(seek(fd, 0, 0) < 0)
		return 0;
	if((n=read(fd, buf, sizeof(buf)-1)) < 0)
		return 0;
	buf[n] = 0;
	return atoi(buf);
}

/*
 * debugging: make del pause so that we can
 * inspect window.
 */
jmp_buf j;
static void
myhandler(void *v, char *s)
{
	if(strcmp(s, "interrupt") == 0)
		notejmp(v, j, -1);
	noted(NDFLT);
}

void
main(int argc, char *argv[])
{
	int c;
	long now, then;

	ARGBEGIN
	{
	case 'i':
		iflag = 1;
		gravity = no_gravity;
		track_length = 64;
		track_width = 2;
		break;

	case 'k':
		k_floor = atof(ARGF());
		break;

	case 'n':
		track_length = atoi(ARGF());
		break;

	case 'w':
		track_width = atoi(ARGF());
		break;

	case 'x':
		gravity.x = atof(ARGF());
		break;

	case 'y':
		gravity.y = atof(ARGF());
		break;

	default:
		fprint(2, "Usage: %s [-i] [-k k_floor] [-n track_length] [-w track_width] [-x gravityx] [-y gravityy]\n", argv0);
		exits("usage");
	} ARGEND

	if (track_length > MAXTRACKS)
		track_length = MAXTRACKS;

	if (track_length == 0)
		track_width = 0;

	srand(time(0));

	initdraw(0,0,0);
	im = eallocimage(display, Rect(0, 0, PDUP*NPJW, PDUP*NPJW), CMAP8, 0, DNofill);

	draw(screen, screen->r, display->black, nil, ZP);
	
/*	track = balloc(Rect(0, 0, track_width, track_width), 0); */

	edot = &dot[0];

	setup(edot++, "andrew", andrewbits, nels(andrewbits));
	setup(edot++, "bart", bartbits, nels(bartbits));
	setup(edot++, "bwk", bwkbits, nels(bwkbits));
	setup(edot++, "dmr", dmrbits, nels(dmrbits));
	setup(edot++, "doug", dougbits, nels(dougbits));
	setup(edot++, "gerard", gerardbits, nels(gerardbits));
	setup(edot++, "howard", howardbits, nels(howardbits));
	setup(edot++, "ken", kenbits, nels(kenbits));
	setup(edot++, "philw", philwbits, nels(philwbits));
	setup(edot++, "pjw", pjwbits, nels(pjwbits));
	setup(edot++, "presotto", presottobits, nels(presottobits));
	setup(edot++, "rob", robbits, nels(robbits));
	setup(edot++, "sean", seanbits, nels(seanbits));
	setup(edot++, "td", tdbits, nels(tdbits));

	if(PDUP > 1) {	/* assume debugging */
		setjmp(j);
		read(0, &c, 1);

		/* make DEL pause so that we can inspect window */
		notify(myhandler);
	}
	SET(c);
	USED(c);

#define DELAY 100
	for (then = msec();; then = msec())
	{
		Dot	*d;

		for (d = dot; d != edot; d++)
			undraw(d);
		for (d = dot; d != edot; d++)
			upd(d);
		draw(screen, screen->r, screen, nil, screen->r.min);
		flushimage(display, 1);
		now = msec();
		if(now - then < DELAY)
			sleep(DELAY - (now - then));
	}
}
