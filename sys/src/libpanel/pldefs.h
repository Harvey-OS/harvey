#define	nil	((void*)0)
/*
 * Definitions for internal use only
 */
/*
 * Variable-font text routines
 * These could make a separate library.
 */
int pl_rtfmt(Rtext *, int);
Rtext *pl_rtdraw(Bitmap *, Rectangle, Rtext *);
Rtext *pl_rthit(Rtext *, Point, Point);
Rtext *pl_rtscroll(Rtext *, Rtext *, int, int, int);
#define	HITME	0x08000		/* tells ptinpanel not to look at children */
#define	LEAF	0x10000		/* newpanel will refuse to attach children */
#define	INVIS	0x20000		/* don't draw this */
#define	REMOUSE	0x40000		/* send next mouse event here, even if not inside */
/*
 * States, also styles
 */
enum{
	UP,
	DOWN1,
	DOWN2,
	DOWN3,
	DOWN,
	PASSIVE,
	FRAME
};
/*
 * Scroll flags
 */
enum{
	SCROLLUP,
	SCROLLDOWN,
	SCROLLABSY,
	SCROLLLEFT,
	SCROLLRIGHT,
	SCROLLABSX,
};
/*
 * Scrollbar, slider orientations
 */
enum{
	HORIZ,
	VERT
};
Panel *pl_newpanel(Panel *, int);	/* make a new Panel, given parent & data size */
void *pl_emalloc(int);			/* allocate some space, exit on error */
void pl_print(Panel *);			/* print a Panel tree */
/*
 * Drawing primitives
 */
int pl_drawinit(int);
Rectangle pl_box(Bitmap *, Rectangle, int);
Rectangle pl_outline(Bitmap *, Rectangle, int);
Point pl_boxsize(Point, int);
void pl_interior(int, Point *, Point *);
void pl_drawicon(Bitmap *, Rectangle, int, int, Icon *);
Rectangle pl_check(Bitmap *, Rectangle, int);
Rectangle pl_radio(Bitmap *, Rectangle, int);
int pl_ckwid(void);
void pl_sliderupd(Bitmap *, Rectangle, int, int, int);
void pl_invis(Panel *, int);
Point pl_iconsize(int, Icon *);
/*
 * Rune mangling functions
 */
int pl_idchar(int);
int pl_rune1st(int);
char *pl_nextrune(char *);
int pl_runewidth(Font *, char *);
/*
 * Fixed-font Text-window routines
 * These could be separated out into a separate library.
 */
typedef struct Textwin Textwin;
struct Textwin{
	Rune *text, *etext, *eslack;	/* text, with some slack off the end */
	int top, bot;			/* range of runes visible on screen */
	int sel0, sel1;			/* selection */
	Point *loc, *eloc;		/* ul corners of visible runes (+1 more at end!) */
	Bitmap *b;			/* bitmap the text is drawn in */
	Rectangle r;			/* rectangle the text is drawn in */
	Font *font;			/* font text is drawn in */
	int hgt;			/* same as font->height */
	int tabstop;			/* tab settings are every tabstop pixels */
	int mintab;			/* the minimum size of a tab */
};
Textwin *twnew(Bitmap *, Font *, Rune *, int);
void twfree(Textwin *);
void twhilite(Textwin *, int, int, int);
void twselect(Textwin *, Mouse *);
void twreplace(Textwin *, int, int, Rune *, int);
void twscroll(Textwin *, int);
int twpt2rune(Textwin *, Point);
void twreshape(Textwin *, Rectangle);
