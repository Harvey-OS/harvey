typedef struct Mouseinfo Mouseinfo;
typedef struct Mousestate Mousestate;
typedef struct Cursorinfo Cursorinfo;
typedef struct Screeninfo Screeninfo;

#define Mousequeue 16		/* queue can only have Mousequeue-1 elements */
#define Mousewindow 500		/* mouse event window in millisec */

struct Mousestate {
	int	buttons;
	Point	xy;
	ulong	msec;
};

struct Mouseinfo {
	Lock	lk;
	Mousestate queue[Mousequeue];
	int	ri, wi;
	int	lastb;
	int	trans;
	int	open;
	Rendez	r;
};

struct Cursorinfo {
	Lock	lk;
	Point	offset;
	uchar	clr[2*16];
	uchar	set[2*16];
};

struct Screeninfo {
	Lock		lk;
	Memimage	*newsoft;
	int		reshaped;
	int		depth;
	int		dibtype;
};

extern	Memimage *gscreen;
extern	Mouseinfo mouse;
extern	Cursorinfo cursor;
extern	Screeninfo screen;

void	screeninit(void);
void	screenload(Rectangle, int, uchar *, Point, int);

void	getcolor(ulong, ulong*, ulong*, ulong*);
void	setcolor(ulong, ulong, ulong, ulong);

void	refreshrect(Rectangle);

void	cursorarrow(void);
void	setcursor(void);
void	mouseset(Point);
void	drawflushr(Rectangle);
void	flushmemscreen(Rectangle);
uchar *attachscreen(Rectangle*, ulong*, int*, int*, int*, void**);

void	drawqlock(void);
void	drawqunlock(void);
int	drawcanqlock(void);
void	terminit(void);
