typedef struct Mouseinfo	Mouseinfo;
typedef struct Cursorinfo	Cursorinfo;

struct Mouseinfo{
	/*
	 * First three fields are known in some l.s's
	 */
	int	dx;		/* interrupt-time delta */
	int	dy;
	int	track;		/* update cursor on screen */
	Mouse;
	int	changed;	/* mouse structure changed since last read */
	Rendez	r;
	int	newbuttons;	/* interrupt time access only */
	int	clock;		/* check mouse.track on RTE */
};

struct Cursorinfo{
	Cursor;
	Lock;
	int	visible;	/* on screen */
	int	disable;	/* from being used */
	Rectangle r;		/* location */
};

extern Mouseinfo	mouse;
extern Cursorinfo	cursor;

void	mouseupdate(int);

void	_gbitblt(GBitmap*, Point, GBitmap*, Rectangle, Fcode);
void	_gtexture(GBitmap*, Rectangle, GBitmap*, Fcode);
void	_gsegment(GBitmap*, Point, Point, int, Fcode);
void	_gpoint(GBitmap*, Point, int, Fcode);
void	hwscreenwrite(int, int);

/* for devbit.c */

#define	gbitblt		_gbitblt
#define	gtexture	_gtexture
#define	gsegment	_gsegment
#define	gpoint		_gpoint

#define mbbpt(x)
#define mbbrect(x)
#define screenupdate()
#define mousescreenupdate()
