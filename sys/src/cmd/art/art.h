/*
 * definitions used in more than one file
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#define	DPI	100.
typedef double Flt;
typedef struct Dpoint Dpoint;
typedef struct Drectangle Drectangle;
typedef struct Item Item;
typedef struct Typeface Typeface;
typedef struct Itemfns Itemfns;
typedef struct Align Align;
typedef struct Grid Grid;
typedef struct Alpt Alpt;
typedef struct Scsave Scsave;
struct Dpoint{
	Flt x, y;
};
#define	ORDER	4
typedef Dpoint Bezier[ORDER];
#define	Dpt(x,y)	((Dpoint){(x), (y)})
struct Drectangle{
	Dpoint min, max;
};
Drectangle dwgdrect;			/* screen dimensions in drawing coordinates */
#define	Drect(x0, y0, x1, y1)	((Drectangle){(Dpoint){(x0), (y0)}, (Dpoint){(x1), (y1)}})
#define	Drpt(p, q)	((Drectangle){p, q})
/*
 * Fig edits scenes composed of lines, circular arcs, circles, boxes and text.
 */
struct Item{
	Item *next, *prev;	/* circular connections within group */
	Item *orig;		/* whence the prototype for this object came */
	Item *near;		/* links on near list */
	int np;			/* length of p */
	Dpoint *p;		/* p[0] is start point of text, center of circle */
				/* p[0], p[1] are endpoints of lines, corners of boxes */
				/* p[0], p[1], p[2] are start, middle and end of arcs */
	Dpoint nearpt;		/* point near test point */
	Flt r;			/* radius of circles */
	Typeface *face;		/* font if text */
	char *text;		/* characters of text */
	int group;		/* index in group table */
	char style;		/* line style */
	char type;		/* what kind of object */
	char flags;		/* various Boolean annotations */
	Itemfns *fn;		/* object-orientedness */
};
/*
 * item types
 */
#define	HEAD	0
#define	LINE	1
#define	ARC	2
#define	CIRCLE	3
#define	TEXT	4
#define	BOX	5
#define	SPLINE	6
#define	GROUP	7
/*
 * item flags
 */
#define	MOVING	1			/* item is moving, therefore no gravity */
#define	HOT	2			/* item generates alignments */
#define	BOXED	4			/* item is boxed */
#define	FLAT	8			/* item was created by flattening scene */
#define	INVIS	16			/* don't draw this! */
/*
 * line style bits
 */
#define	ARROW0	1			/* arrowhead at p[0] */
#define	ARROW1	2			/* arrowhead at p[np-1] */
#define	DASH	4			/* dashed line */
#define	DOT	8			/* dotted line */
struct Itemfns{
	void (*delete)(Item *);				/* delete item, free up storage */
	void (*write)(Item *, int);			/* put in file */
	void (*activate)(Item *);			/* indicate hot points and lines */
	Dpoint (*near)(Item *, Dpoint);			/* item point nearest test point */
	void (*draw)(Item *, int, Bitmap *, Fcode);	/* draw the item */
	void (*edit)(void);				/* alter item with mouse */
	void (*translate)(Item *, Dpoint);		/* transform item by translation */
	int (*inbox)(Item *, Drectangle);		/* does object intersect box? */
	Drectangle (*bbox)(Item *);			/* return object's bounding box */
	Dpoint (*nearvert)(Item *, Dpoint);		/* item vertex nearest test point */
};
struct Typeface{
	char *name;
	char *file;
	Font *font;
};
#define	NBUTTON	16
struct Align{
	Flt value;
	char active;
	char button[NBUTTON];
};
#define	NGBUTTON	60
struct Grid{
	Dpoint origin;
	Dpoint delta;
	char button[NGBUTTON];
};
struct Alpt{
	Dpoint p;
	Item *on;			/* an item that alpt is on */
	Alpt *next;
};
struct Scsave{
	Item *scene;
	Dpoint scoffs;
};
Typeface *curface;
Item *scene;		/* the whole drawing */
Dpoint curpt;
Dpoint anchor;
Flt gravity; /* radius within which active objects attract, off if zero */
#define	NALIGN	20
#define	SLOPE	0
#define	ANGLE	1
#define	PARA	2
#define	CIRC	3
Align align[4][NALIGN];
int nalign[4];
#define	NGRID	20
Grid grid[NGRID];
int ngrid;
int gridsel;
#define	NARG	3
Dpoint arg[NARG];
int narg;
#define	NCMD	128
Rune rcmd[NCMD+2];
char cmd[3*(NCMD+2)];
char *cmdname;
Item *selection;			/* the item closest to curpt */
Item *active;				/* alignment objects */
Alpt *alpt;
Bitmap *cur;				/* caret bitmap */
Bitmap *anchormark;			/* anchor bitmap */
Bitmap *sel;
Bitmap *bg;
Font *fontp;				/* font for msg */
int mousefd;				/* fd of /dev/dmouse */
int mousectl;				/* fd of /dev/mousectl */
Bitmap display;				/* display bitmap */
Rectangle dwgbox;			/* drawing area */
Rectangle echobox;			/* character echo area */
Rectangle msgbox;			/* message area */
Mouse mouse;				/* mouse data */
int heatnew;				/* should new or edited items be heated */
int faint, dark;			/* colors of faint and dark items */
#define	radians(deg)	((deg)*.0174532925199432957)
#define	degrees(rad)	((rad)*57.29577951308232103)
#define	NGROUP	100
Item *group[NGROUP];
int grpmap[NGROUP];			/* mapping between internal & file group nums */
int ngrp;
Dpoint scoffs;				/* offset at which to draw scene */
Dpoint svpoint;				/* previous point, used in moveall */
#define	NSCSTACK	128
Scsave scstack[NSCSTACK];
Scsave *scsp;
#define	CLOSE	(.1/DPI)	/* closeness of coordinates, see same(.,.) */
#define	SMALL	1e-5		/* avoid divide check, see circumcenter, pldist, etc. */
Itemfns splinefns;
#define	L	1		/* heat left endpoint of line in hotline */
#define	R	2		/* heat right endpoint of line in hotline */
#include "proto.h"
