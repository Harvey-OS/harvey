typedef	struct	Box Box;
typedef	struct	Cimage Cimage;
typedef	struct	Column Column;
typedef	struct	Exec Exec;
typedef	struct	Line Line;
typedef	struct	Page Page;
typedef	struct	Row Row;
typedef	struct	Runestr Runestr;
typedef	struct	Text	Text;
typedef	struct	Timer Timer;
typedef	struct	Url Url;
typedef	struct	Window Window;

struct Runestr
{
	Rune	*r;
	int	nr;
};

enum
{
	Rowtag,
	Columntag,
	Tag,
	Urltag,
	Statustag,
	Entry,
	Textarea,
};

struct Text
{
	Frame;
	uint		org;
	uint		q0;
	uint		q1;
	int		what;
	Window	*w;
	Rectangle scrollr;
	Rectangle lastsr;
	Rectangle all;
	Row		*row;
	Column	*col;
	Runestr	rs;
};

uint		textbacknl(Text*, uint, uint);
int		textbswidth(Text*, Rune);
int		textclickmatch(Text*, int, int, int, uint*);
void		textclose(Text*);
void		textdelete(Text*, uint, uint);
void		textdoubleclick(Text*, uint*, uint*);
void		textfill(Text*);
void		textframescroll(Text*, int);
void		textinit(Text *, Image *, Rectangle, Font *, Image **);
void		textinsert(Text*, uint, Rune*, uint);
void		textredraw(Text *, Rectangle, Font *, Image *);
int		textresize(Text *, Image *, Rectangle);
void		textscrdraw(Text*);
void		textscroll(Text*, int);
void		textselect(Text*);
int		textselect2(Text *, uint *, uint *, Text **);
int		textselect3(Text *, uint *, uint *);
void		textset(Text *, Rune *, int);
void		textsetorigin(Text*, uint, int);
void		textsetselect(Text*, uint, uint);
void		textshow(Text*, uint, uint, int);
void		texttype(Text*, Rune);
void		textmouse(Text *, Point, int);

struct Line
{
	Rectangle	r;
	int		state;
	int		hastext;
	int		hastable;
	Box		*boxes;
	Box		*lastbox;
	Line		*prev;
	Line		*next;
};

struct Box
{
	Item 	*i;
	Rectangle	 r;

	void		(*draw)(Box *, Page *, Image *);
	void		(*mouse)(Box *, Page *, int);
	void		(*key)(Box *, Page *, Rune);
	Box		*prev;
	Box		*next;
};

Box*		boxalloc(Line *, Item *, Rectangle);
void		boxinit(Box *);

struct Lay
{
	Rectangle	r;
	int		width;
	int		xwall;
	Line		*lines;
	Line		*lastline;
	Font		*font;
	Ifloat	*floats;
	int		laying;
};

void		laypage(Page *p);
Lay*		layitems(Item *, Rectangle, int);
void		laydraw(Page *, Image *, Lay *);
void		layfree(Lay *);

struct Cimage
{
		Ref;
	Image	*i;
	Memimage *mi;
	Url		*url;
	Cimage	*next;
};

struct Url
{
		Ref;			/* urls in window.url[] are not freed */
	int		id;
	int		method;	/* HGet or HPost */
	Runestr	src;		/* requested url */
	Runestr	act;		/* actual url (redirection) */
	Runestr	post;		/* only set if method==HPost */
	Runestr	ctype;	/* content type */
};

Url*		urlalloc(Runestr *, Runestr *, int);
void		urlfree(Url *);
Url*		urldup(Url *);
int		urlopen(Url *);

struct Page
{
	Url		*url;
	Runestr	title;
	Window	*w;
	Image	*b;

	Rectangle	r;
	Rectangle all;
	Rectangle	vscrollr;
	Rectangle	hscrollr;
	Row		*row;
	Column	*col;

	Docinfo	*doc;
	Kidinfo	*kidinfo;
	Item		*items;
	Lay		*lay;
	Point		pos;

	int		selecting;
	Point		top, bot;
	Box		*topbx, *botbx;

	int		aborting;
	int		changed;
	int		loading;

	Rune		*status;

	Page		*parent;
	Page		*child;
	Page		*next;

	Cimage 	**cimage;
	int		ncimage;

	struct{
		long	t;
		Runestr rs;
	}refresh;
};

void		pageget(Page *, Runestr *, Runestr *, int, int);
void		pageload(Page *, Url *, int);
void		pageclose(Page *);
void		pageredraw(Page *);
void		pagerender(Page *);
void		pagemouse(Page *, Point, int);
void		pagetype(Page *, Rune, Point);
void		pagescrldraw(Page *);
void		pagescroll(Page *, int, int);
int		pagescrollxy(Page *, int, int);
int		pageabort(Page *);
void		pagesnarf(Page *);
void		pagesetrefresh(Page *);
int		pagerefresh(Page *);

struct Window
{
		Ref;
		QLock;
	Text		tag;
	Text		url;
	Page		page;
	Text		status;
	int		owner;
	int		inpage;
	Rectangle	r;
	Column	*col;
	struct{
		Url	**url;
		int	nurl;
		int	cid;
	}history;
};

void		wininit(Window *, Window *, Rectangle);
int		winclean(Window *, int);
void		winclose(Window *);
int		winresize(Window *, Rectangle, int);
Text*	wintext(Window *, Point);
void		winlock(Window *, int);
void		winunlock(Window *);
void		winaddhist(Window *, Url *);
void		wingohist(Window *, int);
void		winsettag(Window *);
void		winseturl(Window *);
void		winsetstatus(Window *w, Rune *);
Text*	wintype(Window *, Point, Rune);
Text*	winmouse(Window *, Point, int);
void		winmousebut(Window *);
void		windebug(Window *);

struct Column
{
	Rectangle r;
	Text	tag;
	Row		*row;
	Window	**w;
	int		nw;
	int		safe;
};

void		colinit(Column*, Rectangle);
Window*	coladd(Column*, Window*, Window*, int);
void		colclose(Column*, Window*, int);
void		colcloseall(Column*);
void		colresize(Column*, Rectangle);
Text*	colwhich(Column*, Point, Rune, int);
void		coldragwin(Column*, Window*, int);
void		colgrow(Column*, Window*, int);
int		colclean(Column*);
void		colsort(Column*);
void		colmousebut(Column*);

struct Row
{
	QLock;
	Rectangle r;
	Text	tag;
	Column	**col;
	int		ncol;

};

void		rowinit(Row*, Rectangle);
Column*	rowadd(Row*, Column *c, int);
void		rowclose(Row*, Column*, int);
Text*	rowwhich(Row*, Point, Rune, int);
Column*	rowwhichcol(Row*, Point);
void		rowresize(Row*, Rectangle);
void		rowdragcol(Row*, Column*, int but);

struct Exec
{
	char		*cmd;
	int		p[2];		/* p[1] is write to program; p[0] set to prog fd 0*/
	int		q[2];		/* q[0] is read from program; q[1] set to prog fd 1 */
	Channel	*sync;	/* chan(ulong) */
};

struct Timer
{
	int		dt;
	int		cancel;
	Channel	*c;		/* chan(int) */
	Timer	*next;
};

enum
{
	Scrollsize	=	12,
	Scrollgap	=	4,
	Margin =		4,
	Border =		2,
	Space  =		2,
	Tabspace =	30,
	Boxsize =		12,
	WFont =		FntR*NumSize+Tiny,

	Panspeed = 	4,
	Maxtab =		8,

	BUFSIZE =		1024*8,
	RBUFSIZE =	BUFSIZE/sizeof(Rune),
	STACK =		64*1024,
};

enum
{
	FALSE,
	TRUE,
	XXX,
};

enum
{
	Light = 0xEEEEEE,
	Dark = 0x666666,
	Red = 0xBB0000,
	Back = 0xCCCCCC,
};

Mouse		*mouse;
Mousectl		*mousectl;
Keyboardctl	*keyboardctl;
Image		*tagcols[NCOL];
Image		*textcols[NCOL];
Image		*but2col;
Image		*but3col;
Image		*button;
Image		*colbutton;
Font			*passfont;
Cursor		boxcursor;
Row			row;
Text			*argtext;
Text			*seltext;
Text			*typetext;
Page			*selpage;
Column		*activecol;
char			*webmountpt;
int			plumbsendfd;
int			webctlfd;
char			*charset;
int			procstderr;

enum
{
	Kscrolloneup		= KF|0x20,
	Kscrollonedown	= KF|0x21,
};

Channel		*cplumb;		/* chan(Plumbmsg*) */
Channel		*cexit;		/* chan(int) */
Channel		*crefresh;		/* chan(page *) */
