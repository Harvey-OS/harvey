#include	<u.h>
#include	<libc.h>
#include	<libg.h>
#include	<bio.h>
#include	<regexp.h>

/*
 * todo
 */

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
#define	RADE		3444.054
#define	TWOPI		(PI+PI)
#define	UNKNOWN		(-2.0)
#define	MISSING		(-1.0)
#define	DEGREE(rad)	((rad)*(180/PI))
#define	RSCALE(x)	((50*logsca[rscale])/(x))
#define	CLOSE		(.5/DEGREE(1))

typedef	struct	Rec	Rec;
typedef	struct	Loc	Loc;
typedef	struct	Map	Map;
typedef	struct	Page	Page;
typedef	Rectangle	Box;
typedef	struct	Sym	Sym;

enum
{
	Msize		= 50,
	Update		= 5*1000,		/* seconds per update */
	Gsize		= 10000,		/* locs saved */
	Inset		= 5,
	Nscale		= 10*2-1,
	Reshape		= 0x80,
	Cheight		= 16,
	Nline		= 4,
	Logrec		= 58,
	But1		= 1<<0,
	But2		= 1<<1,
	But3		= 1<<2,
	Etimer		= 1<<5,
	Nclass		= 200,
	Z3		= ~0,			/* black */
	Z2		= ~1,			/* dark grey */
	Z1		= ~2,			/* light grey */
};
struct	Loc
{
	double	lat;	/* radians, N is pos */
	double	lng;	/* radians, E is pos */
	double	sinlat;
	double	coslat;
};
struct	Map
{
	Bitmap*	off;
	Bitmap*	poi;
	int	width;	/* in pix */
	int	height;	/* in pix */
	Loc	loc;	/* lat/lng of center of map */
	Loc	here;	/* lat/lng of moving display */
	double	scale;	/* nm per pix */
	int	offx;
	int	offy;
	int	x;	/* pix plot */
	int	y;	/* pix plot */
	int	moving;
};
struct	Page
{
	Bitmap*	bit;
	Point	loc;
	char	line[Nline][300];
};
struct	Rec
{
	Rec*	link;
	union
	{
		char*	name;
		char**	names;
	};
	Loc*	loc;
	short	nloc;
	uchar	nname;
	uchar	type;
};
struct	Sym
{
	Sym*	link;
	char*	name;
};

Rune	cmd[100];
int	cmdi;
Event	ev;
double	logsca[Nscale];	/* 0base is (Nscale-1)/2 */
double	magvar;
struct
{
	uchar	type;
	int	nname;
	char*	name[30];
	int	scale;
	int	nlat;
	long	lat[20000];	/* list of lat/lng */
} rec;
Rec*	reclist;
Map	map;
int	rscale;
uchar	dispclass[Nscale][Nclass];
uchar	saveclass[Nclass];
uchar	lightclass[Nclass];
uchar	darkclass[Nclass];
Sym*	hash[10007];
struct
{
	Loc	loc;
	Page	p1;
} w;

uchar	classa[100];
uchar	classb[100];
uchar	classc[100];
uchar	classd[100];
uchar	classe[100];
uchar	classf[100];
uchar	classh[100];
uchar	classx[100];
char*	big[];
char*	small[];
char*	light[];
char*	dark[];
Cursor	thinking;
Cursor	bullseye;
Cursor	reading;

double	angle(Loc*, Loc*);
char*	colskip(char*);
int	convL(Loc**, int, int, int, int);
double	dist(Loc*, Loc*);
Loc	dmecalc(Loc *l, double az, double d);
void	docmd(void);
void	drawmap(void);
void	ereshaped(Box);
double	getlat(char*);
void	getloc(Loc*, char*);
void	initloc(Loc*);
Loc	invmap(int);
double	leg(double);
void	logoinit(void);
void	lookup(Loc*, int);
void	mapcoord(Loc*);
double	norm(double, double);
void	normscales(void);
void	poilogo(Bitmap*);
double	qatof(char*);
double	sphere(double, double, double, double, double, double);

void	drawlist(double, double, double, double);
void	setdclass(void);
void	readdict(char*, Loc*, int);
int	class(char*);
int	Tinit(char*);
void*	Trdline(void);
int	Tclose(void);
void	search(char*);
char*	convcmd(void);
void	getplace(char*, Loc*);
