#ifndef __LIBG_H
#define __LIBG_H
#ifndef _LIBXG_EXTENSION
#ifndef _LIBG_EXTENSION
    This header file is an extension to ANSI/POSIX
#endif
#endif
#pragma lib "/$M/lib/ape/libg.a"
/*
 *  you may think it's a blit, but it's gnot
 *
 *  like Plan 9 libg, but had to rename div -> ptdiv,
 *  and don't assume uchar defined
 */

enum{ EMAXMSG = 128+8192 };	/* max size of a 9p message including data */

/*
 * Types
 */

typedef	struct	Bitmap		Bitmap;
typedef struct	Point		Point;
typedef struct	Rectangle 	Rectangle;
typedef struct	Cursor		Cursor;
typedef struct	Mouse		Mouse;
typedef struct	Menu		Menu;
typedef struct	Fontchar	Fontchar;
typedef struct	Font		Font;
typedef struct	Event		Event;
typedef struct	RGB		RGB;
typedef void	 (*Errfunc)(char *);

struct	Point
{
	int	x;
	int	y;
};

struct Rectangle
{
	Point min;
	Point max;
};

struct	Bitmap
{
	Rectangle r;		/* rectangle in data area, local coords */
	int	ldepth;
	int	id;
	Bitmap	*cache;		/* zero; distinguishes bitmap from layer */
};

struct	Mouse
{
	int	buttons;	/* bit array: LMR=124 */
	Point	xy;
};

struct	Cursor
{
	Point		offset;
	unsigned char	clr[2*16];
	unsigned char	set[2*16];
#ifdef _LIBXG_EXTENSION
	int		id;	/* init to zero; used by library */
#endif
};

struct Menu
{
	char	**item;
	char	*(*gen)(int);
	int	lasthit;
};

/*
 * Fonts
 *
 * given char c, Font *f, Fontchar *i, and Point p, one says
 *	i = f->info+c;
 *	bitblt(b, Pt(p.x+i->left,p.y),
 *		bitmap, Rect(i->x,i->top,(i+1)->x,i->bottom),
 *		fc);
 *	p.x += i->width;
 * where bitmap is the repository of the glyphs.
 *
 */

struct	Fontchar
{
	short		x;		/* left edge of bits */
	unsigned char	top;		/* first non-zero scan-line */
	unsigned char	bottom;		/* last non-zero scan-line */
	char		left;		/* offset of baseline */
	unsigned char	width;		/* width of baseline */
};

struct	Font
{
	short		n;		/* number of chars in font */
	unsigned char	height;		/* height of bitmap */
	char		ascent;		/* top of bitmap to baseline */
	Fontchar 	*info;		/* n+1 character descriptors */
	int		id;		/* of font */
};

struct	Event
{
	int		kbdc;
	Mouse		mouse;
	int		n;		/* number of characters in mesage */
	unsigned char	data[EMAXMSG];	/* message from an arbitrary file descriptor */
};

struct RGB
{
	unsigned long	red;
	unsigned long	green;
	unsigned long	blue;
};

/*
 * Codes for bitblt etc.
 *
 *	       D
 *	     0   1
 *         ---------
 *	 0 | 1 | 2 |
 *     S   |---|---|
 * 	 1 | 4 | 8 |
 *         ---------
 *
 *	Usually used as D|S; DorS is so tracebacks are readable.
 */
typedef
enum	Fcode
{
	Zero		= 0x0,
	DnorS		= 0x1,
	DandnotS	= 0x2,
	notS		= 0x3,
	notDandS	= 0x4,
	notD		= 0x5,
	DxorS		= 0x6,
	DnandS		= 0x7,
	DandS		= 0x8,
	DxnorS		= 0x9,
	D		= 0xA,
	DornotS		= 0xB,
	S		= 0xC,
	notDorS		= 0xD,
	DorS		= 0xE,
	F		= 0xF,
	SUB		= 0x10,		/* balu arithmetic codes */
	SSUB		= 0x11,
	ADD		= 0x12,
	SADD		= 0x13,
	MIN		= 0x14,
	MAX		= 0x15
} Fcode;

/*
 * Miscellany
 */

extern Point		add(Point, Point);
extern Point		sub(Point, Point);
extern Point		mul(Point, int);
extern Point		ptdiv(Point, int);
extern Rectangle	rsubp(Rectangle, Point);
extern Rectangle	raddp(Rectangle, Point);
extern Rectangle	inset(Rectangle, int);
extern Rectangle	rmul(Rectangle, int);
extern Rectangle	rdiv(Rectangle, int);
extern Rectangle	rshift(Rectangle, int);
extern Rectangle	rcanon(Rectangle);
extern Bitmap*		balloc(Rectangle, int);
extern void		bfree(Bitmap*);
extern int		rectclip(Rectangle*, Rectangle);
#ifdef _LIBG_EXTENSION
extern void		binit(void(*)(char*), char*, char*);
#endif
#ifdef _LIBXG_EXTENSION
extern void		xtbinit(Errfunc, char*, int*, char**);
#endif
extern void		bclose(void);
extern void		berror(char*);
extern void		bitblt(Bitmap*, Point, Bitmap*, Rectangle, Fcode);
extern void		bitbltclip(void*);
extern Font*		falloc(int, int, int, Fontchar*, Bitmap*);
extern void		ffree(Font*);
extern Point		string(Bitmap*, Point, Font*, char*, Fcode);
extern void		segment(Bitmap*, Point, Point, int, Fcode);
extern void		point(Bitmap*, Point, int, Fcode);
extern void		arc(Bitmap*, Point, Point, Point, int, Fcode);
extern void		circle(Bitmap*, Point, int, int, Fcode);
extern void		disc(Bitmap*, Point, int, int, Fcode);
extern void		ellipse(Bitmap*, Point, int, int, int, Fcode);
extern long		strwidth(Font*, char*);
extern Point		strsize(Font*, char*);
extern long		charwidth(Font*, unsigned short);
extern void		texture(Bitmap*, Rectangle, Bitmap*, Fcode);
extern void		wrbitmap(Bitmap*, int, int, unsigned char*);
extern void		rdbitmap(Bitmap*, int, int, unsigned char*);
extern void		wrbitmapfile(int, Bitmap*);
extern Bitmap*		rdbitmapfile(int);
extern void		wrfontfile(int, Font*);
extern Font*		rdfontfile(int, Bitmap*);
extern void		rdcolmap(Bitmap*, RGB*);
extern void		wrcolmap(Bitmap*, RGB*);
extern int		ptinrect(Point, Rectangle);
extern int		rectXrect(Rectangle, Rectangle);
extern int		eqpt(Point, Point);
extern int		eqrect(Rectangle, Rectangle);
extern void		border(Bitmap*, Rectangle, int, Fcode);
extern void		cursorswitch(Cursor*);
extern void		cursorset(Point);
extern Rectangle	bscreenrect(void);
extern unsigned char*	bneed(int);
extern void		bflush(void);
extern void		bexit(void);
extern int		bwrite(void);

extern void		einit(unsigned long);
extern unsigned long	estart(unsigned long, int, int);
extern unsigned long	etimer(unsigned long, int);
extern unsigned long	event(Event*);
extern unsigned long	eread(unsigned long, Event*);
extern Mouse		emouse(void);
extern int		ekbd(void);
extern int		ecanread(unsigned long);
extern int		ecanmouse(void);
extern int		ecankbd(void);
extern void		ereshaped(Rectangle);	/* supplied by user */
extern int		menuhit(int, Mouse*, Menu*);
extern Rectangle	getrect(int, Mouse*);
extern unsigned long	rgbpix(Bitmap*, RGB);
enum{
	Emouse		= 1,
	Ekeyboard	= 2,
};

#define	Pt(x, y)		((Point){(x), (y)})
#define	Rect(x1, y1, x2, y2)	((Rectangle){Pt(x1, y1), Pt(x2, y2)})
#define	Rpt(p1, p2)		((Rectangle){(p1), (p2)})


#define	Dx(r)	((r).max.x-(r).min.x)
#define	Dy(r)	((r).max.y-(r).min.y)

extern	int	bitbltfd;
extern	Bitmap	screen;
extern	Font	*font;

#define	BGSHORT(p)		(((p)[0]<<0) | ((p)[1]<<8))
#define	BGLONG(p)		((BGSHORT(p)<<0) | (BGSHORT(p+2)<<16))
#define	BPSHORT(p, v)		((p)[0]=(v), (p)[1]=((v)>>8))
#define	BPLONG(p, v)		(BPSHORT(p, (v)), BPSHORT(p+2, (v)>>16))
#endif
