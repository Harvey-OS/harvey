#pragma	lib	"cda/libfizz.a"

#define exit(x) exits(x ? "abnormal exit" : (char *) 0)

#define		S_CHIP		1	/* chip name -> (Chip *) */
#define		S_SIGNAL	2	/* signal(net) name -> (Signal *) */
#define		S_PACKAGE	3	/* package name -> (Package *) */
#define		S_TYPE		4	/* type name -> (Type *) */
#define		S_TT		5	/* net name -> (Nsig *) */
#define		S_VSIG		6	/* special sig name -> sig number+1 */
#define		S_CSMAP		7	/* chip name to Signal ** */

typedef struct Point
{
	short x, y;
} Point;

typedef struct Rectangle
{
	Point min, max;
} Rectangle;

typedef struct Plane
{
	Rectangle r;
	char *sig;		/* signal name */
	unsigned short layer;
	short sense;		/* +-1 */
} Plane;

typedef struct Coord
{
	char *chip;
	short pin;
	char *pinname;
} Coord;

typedef struct Pin
{
	Point p;
	char drill;
	char used;
} Pin;

typedef struct Wire
{
	float length;
	int ninf;
	struct Wire *next, *previous;
	Pin *inflections;
} Wire;

/*
	routing algorithms
*/
enum Algorithm {
	RTSP=1,		/* travelling salesman */
	RTSPE,		/* travelling salesman with one end specified */
	RMST,		/* minimal spanning tree */
	RMST3,		/* minimal spanning tree of degree 3 */
	RDEFAULT,	/* use default (normally RTSP or RMST) */
	RHAND		/* hand routed */
};

typedef struct Signal
{
	char *name;
	short n;
	short nw;
	short nr;
	char type;
	char done;		/* for place: set if signal routed and sent */
	enum Algorithm alg;	/* alg used to route */
	void  (*prfn)(struct Signal *);	/* reccomended function to print routed signal */
	long length;
	Coord *coords;
	Coord *routes;
	Pin *pins;
	Wire *wires;
	int nlayout;
	Pin *layout;
	long num;
	long nam;		/* for place */
} Signal;

/* values for type */
#define		NORMSIG		0
#define		VSIG		1
#define		SUBVSIG		2
#define		NOVPINS		4

typedef struct Package
{
	char *name;
	Rectangle r;
	short pmin, pmax;
	short npins, ndrills;
	long class;
	char *xyid, *xydef;
	Point xyoff;
	Pin *pins, *drills;
	short ncutouts, nkeepouts;
	Plane *cutouts, *keepouts;
} Package;

typedef struct Pinhole
{
	Point o, len;
	Point sp;
	char drill;
	struct Pinhole *next;
} Pinhole;

typedef struct Pinspec
{
	short p1, pd, p2;
	Point pt1, pt2;
	char drill;
} Pinspec;

typedef struct Nsig
{
	Coord c;
	char *sig;
	struct Nsig *next;
} Nsig;

typedef struct Type
{
	char *name;
	char *pkgname;
	Package *pkg;
	char *comment;
	char *tt;
	char *code, *value, *part;	/* for saf goo (bart) */
	char *family;
	long nam;		/* for place */
} Type;

typedef struct Vsig
{
	char *name;
	short signo;
	short npins;
	Pin *pins;
} Vsig;

typedef struct Chip
{
	char *name;
	char *typename;
	char *comment;
	Type *type;
	Point pt;
	Rectangle r;
	short rotation;
	short flags;
	short npins, ndrills;
	short pmin;
	Pin *pins, *drills;
	char *netted;
	long nam;		/* for place */
} Chip;

/* values for netted */
#define		NOTNETTED	0
#define		SIGNUM		0x07
#define		NETTED		0x08
#define		VBNET		0x10
#define		VBPIN		0x20	/* not netted because of coincident pin */

#define		MAXLAYER	16
#define		LAYER(i)	(1<<(i))

typedef struct Drillsz {
	char letter;
	short dia;
	char type;
	Point *pos;
	short count;
	char *xydef;
} Drillsz;



typedef struct Board
{
	char *name;
	Point align[4];
	Vsig v[6];
	Point ne, nw, se, sw;		/* extrema */
	Rectangle prect;		/* bounding rect of pinholes+vbpins */
	Pinhole *pinholes;		/* compact form for display */
	Plane *planes;			/* all positive sense */
	int nplanes;
	Plane *keepouts;		/* set up by cutouts */
	int nkeepouts;
	char *layer[MAXLAYER];		/* xymask layers */
	unsigned short layers;		/* layers to output */
	Pin datums[3];
	int ndrillsz;
	Drillsz *drillsz;
	short ndrills;
	Pin *drills;
	char *xyid, *xydef;
} Board;

#define		NEW(type)	((type *)f_malloc((long)sizeof(type)))

#define	ROWLEN		40000L
#define	XY(x,y)	((y)*ROWLEN+(long)(x))
#define	MAXCOORD	32767
#define	MAXNET		1000
#define	INCH		1000

/*
	chip flags
*/
#define	PLACE		3
#define	SOFT		0
#define	HARD		1
#define	NOMOVE		2
#define	UNPLACED	4
#define	IGRECT		8
#define	IGPINS		16
#define	IGNAME		32	/* don't plot name on artwork */
#define	IGPIN1		64	/* don't box pin 1 on artwork */
#define WSIDE		128	/* on wire side */
#define IGBOX		256	/* don't plot box on artwork */

#define	NNAME	137
#define	ISBL(c)	(f_chtype[c] < 1)
#define	ISNM(c)	(f_chtype[c] > 1)
#define	isdigit(c)	(f_chtype[c] == 3)
#define	isupper(c)	(((c) >= 'A') && ((c) <= 'Z'))
#define	tolower(c)	((c)-'A'+'a')

#define	NUM(p,n)	{\
	int sign = 1; nm = 0;\
	if(*p == '-') sign=-1, p++;\
	if(!isdigit(*p))f_minor("expected a number, found '%s'", p);\
	while(isdigit(*p)) nm = nm*10 + *p++ - '0';\
	n = nm*sign; while(ISBL(*p)) p++; }
#define	NAME(p)	{\
	char *op = p;\
	if(!ISNM(*p))f_minor("expected a name, found '%s'", p);\
	while(ISNM(*p)) p++;\
	if(p>=&op[NNAME]){ f_minor("name '%s' too long", op); op[NNAME-1] = 0;}\
	if(*p) *p++ = 0; while(ISBL(*p)) p++; }
#define	BLANK(p)	{ while(ISBL(*p)) p++; }
#define	COORD(s, x, y)	{\
	BLANK(s) NUM(s, x)\
	if(*s++ != '/') f_minor("expect a / in coord, not '%c'", s[-1]);\
	BLANK(s) NUM(s, y) }
#define	EOLN(p)	{ f_minor("unexpect goo '%s' at end of line\n", p); }

typedef struct Keymap{
	char *id;
	void  (*fn)(char *);
	int index;
} Keymap;
#define NULLFN (void (*)(char *)) 0

extern char	f_chtype[];
extern int	quiet;
extern long	wwraplen, wwires;
extern int	f_nerrs;
extern int	scale;
extern char	*f_ifname;
extern Board	*f_b;

void	blim(Point p);
void	chkvsig(Vsig *v, int n);
void	csmap(void);
void	cutout(Board *b);
int	f_crack(char *file, Board *b);
void	f_init(Board *b);
char 	*f_inline(void);
int	f_keymap(Keymap *, char **);
char 	*f_malloc(long);
Pin	*f_pins(char *,int, int);
char	*f_strdup(char *);
char	*f_tabline(void);
void	fizzinit(void);
int	fizzplace(void);
void	fizzplane(Board *b);
int	fizzprewrap(void);
void	Free(char *s);
int	getopt(int argc, char **argv, char *opts);
int	layerof(unsigned short);
char	*lmalloc(long);
void	netlen(Signal *s);
void	nn(Point, long *, short *);
void	nnprep(Pin *, int);
void	padd(Point *, Point );
void	pininit(void); 
long	pinlook(long , long );
void	pintraverse(void (*)(Pin));
void	pkgclass(void *);
void	place(Chip *);
void	planerot(Rectangle *rr, Chip *c);
void	prwrap(Signal *s);
void	fg_padd(Point *, Point);
void	fg_psub(Point *, Point);
int	fg_ptinrect(Point p, Rectangle r);
void	putnet(Signal *);
Rectangle fg_raddp(Rectangle, Point);
char	*Realloc(char *ptr, long n);
void	fg_rcanon(Rectangle *);
Chip	*rectadd(Chip *);
int	fg_rectclip(Rectangle *, Rectangle);
int	fg_rectXrect(Rectangle, Rectangle, Rectangle *);
void	reorder(Coord *, Pin *, short *, int);
void	routedefault(int n);
int	fg_rXr(Rectangle, Rectangle);
Point	SC(Point);
void	setroot(char **av, int b, int e);
void	*symlook(char *sym, int space, void *install);
void	symtraverse(int, void (*)(void *));
long	timer(int);
void	ttadd(int, char *, int);
void	ttclean(Nsig *nn);
char	*ttnames;
void	unplace(Chip *);
void	vbest(char *s);
void	wrap(Signal *);
void	xyrot(Point *off, int r);
