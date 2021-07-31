#include	<u.h>
#include	<libc.h>
#include	<libg.h>

typedef	unsigned	Angle;
typedef	int		Fract;
typedef	struct	Plane	Plane;
typedef	struct	Nav	Nav;
typedef	struct	Apt	Apt;
typedef	struct	Var	Var;


#undef	muldiv
#undef	PI

#define	muldiv(a,b,c)	((((long)a)*((long)b))/((long)c))
#define	DEG(x)		muldiv(x,PI,180)
#define	NNULL		((Nav*)0)
#define	ANULL		((Apt*)0)
#define	D		&screen
#define	Drect		screen.r

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
	LEVEL		= 3,
	F_CLR		= Zero,
	F_STORE		= S,
	F_XOR		= DxorS,
	F_OR		= DorS,
	PI		= 32768,
	ONE		= 32768,
	KNOT		= 170,
	MILE		= 6076,
	NONE		= 0x8000,
	BREAK		= 0x8001,
	H		= 14,
	W		= 9,
	Q		= 4,
};

struct	Plane
{
	long	time;

	long	x;		/* location */
	long	y;
	short	z;		/* height */
	short	tz;
	Angle	ah;		/* angle of heading */

	short	dx;		/* speed */
	short	dz;		/* rate of climb */
	Angle	ap;
	Angle	ab;		/* angle of bank */
	Angle	rb;		/* rate of angle of bank */

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
	 * the 4 objects tuned in
	 * and 2 viewable apts
	 */
	Apt*	ap1;
	Apt*	ap2;
	Nav*	v1;
	Nav*	g1;
	Nav*	v2;
	Nav*	g2;
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
	char	key[4];
};
struct	Apt
{
	long	x, y;
	short	z;
};
struct Nav
{
	ushort	flag;
	ushort	obs;
	ushort	freq;
	short	z;
	long	x, y;
};
struct	Var
{
	long	x;
	long	y;
	Angle	v;
};

extern	Var	var[];
extern	Nav	nav[];
extern	Apt	apt[];
Plane	plane;
Mouse	mouse;

long	fmul(Fract, long);
Fract	fdiv(long, long);
void	rectf(Bitmap*, Rectangle, Fcode);
void	ringbell(void);
int	button(int);
long	realtime(void);
void	nap(void);

void	altinit(void);
void	altupdat(void);
void	doalt(int);
void	apinit(void);
void	apupdat(void);
void	adfinit(void);
void	adfupdat(void);
void	attinit(void);
void	attupdat(void);
void	horiz(int);
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
void	inpinit(void);
void	inpupdat(void);
void	spot(int, int);
void	main(int, char*[]);
void	blinit(void);
void	blupdat(void);
void	mginit(void);
void	mgupdat(void);
void	calcvar(void);
void	setxyz(int, int, int);
void	pwrinit(void);
void	pwrupdat(void);
void	nvinit(void);
void	nvupdat(void);
void	plupdat(void);
void	plinit(void);
void	setorg(void);
void	vorlogo(Bitmap*);
void	dmelogo(Bitmap*);
void	ndblogo(Bitmap*);
void	mbklogo(Bitmap*);
void	aptlogo(Bitmap*);
void	vorinit(void);
void	vorupdat(void);
void	cross(int, int, int, int);
void	rocinit(void);
void	rocupdat(void);
void	tabinit(void);
void	tabupdat(void);
Angle	bearto(long, long);
long	fdist(long, long);
void	dconv(int, char*, int);
int	deg(Angle);
int	hit(int, int, int);
int	mod3(int, int);
int	mod6(int, int);
void	draw(int, long*, Rectangle*);
void	outd(int);
void	normal(long*);
void	rad50(ushort, char*);
int	rchar(int);
void	mkbinit(void);
void	mkbupdat(void);
