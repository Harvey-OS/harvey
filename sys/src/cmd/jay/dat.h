/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum
{
	Qdir,			/* /dev for this window */
	Qcons,
	Qconsctl,
	Qcursor,
	Qwdir,
	Qwinid,
	Qwinname,
	Qjayctl,
	Qkbdin,
	Qlabel,
	Qmouse,
	Qnew,
	Qscreen,
	Qsnarf,
	Qtext,
	Qwctl,
	Qwindow,
	Qwsys,		/* directory of window directories */
	Qwsysdir,		/* window directory, child of wsys */

	QMAX,
};

enum
{
	Kscrolloneup = KF|0x20,
	Kscrollonedown = KF|0x21,
};

#define	STACK	8192

typedef	struct	Consreadmesg Consreadmesg;
typedef	struct	Conswritemesg Conswritemesg;
typedef	struct	Stringpair Stringpair;
typedef	struct	Dirtab Dirtab;
typedef	struct	Fid Fid;
typedef	struct	Filsys Filsys;
typedef	struct	Mouseinfo	Mouseinfo;
typedef	struct	Mousereadmesg Mousereadmesg;
typedef	struct	Mousestate	Mousestate;
typedef	struct	Ref Ref;
typedef	struct	Timer Timer;
typedef	struct	Wctlmesg Wctlmesg;
typedef	struct	Window Window;
typedef	struct	Xfid Xfid;
typedef	struct TPanel TPanel;
typedef struct MenuEntry MenuEntry;
typedef struct StartMenu StartMenu;

enum
{
	Selborder		= 4,		/* border of selected window */
	Unselborder	= 1,		/* border of unselected window */
	Scrollwid 		= 12,		/* width of scroll bar */
	Scrollgap 		= 4,		/* gap right of scroll bar */
	BIG			= 3,		/* factor by which window dimension can exceed screen */
	TRUE		= 1,
	FALSE		= 0,
};

#define	QID(w,q)	((w<<8)|(q))
#define	WIN(q)	((((u32)(q).path)>>8) & 0xFFFFFF)
#define	FILE(q)	(((u32)(q).path) & 0xFF)

enum	/* control messages */
{
	Wakeup,
	Reshaped,
	Moved,
	Refresh,
	Movemouse,
	Rawon,
	Rawoff,
	Holdon,
	Holdoff,
	Deleted,
	Exited,
};

enum /* Position */
{
	Up,
	Down,
	Left,
	Right,
};

struct Wctlmesg
{
	int		type;
	Rectangle	r;
	Image	*image;
};

struct Conswritemesg
{
	Channel	*cw;		/* chan(Stringpair) */
};

struct Consreadmesg
{
	Channel	*c1;		/* chan(tuple(char*, int) == Stringpair) */
	Channel	*c2;		/* chan(tuple(char*, int) == Stringpair) */
};

struct Mousereadmesg
{
	Channel	*cm;		/* chan(Mouse) */
};

struct Stringpair	/* rune and nrune or byte and nbyte */
{
	void		*s;
	int		ns;
};

struct Mousestate
{
	Mouse mouse;
	u32	counter;	/* serial no. of mouse event */
};

struct Mouseinfo
{
	Mousestate	queue[16];
	int	ri;	/* read index into queue */
	int	wi;	/* write index */
	u32	counter;	/* serial no. of last mouse event we received */
	u32	lastcounter;	/* serial no. of last mouse event sent to client */
	int	lastb;	/* last button state we received */
	unsigned char	qfull;	/* filled the queue; no more recording until client comes back */
};

struct TPanel{
	Rectangle r; // Total TPanel
	Image *sb; // StartButton size
	Image *i;
	int position;
};

struct MenuEntry {
  char *name;
  char *action;
  Image *i; //normal
  Image *ih; //hoover
  StartMenu *submenu;
};

struct StartMenu {
  int numEntries;
  MenuEntry *entries[50];
  Image *i;
};

struct Window
{
	Ref Ref;
	QLock QLock;
	Frame Frame;
	Image		*i; //functional part
	Image		*t; //title part
	Rectangle r; //i+t
	Rectangle bc; //button close
	Rectangle bmax; //button maximize
	Rectangle bmin; //button minimize
	Mousectl		mc;
	Mouseinfo	mouse;
	Channel		*ck;			/* chan(Rune[10]) */
	Channel		*cctl;		/* chan(Wctlmesg)[20] */
	Channel		*conswrite;	/* chan(Conswritemesg) */
	Channel		*consread;	/* chan(Consreadmesg) */
	Channel		*mouseread;	/* chan(Mousereadmesg) */
	Channel		*wctlread;		/* chan(Consreadmesg) */
	u32			nr;			/* number of runes in window */
	u32			maxr;		/* number of runes allocated in r */
	Rune			*run;
	u32			nraw;
	Rune			*raw;
	u32			org;
	u32			q0;
	u32			q1;
	u32			qh;
	int			id;
	char			name[32];
	u32			namecount;
	Rectangle		scrollr;
	/*
	 * Jay once used originwindow, so screenr could be different from i->r.
	 * Now they're always the same but the code doesn't assume so.
	*/
	Rectangle		screenr;	/* screen coordinates of window */
	int			resized;
	int			wctlready;
	Rectangle		lastsr;
	int			topped;
	int			notefd;
	unsigned char		scrolling;
	Cursor		cursor;
	Cursor		*cursorp;
	unsigned char		holding;
	unsigned char		rawing;
	unsigned char		ctlopen;
	unsigned char		wctlopen;
	unsigned char		deleted;
	unsigned char		mouseopen;
	char			*label;
	int			pid;
	char			*dir;
	int		isVisible;

	Rectangle originalr;
	int maximized;
};

int		winborder(Window*, Point);
Image *winspace(Window *w);
void		winctl(void*);
void		winshell(void*);
Window*	wlookid(int);
Window*	wmk(Image*, Mousectl*, Channel*, Channel*, int);
Window*	wpointto(Point);
Window*	wtop(Point);
void		wtopme(Window*);
void		wbottomme(Window*);
char*	wcontents(Window*, int*);
int		wbswidth(Window*, Rune);
int		wclickmatch(Window*, int, int, int, u32*);
int		wclose(Window*);
int		wctlmesg(Window*, int, Rectangle, Image*);
int		wctlmesg(Window*, int, Rectangle, Image*);
u32		wbacknl(Window*, u32, u32);
u32		winsert(Window*, Rune*, int, u32);
void		waddraw(Window*, Rune*, int);
void		wborder(Window*, int);
void		wclosewin(Window*);
void		wcurrent(Window*);
void		wcut(Window*);
void		wdelete(Window*, u32, u32);
void		wdoubleclick(Window*, u32*, u32*);
void		wfill(Window*);
void		wframescroll(Window*, int);
void		wkeyctl(Window*, Rune);
void		wmousectl(Window*);
void		wmovemouse(Window*, Point);
void		wpaste(Window*);
void		wplumb(Window*);
void		wrefresh(Window*, Rectangle);
void		wrepaint(Window*);
void		wresize(Window*, Image*, int);
void		wscrdraw(Window*);
void		wscroll(Window*, int);
void		wselect(Window*);
void		wsendctlmesg(Window*, int, Rectangle, Image*);
void		wsetcursor(Window*, int);
void		wsetname(Window*);
void		wsetorigin(Window*, u32, int);
void		wsetpid(Window*, int, int);
void		wsetselect(Window*, u32, u32);
void		wshow(Window*, u32);
void		wsnarf(Window*);
void 		wscrsleep(Window*, u32);
void		wsetcols(Window*);
void		wcurrentnext();
void		whooverbutton(Point p, Window *w);
Rectangle windowMinusTitle(Rectangle r, Image *t);

struct Dirtab
{
	char		*name;
	unsigned char	type;
	u32		qid;
	u32		perm;
};

struct Fid
{
	int		fid;
	int		busy;
	int		open;
	int		mode;
	Qid		qid;
	Window	*w;
	Dirtab	*dir;
	Fid		*next;
	int		nrpart;
	unsigned char	rpart[UTFmax];
};

struct Xfid
{
		Ref Ref;
		Xfid		*next;
		Xfid		*free;
		Fcall Fcall;
		Channel	*c;	/* chan(void(*)(Xfid*)) */
		Fid		*f;
		unsigned char	*buf;
		Filsys	*fs;
		QLock	active;
		int		flushing;	/* another Xfid is trying to flush us */
		int		flushtag;	/* our tag, so flush can find us */
		Channel	*flushc;	/* channel(int) to notify us we're being flushed */
};

Channel*	xfidinit(void);
void		xfidctl(void*);
void		xfidflush(Xfid*);
void		xfidattach(Xfid*);
void		xfidopen(Xfid*);
void		xfidclose(Xfid*);
void		xfidread(Xfid*);
void		xfidwrite(Xfid*);

enum
{
	Nhash	= 16,
};

struct Filsys
{
		int		cfd;
		int		sfd;
		int		pid;
		char		*user;
		Channel	*cxfidalloc;	/* chan(Xfid*) */
		Fid		*fids[Nhash];
};

Filsys*	filsysinit(Channel*);
int		filsysmount(Filsys*, int);
Xfid*		filsysrespond(Filsys*, Xfid*, Fcall*, char*);
void		filsyscancel(Xfid*);

void		wctlproc(void*);
void		wctlthread(void*);

void		deletetimeoutproc(void*);

struct Timer
{
	int		dt;
	int		cancel;
	Channel	*c;	/* chan(int) */
	Timer	*next;
};

Font		*font;
Mousectl	*mousectl;
Mouse	*mouse;
Keyboardctl	*keyboardctl;
Display	*display;
Image	*view;
Screen	*wscreen;
Cursor	defcursor;
Cursor	boxcursor;
Cursor	crosscursor;
Cursor	sightcursor;
Cursor	whitearrow;
Cursor	query;
Cursor	*corners[9];
Image	*background;
Image	*lightgrey;
Image	*red;
Window	**window;
Window	*wkeyboard;	/* window of simulated keyboard */
int		nwindow;
int		snarffd;
Window	*input;
QLock	all;			/* BUG */
Filsys	*filsys;
Window	*hidden[100];
int		nhidden;
int		nsnarf;
Rune*	snarf;
int		scrolling;
int		maxtab;
Channel*	winclosechan;
Channel*	deletechan;
char		*startdir;
int		sweeping;
int		wctlfd;
char		srvpipe[64];
char		srvwctl[64];
int		errorshouldabort;
int		menuing;		/* menu action is pending; waiting for window to be indicated */
int		snarfversion;	/* updated each time it is written */
int		messagesize;		/* negotiated in 9P version setup */

// Window parts
Image *closebutton, *closebuttonhoover;
Image *minimizebutton, *minimizebuttonhoover;
Image *maximizebutton, *maximizebuttonhoover;


TPanel *taskPanel;
Rectangle windowspace;

void		initpanel();
void		redrawpanel();
void		clickpanel(Point p, TPanel *tp);

StartMenu		*loadStartMenu(TPanel *p);
void		freeStartMenu(StartMenu *sm);
void		hoovermenu(StartMenu *sm, Point p);
void		execmenu(StartMenu *sm, Point p);

//Colors
Image *blk;
Image *wht;

//Config
Jayconfig *jayconfig;

char *getjayconfig();
void setjayconfig(char *conf);
void jayredraw(void);
