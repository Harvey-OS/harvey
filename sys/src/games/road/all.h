#include	<u.h>
#include	<libc.h>
#include	<libg.h>
#include	<bio.h>
#include	<regexp.h>

long	count;
long	histo[51];

/*
 * todo
 *	page display
 *		scale
 *		lat.lng
 *		mapreading
 *		gpsreadings
 *		dist/bearing to nearest regexp
 *		errors now printed
 *	some bug with dist to nearest point/line
 *	make big scale set of maps
 *	make aviation set of maps
 *	put in better setting of map.loc when moving.
 *	partial map drawing when moving.
 *	asynchronous loading and drawing
 *	fast vector draw (from rob)
 *	put in 'clear' part of cursors.
 */


typedef	Rectangle	Rec;

typedef	struct	Mem	Mem;
typedef	struct	Loc	Loc;
typedef	struct	Page	Page;
typedef	struct	Box	Box;
typedef	struct	Sym	Sym;
typedef	struct	Record	Record;
typedef	struct	Name	Name;
typedef	struct	Line	Line;

enum
{
	Perdegree	= 5000000,
	Perbox		= 1000000,
	Close1		= 100000,
	Msize		= 50,
	Gpsrsize	= 58,			/* gps record size */
	Update		= 5*1000,		/* seconds per update */
	Inset		= 1,
	Reshape		= 0x80,
	Cheight		= 16,
	Nline		= 4,
	Ntrack		= 1000,
	But1		= 1<<0,
	But2		= 1<<1,
	But3		= 1<<2,
	Etimer		= 1<<5,
	Z3		= 0,			/* black */
	Z2		= 1,			/* dark grey */
	Z1		= 2,			/* light grey */
	Nscale		= 21,
	Scale12		= 11,			/* border between scales */
	Scale23		= 15,
};

struct	Mem	
{
	Mem*	link;
};
struct	Loc
{
	long	lat;	/* 5e6==1°, N is pos */
	long	lng;	/* 5e6==1°, E is pos */
};
struct	Page
{
	Bitmap*	bit;
	Point	loc;
	char	line[Nline][300];
};
struct	Box
{
	Box*	link;		/* list of boxes, based at boxlist */
	void*	memchain;	/* handle to free whole box */
	Sym*	hash[101];	/* symbol hashtable for this box */
	Record*	lines;		/* list of lines in this box */
	Record*	points;		/* list of points in this box */
	short	boxx;		/* coordinates of this box */
	short	boxy;
	char	scale;		/* gross scale of this box */
	char	used;
};
struct	Record
{
	Record*	link;
	Name*	name;
	Line*	line;
	long	linex1;
	long	linex2;
	long	liney1;
	long	liney2;
	char	grey;
};
struct	Name
{
	Name*	link;
	Sym*	sym;
};
struct	Sym
{
	Sym*	link;
	char*	chars;
	uchar	match;
};
struct	Line
{
	Line*	link;
	Loc;
};

Loc	track[Ntrack];
int	ntrack;
Rune	cmd[100];
int	cmdi;
Event	ev;
struct
{
	Box*	boxlist;
	Bitmap*	off;		/* off-screen bitmap 1.5×1.5 screen */
	Bitmap*	poi;		/* point-of-interest logo */
	Bitmap*	com;		/* center-of-map logo */
	Bitmap*	pla;		/* place logo */
	long	memleft;
	char*	memptr;
	Mem*	membase;
	long	memtotal1;
	long	memtotal2;
	Page	p1;
	Rec	screenr;
	int	width;		/* of off-screen in pix */
	int	height;		/* of off-screen in pix */
	Loc	loc;		/* lat/lng of center of off-screen */
	Loc	last;		/* lat/lng of center of last off-screen */
	Loc	here;		/* lat/lng of center of screen */
	int	indraw;		/* flag -- inside draw */
	int	dodelay;
	double	xscale;		/* conversion (lat/lng to off-screen pix) parameters */
	double	yscale;
	double	xoffset;
	double	yoffset;
	double	logsca[Nscale];
	int	rscale;
	char*	glog;
	int	gpsfd;
	int	gpsenabled;
	int	screend;	/* ldepth of screen */
} map;

extern	Cursor	thinking;
extern	Cursor	bullseye;
extern	Cursor	reading;
extern	Cursor	drawing;

Loc	adjloc(void);
void*	ca(long);
char*	convcmd(void);
void	docmd(void);
void	drawmap(Loc);
void	drawoff(void);
void	drawpage(Page*);
long	dtoline(long, long, long, long, long, long);
void	ereshaped(Rec);
int	getgps(Loc*);
void	getplace(char*, Loc*);
void	getfiles(char*);
void	getlatlng(char*, Loc*);
void	initloc(Loc*);
Loc	invmap(int);
void	loadbox(int, int, int);
void	loadboxs(void);
void	logoinit(void);
void	lookup1(Loc, int);
void	lookup2(Loc, int);
void*	memchain(void);
void	memfree(void*);
int	myatol(char*);
Box*	newbox(int, int, int);
void	norm(Loc*, double, double);
void	normal(Loc*);
void	panic(char*);
int	pointvis(int, int, Rec*);
void	search(char*);
int	segvis(int, int, int, int, Rec*);
int	Tclose(void);
int	Tinit(char*);
void*	Trdline(void);
void	doexit(void);
void	bitclr(Bitmap*);
void	doevent(void);

void	hiseg(long, long, long, long, long, Rec*);
void	hipoint(long, long, long, Rec*);
void	trackgps(void);
void	gpsoff(void);
int	getgpslat(char*, Loc*);
