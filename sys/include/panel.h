#pragma	src	"/sys/src/libpanel"
#pragma	lib	"libpanel.a"
typedef struct Scroll Scroll;
typedef struct Panel Panel;		/* a Graphical User Interface element */
typedef struct Rtext Rtext;		/* formattable text */
typedef void Icon;			/* Always used as Icon * -- Bitmap or char */
typedef struct Idol Idol;		/* A picture/text combo */
struct Scroll{
	Point pos, size;
};
struct Rtext{
	int hot;		/* responds to hits? */
	void *user;		/* user data */
	int space;		/* how much space before, if no break */
	int indent;		/* how much space before, after a break */
	Bitmap *b;		/* what to display, if nonzero */
	Panel *p;		/* what to display, if nonzero and b==0 */
	Font *font;		/* font in which to draw text */
	char *text;		/* what to display, if b==0 and p==0 */
	Rtext *next;		/* next piece */
	/* private below */
	Rtext *nextline;	/* links line to line */
	Rtext *last;		/* last, for append */
	Rectangle r;		/* where to draw, if origin were Pt(0,0) */
	int topy;		/* y coord of top of line */
	int wid;		/* not including space */
};
struct Panel{
	Point ipad, pad;				/* extra space inside and outside */
	Point fixedsize;				/* size of Panel, if FIXED */
	int user;					/* available for user */
	void *userp;					/* available for user */
	Rectangle r;					/* where the Panel goes */
	/* private below */
	Panel *next;					/* It's a list! */
	Panel *child, *echild, *parent;			/* No, it's a tree! */
	Bitmap *b;					/* where we're drawn */
	int flags;					/* position flags, see below */
	char *kind;					/* what kind of panel? */
	int state;					/* for hitting & drawing purposes */
	Point size;					/* space for this Panel */
	Point sizereq;					/* size requested by this Panel */
	Point childreq;					/* total size needed by children */
	Panel *lastmouse;				/* who got the last mouse event? */
	Panel *scrollee;				/* pointer to scrolled window */
	Panel *xscroller, *yscroller;			/* pointers to scroll bars */
	Scroll scr;					/* scroll data */
	void *data;					/* kind-specific data */
	void (*draw)(Panel *);				/* draw panel and children */
	int (*hit)(Panel *, Mouse *);			/* process mouse event */
	void (*type)(Panel *, Rune);			/* process keyboard event */
	Point (*getsize)(Panel *, Point);		/* return size, given child size */
	void (*childspace)(Panel *, Point *, Point *);	/* child ul & size given our size */
	void (*scroll)(Panel *, int, int, int, int);	/* scroll bar to scrollee */
	void (*setscrollbar)(Panel *, int, int, int);	/* scrollee to scroll bar */
	void (*free)(Panel *);				/* free fields of data when done */
};
/*
 * Panel flags -- there are more private flags in panelprivate.h
 * that need to be kept synchronized with these!
 */
#define	PACK	0x0007		/* which side of the parent is the Panel attached to? */
#define		PACKN	0x0000
#define		PACKE	0x0001
#define		PACKS	0x0002
#define		PACKW	0x0003
#define		PACKCEN	0x0004	/* only used by pulldown */
#define	FILLX	0x0008		/* grow horizontally to fill the available space */
#define	FILLY	0x0010		/* grow vertically to fill the available space */
#define	PLACE	0x01e0		/* which side of its space should the Panel adhere to? */
#define		PLACECEN 0x0000
#define		PLACES	0x0020
#define		PLACEE	0x0040
#define		PLACEW	0x0060
#define		PLACEN	0x0080
#define		PLACENE	0x00a0
#define		PLACENW	0x00c0
#define		PLACESE	0x00e0
#define		PLACESW	0x0100
#define	EXPAND	0x0200		/* use up all extra space in the parent */
#define	FIXED	0x0c00		/* don't pass children's size requests through to parent */
#define	FIXEDX	0x0400
#define	FIXEDY	0x0800
#define	MAXX	0x1000		/* make x size as big as biggest sibling's */
#define	MAXY	0x2000		/* make y size as big as biggest sibling's */
#define	BITMAP	0x4000		/* text argument is a bitmap, not a string */
/*
 * An extra bit in Mouse.buttons
 */
#define	OUT	8			/* Mouse.buttons bit, set when mouse leaves Panel */
int plinit(int);			/* initialization */
int plpack(Panel *, Rectangle);		/* figure out where to put the Panel & children */
void plmove(Panel *, Point);		/* move an already-packed panel to a new location */
void pldraw(Panel *, Bitmap *);		/* display the panel on the bitmap */
void plfree(Panel *);			/* give back space */
void plgrabkb(Panel *);			/* this Panel should receive keyboard events */
void plkeyboard(Rune);			/* send a keyboard event to the appropriate Panel */
void plmouse(Panel *, Mouse);		/* send a Mouse event to a Panel tree */
void plscroll(Panel *, Panel *, Panel *); /* link up scroll bars */
char *plentryval(Panel *);		/* entry delivers its value */
void plsetbutton(Panel *, int);		/* set or clear the mark on a button */
void plsetslider(Panel *, int, int);	/* set the value of a slider */
Rune *pleget(Panel *);			/* get the text from an edit window */
int plelen(Panel *);			/* get the length of the text from an edit window */
void plegetsel(Panel *, int *, int *);	/* get the selection from an edit window */
void plepaste(Panel *, Rune *, int);	/* paste in an edit window */
void plesel(Panel *, int, int);		/* set the selection in an edit window */
void plescroll(Panel *, int);		/* scroll an edit window */
Scroll plgetscroll(Panel *);		/* get scrolling information from panel */
void plsetscroll(Panel *, Scroll);	/* set scrolling information */
void plplacelabel(Panel *, int);	/* label placement */
/*
 * Panel creation & reinitialization functions
 */
Panel *plbutton(Panel *pl, int, Icon *, void (*)(Panel *pl, int));
Panel *plcanvas(Panel *pl, int, void (*)(Panel *), void (*)(Panel *pl, Mouse *));
Panel *plcheckbutton(Panel *pl, int, Icon *, void (*)(Panel *pl, int, int));
Panel *pledit(Panel *, int, Point, Rune *, int, void (*)(Panel *));
Panel *plentry(Panel *pl, int, int, char *, void (*)(Panel *pl, char *));
Panel *plframe(Panel *pl, int);
Panel *plgroup(Panel *pl, int);
Panel *plidollist(Panel*, int, Point, Font*, Idol*, void (*)(Panel*, int, void*));
Panel *pllabel(Panel *pl, int, Icon *);
Panel *pllist(Panel *pl, int, char *(*)(Panel *, int), int, void(*)(Panel *pl, int, int));
Panel *plmenu(Panel *pl, int, Icon **, int, void (*)(int, int));
Panel *plmenubar(Panel *pl, int, int, Icon *, Panel *pl, Icon *, ...);
Panel *plmessage(Panel *pl, int, int, char *);
Panel *plpopup(Panel *pl, int, Panel *pl, Panel *pl, Panel *pl);
Panel *plpulldown(Panel *pl, int, Icon *, Panel *pl, int);
Panel *plradiobutton(Panel *pl, int, Icon *, void (*)(Panel *pl, int, int));
Panel *plscrollbar(Panel *plparent, int flags);
Panel *plslider(Panel *pl, int, Point, void(*)(Panel *pl, int, int, int));
Panel *pltextview(Panel *, int, Point, Rtext *, void (*)(Panel *, int, Rtext *));
void plinitbutton(Panel *, int, Icon *, void (*)(Panel *, int));
void plinitcanvas(Panel *, int, void (*)(Panel *), void (*)(Panel *, Mouse *));
void plinitcheckbutton(Panel *, int, Icon *, void (*)(Panel *, int, int));
void plinitedit(Panel *, int, Point, Rune *, int, void (*)(Panel *));
void plinitentry(Panel *, int, int, char *, void (*)(Panel *, char *));
void plinitframe(Panel *, int);
void plinitgroup(Panel *, int);
void plinitidollist(Panel*, int, Point, Font*, Idol*, void (*)(Panel*, int, void*));
void plinitlabel(Panel *, int, Icon *);
void plinitlist(Panel *, int, char *(*)(Panel *, int), int, void(*)(Panel *, int, int));
void plinitmenu(Panel *, int, Icon **, int, void (*)(int, int));
void plinitmessage(Panel *, int, int, char *);
void plinitpopup(Panel *, int, Panel *, Panel *, Panel *);
void plinitpulldown(Panel *, int, Icon *, Panel *, int);
void plinitradiobutton(Panel *, int, Icon *, void (*)(Panel *, int, int));
void plinitscrollbar(Panel *parent, int flags);
void plinitslider(Panel *, int, Point, void(*)(Panel *, int, int, int));
void plinittextview(Panel *, int, Point, Rtext *, void (*)(Panel *, int, Rtext *));
/*
 * Rtext constructors & destructor
 */
Rtext *plrtstr(Rtext **, int, int, Font *, char *, int, void *);
Rtext *plrtbitmap(Rtext **, int, int, Bitmap *, int, void *);
Rtext *plrtpanel(Rtext **, int, int, Panel *, void *);
void plrtfree(Rtext *);
Rtext *plgetpostextview(Panel *);
void plsetpostextview(Panel *, Rtext *);
/*
 * Idols
 */
Idol *plmkidol(Idol**, Bitmap*, Bitmap*, char*, void*);
void plfreeidol(Idol*);
Point plidolsize(Idol*, Font*, int);
void *plidollistgetsel(Panel*);
