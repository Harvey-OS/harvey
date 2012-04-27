typedef struct Cursor Cursor;
typedef struct Cursorinfo	Cursorinfo;
typedef struct OScreen OScreen;
typedef struct Omap3fb Omap3fb;
typedef struct Settings Settings;

struct Cursorinfo
{
	Cursor;
	Lock;
};

extern Cursor	arrow;
extern Cursorinfo cursor;

/* devmouse.c */
extern void mousetrack(int, int, int, int);
extern Point mousexy(void);

extern void	mouseaccelerate(int);
extern void	mouseresize(void);

/* screen.c */
extern uchar* attachscreen(Rectangle*, ulong*, int*, int*, int*);
extern void	flushmemscreen(Rectangle);
extern int	cursoron(int);
extern void	cursoroff(int);
extern void	setcursor(Cursor*);
extern int	screensize(int, int, int, ulong);
extern int	screenaperture(int, int);
extern Rectangle physgscreenr;	/* actual monitor size */
extern void	blankscreen(int);

extern void swcursorinit(void);
extern void swcursorhide(void);
extern void swcursoravoid(Rectangle);
extern void swcursorunhide(void);

/* devdraw.c */
extern void	deletescreenimage(void);
extern void	resetscreenimage(void);
extern int		drawhasclients(void);
extern ulong	blanktime;
extern void	setscreenimageclipr(Rectangle);
extern void	drawflush(void);
extern int drawidletime(void);
extern QLock	drawlock;

#define ishwimage(i)	0		/* for ../port/devdraw.c */

/* for communication between devdss.c and screen.c */

enum {
	/* maxima */
	Wid		= 1280,
	Ht		= 1024,
	Depth		= 16,		/* bits per pixel */

	Pcolours	= 256,		/* Palette */
	Pred		= 0,
	Pgreen		= 1,
	Pblue		= 2,

	Pblack		= 0x00,
	Pwhite		= 0xFF,

	/* settings indices */
	Res800x600	= 0,
	Res1024x768,
	Res1280x1024,
	Res1400x1050,
};

struct Settings {
	uint	wid;		/* width in pixels */
	uint	ht;		/* height in pixels */
	uint	freq;		/* refresh frequency; only printed */
	uint	chan;		/* draw chan */

	/* shouldn't be needed? */
	uint	pixelclock;

	/* horizontal timing */
	uint	hbp;		/* back porch: pixel clocks before scan line */
	uint	hfp;		/* front porch: pixel clocks after scan line */
	uint	hsw;		/* sync pulse width: more hfp */

	/* vertical timing */
	uint	vbp;		/* back porch: line clocks before frame */
	uint	vfp;		/* front porch: line clocks after frame */
	uint	vsw;		/* sync pulse width: more vfp */
};

struct OScreen {
	Lock;
	Cursor;
	Settings *settings;
	int	open;
};

struct Omap3fb {		/* frame buffer for 24-bit active color */
//	short	palette[256];
	/* pixel data, even; base type's width must match Depth */
	ushort	pixel[Wid*Ht];
};
