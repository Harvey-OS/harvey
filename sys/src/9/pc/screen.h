typedef struct Cursor Cursor;
typedef struct Cursorinfo Cursorinfo;
struct Cursorinfo {
	Cursor;
	Lock;
};

/* devmouse.c */
extern void mousetrack(int, int, int, int);
extern Point mousexy(void);

extern void mouseaccelerate(int);
extern int m3mouseputc(Queue*, int);
extern int m5mouseputc(Queue*, int);
extern int mouseputc(Queue*, int);

extern Cursorinfo cursor;
extern Cursor arrow;

/*
 * Generic VGA registers.
 */
enum {
	MiscW		= 0x03C2,	/* Miscellaneous Output (W) */
	MiscR		= 0x03CC,	/* Miscellaneous Output (R) */
	Status0		= 0x03C2,	/* Input status 0 (R) */
	Status1		= 0x03DA,	/* Input Status 1 (R) */
	FeatureR	= 0x03CA,	/* Feature Control (R) */
	FeatureW	= 0x03DA,	/* Feature Control (W) */

	Seqx		= 0x03C4,	/* Sequencer Index, Data at Seqx+1 */
	Crtx		= 0x03D4,	/* CRT Controller Index, Data at Crtx+1 */
	Grx		= 0x03CE,	/* Graphics Controller Index, Data at Grx+1 */
	Attrx		= 0x03C0,	/* Attribute Controller Index and Data */

	PaddrW		= 0x03C8,	/* Palette Address Register, write */
	Pdata		= 0x03C9,	/* Palette Data Register */
	Pixmask		= 0x03C6,	/* Pixel Mask Register */
	PaddrR		= 0x03C7,	/* Palette Address Register, read */
	Pstatus		= 0x03C7,	/* DAC Status (RO) */

	Pcolours	= 256,		/* Palette */
	Pred		= 0,
	Pgreen		= 1,
	Pblue		= 2,

	Pblack		= 0x00,
	Pwhite		= 0xFF,
};

#define VGAMEM()	0xA0000
#define vgai(port)		inb(port)
#define vgao(port, data)	outb(port, data)

extern int vgaxi(long, uchar);
extern int vgaxo(long, uchar, uchar);

/*
 */
typedef struct VGAdev VGAdev;
typedef struct VGAcur VGAcur;
typedef struct VGAscr VGAscr;

struct VGAdev {
	char*	name;

	void	(*enable)(VGAscr*);
	void	(*disable)(VGAscr*);
	void	(*page)(VGAscr*, int);
	void	(*linear)(VGAscr*, int, int);
	void	(*drawinit)(VGAscr*);
	int	(*fill)(VGAscr*, Rectangle, ulong);
	void	(*ovlctl)(VGAscr*, Chan*, void*, int);
	int	(*ovlwrite)(VGAscr*, void*, int, vlong);
	void (*flush)(VGAscr*, Rectangle);
};

struct VGAcur {
	char*	name;

	void	(*enable)(VGAscr*);
	void	(*disable)(VGAscr*);
	void	(*load)(VGAscr*, Cursor*);
	int	(*move)(VGAscr*, Point);

	int	doespanning;
};

/*
 */
struct VGAscr {
	Lock	devlock;
	VGAdev*	dev;
	Pcidev*	pci;

	VGAcur*	cur;
	ulong	storage;
	Cursor;

	int	useflush;

	ulong	paddr;		/* frame buffer */
	void*	vaddr;
	int		apsize;

	ulong	io;				/* device specific registers */
	ulong	*mmio;
	
	ulong	colormap[Pcolours][3];
	int	palettedepth;

	Memimage* gscreen;
	Memdata* gscreendata;
	Memsubfont* memdefont;

	int	(*fill)(VGAscr*, Rectangle, ulong);
	int	(*scroll)(VGAscr*, Rectangle, Rectangle);
	void	(*blank)(VGAscr*, int);
	ulong	id;	/* internal identifier for driver use */
	int isblank;
	int overlayinit;
};

extern VGAscr vgascreen[];

enum {
	Backgnd		= 0,	/* black */
};

/* mouse.c */
extern void mousectl(Cmdbuf*);
extern void mouseresize(void);

/* screen.c */
extern int		hwaccel;	/* use hw acceleration; default on */
extern int		hwblank;	/* use hw blanking; default on */
extern int		panning;	/* use virtual screen panning; default off */
extern void addvgaseg(char*, ulong, ulong);
extern uchar* attachscreen(Rectangle*, ulong*, int*, int*, int*);
extern void	flushmemscreen(Rectangle);
extern int	cursoron(int);
extern void	cursoroff(int);
extern void	setcursor(Cursor*);
extern int	screensize(int, int, int, ulong);
extern int	screenaperture(int, int);
extern Rectangle physgscreenr;	/* actual monitor size */
extern void	blankscreen(int);

extern VGAcur swcursor;
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

/* vga.c */
extern void	vgascreenwin(VGAscr*);
extern void	vgaimageinit(ulong);
extern void	vgalinearpciid(VGAscr*, int, int);
extern void	vgalinearpci(VGAscr*);
extern void	vgalinearaddr(VGAscr*, ulong, int);

extern void	drawblankscreen(int);
extern void	vgablank(VGAscr*, int);

extern Lock	vgascreenlock;

#define ishwimage(i)	(vgascreen[0].gscreendata && (i)->data->bdata == vgascreen[0].gscreendata->bdata)
