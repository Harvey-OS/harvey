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
	Rectangle r;		/* location */
};


extern Mouseinfo	mouse;
extern Cursorinfo	cursor;

extern void		mouseupdate(int);

#define kbitblt 	gbitblt
#define	ubitblt		gbitblt
