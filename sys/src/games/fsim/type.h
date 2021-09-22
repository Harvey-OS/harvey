// kcrp ils is screwed up
// put in more floating and less fixed
// placement of labels on map
// move view to center of window
// change map
//	left half ff hidden/always display
//	right half reset view

#include	<u.h>
#include	<libc.h>
#include	<draw.h>
#include	<event.h>
#include	<bio.h>

typedef	unsigned	Angle;
typedef	int		Fract;
typedef	struct	Plane	Plane;
typedef	struct	Nav	Nav;

#undef	muldiv
#undef	PI

#define	muldiv(a,b,c)	((((long)a)*((long)b))/((long)c))
#define	NNULL		((Nav*)0)
#define	DEG(x)		muldiv(x,PI,180)
#define	KNOT(x)		(170*(x))
#define	MILE(x)		(6076*(x))

#define	CR	(3444.054)
#define	CP	(3.14159265358979323846264338327950288419)
#define	C0	((double)(1<<16)*(double)(1<<16))
#define	C1	(2 * CP / C0)
#define	C2	(C1 * MILE(CR))

enum
{
	VOR		= 1<<0,
	NDB		= 1<<1,
	ILS		= 1<<2,
	GS		= 1<<3,
	OM		= 1<<4,
	MM		= 1<<5,
	IM		= 1<<6,
	DME		= 1<<7,
	APT		= 1<<8,
	DATA		= 1<<9,
	LEVEL		= 3,
	PI		= 32768,
	ONE		= 32768,
	NONE		= 0x8000,
	BREAK		= 0x8001,
	H		= 14,
	W		= 9,
	Q		= 4,
};

struct	Plane
{
	long	time;

	ulong	lat;		/* latlng 32 bit fraction of a circle */
	ulong	lng;
	double	coslat;

	short	z;		// height
	short	tz;
	Angle	ah;		// angle of heading

	short	dx;		// speed
	short	dz;		// rate of climb
	Angle	ap;
	Angle	ab;		// angle of bank
	Angle	rb;		// rate of angle of bank

	Angle	wlo, whi;
	short	mlo, mhi;

/*
 * tunables
 */
	short	pwr;
	ushort	vor1;
	ushort	obs1;
	ushort	vor2;
	ushort	obs2;
	ushort	adf;

	/*
	 * constant search to update
	 * the objects tuned in
	 * and viewable apts
	 */
	Nav*	ap1;
	Nav*	ap2;
	Nav*	v1;
	Nav*	g1;
	Nav*	d1;
	Nav*	v2;
	Nav*	g2;
	Nav*	d2;
	Nav*	ad;
	Nav*	bk;

	/*
	 * window
	 */
	int	side;
	int	side14;
	int	side24;
	int	side34;
	int	origx;
	int	origy;
	int	obut;
	int	nbut;
	int	butx;
	int	buty;
	int	tempx;
	int	tempy;
	Angle	magvar;
	Fract	magsin;
	Fract	magcos;
	char	key[6];

	Biobuf*	trace;		// (dtime, lat, lng, alt) per second
	long	otime;

};
struct Nav
{
	char*	name;
	ushort	flag;
	ushort	obs;		// magv for airports
	ushort	freq;
	short	z;
	ulong	lat;		// latlng 32 bit fraction of a circle
	ulong	lng;
};

Nav*	nav;
Plane	plane;
Mouse	mouse;
int	proxflag;

long	fmul(Fract, long);
Fract	fdiv(long, long);
void	ringbell(void);
int	button(int);
long	realtime(void);
void	nap(void);
void	datmain(void);

void	altinit(void);
void	altupdat(void);
void	apinit(void);
void	apupdat(void);
void	adfinit(void);
void	adfupdat(void);
void	gpsinit(void);
void	gpsupdat(void);
void	attinit(void);
void	attupdat(void);
void	asiinit(void);
void	asiupdat(void);
void	clkinit(void);
void	clkupdat(void);
void	dgyupdat(void);
void	dgyinit(void);
Angle	iatan(long, long);
long	ihyp(long, long);
Fract	isin(Angle);
Fract	icos(Angle);
void	convertll(long, long);
void	inpinit(void);
void	inpupdat(void);
void	main(int, char*[]);
void	blinit(void);
void	blupdat(void);
void	mginit(void);
void	mgupdat(void);
void	calcvar(void);
int	setxyz(char*);
Nav*	lookup(char*, int);
void	pwrinit(void);
void	pwrupdat(void);
void	nvinit(void);
void	nvupdat(void);
void	plupdat(void);
void	plinit(void);
void	plstart(void);
void	setorg(void);
void	vorinit(void);
void	vorupdat(void);
void	cross(Image*, int, int, int, int);
void	vsiinit(void);
void	vsiupdat(void);
void	tabinit(void);
void	tabupdat(void);
Angle	bearto(Nav*);
long	fdist(Nav*);
void	dconv(int, char*, int);
int	deg(Angle);
int	hit(int, int, int);
int	mod3(int, int);
int	mod6(int, int);
void	dodraw(Image *, int, long*, Rectangle*);
void	outd(int);
void	normal(long*, long*);
int	rchar(int);
void	mkbinit(void);
void	mkbupdat(void);

void	vorlogo(Image*);
void	dmelogo(Image*);
void	ndblogo(Image*);
void	mbklogo(Image*);
void	aptlogo(Image*);

void	d_clr(Image*, Rectangle);
void	d_set(Image*, Rectangle);
void	d_string(Image*, Point, Font*, char*);
void	d_segment(Image*, Point, Point, int);
void	d_point(Image*, Point, int);
void	d_circle(Image*, Point, int, int);
Image*	d_balloc(Rectangle, int);
void	d_bfree(Image*);
void	d_bflush(void);
void	d_binit(void);
void	d_copy(Image*, Rectangle, Image*);
void	d_or(Image*, Rectangle, Image*);


#define	D		screen
#define	Drect		screen->r

/* data initialization files */
typedef	struct	Dils	Dils;
typedef	struct	Dvor	Dvor;
typedef	struct	Dapt	Dapt;

struct	Dils
{
	char*	name;
	short	hdg;
	short	freq;
	short	im;
	short	mm;
	short	om;
	short	lom;
	short	gs;
	short	dme;
};
struct	Dvor
{
	char*	name;
	float	lat;
	float	lng;
	short	flag;
	short	freq;
	short	elev;
};
struct	Dapt
{
	char*	name;
	float	lat;
	float	lng;
	short	elev;
	short	magv;
};

extern	Dils	dils[];
extern	Dvor	dvor[];
extern	Dapt	dapt[];
