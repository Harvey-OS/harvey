typedef struct Cursor Cursor;
typedef struct Cursorinfo	Cursorinfo;
typedef struct Scr	Scr;

struct Cursorinfo
{
	Cursor;
	Lock;
};

extern Cursor	arrow;
extern ulong	blanktime;
extern Cursorinfo cursor;

extern void	cursoroff(int);
extern int	cursoron(int);
extern void	mouseaccelerate(int);
extern void	mouseresize(void);
extern void	mousetrack(int, int, int, int);
extern void	setcursor(Cursor* curs);

extern uchar*	attachscreen(Rectangle*, ulong*, int*, int*, int*);
extern void	blankscreen(int);
extern void	flushmemscreen(Rectangle);

#define ishwimage(i)	0

/* for communication between devdss.c and screen.c */

enum {					/* maxima */
	Wid		= 1280,
	Ht		= 1024,
	Depth		= 16,		/* bits per pixel */

	Pcolours	= 256,		/* Palette */
	Pred		= 0,
	Pgreen		= 1,
	Pblue		= 2,

	Pblack		= 0x00,
	Pwhite		= 0xFF,
};

typedef struct OScreen OScreen;
typedef struct Omap3fb Omap3fb;

struct OScreen {
	int	open;
	uint	wid;
	uint	ht;
	uint	freq;
	uint	chan;

//	Lock;
//	Cursor;

	/* shouldn't be needed? */
	uint	pixelclock;
	uint	hbp;
	uint	hfp;
	uint	hsw;

	uint	vbp;
	uint	vfp;
	uint	vsw;
};

struct Omap3fb {		/* frame buffer for 24-bit active color */
//	short	palette[256];
	/* pixel data, even; base type's width must match Depth */
	ushort	pixel[Wid*Ht];
};

struct Scr {
	Lock	devlock;
//	VGAdev*	dev;
//	Pcidev*	pci;

//	VGAcur*	cur;
	ulong	storage;
	Cursor;

	int	useflush;

	ulong	paddr;		/* frame buffer */
	void*	vaddr;
	int	apsize;

	ulong	io;			/* device specific registers */
	ulong	*mmio;
	
	ulong	colormap[Pcolours][3];
	int	palettedepth;

	Memimage* gscreen;
	Memdata* gscreendata;
	Memsubfont* memdefont;

	int	(*fill)(Scr*, Rectangle, ulong);
	int	(*scroll)(Scr*, Rectangle, Rectangle);
	void	(*blank)(Scr*, int);

	ulong	id;	/* internal identifier for driver use */
	int	isblank;
	int	overlayinit;
};
