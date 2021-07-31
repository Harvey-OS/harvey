
enum
{
	MsgMouse = 1,
	MsgKeybd,
	MsgRefresh,
	MsgMenu1,
	MsgMenu2,
	MsgReshape,
	MsgClose,
	MsgFlush,
	MsgIO,
	MsgUnhide,
};

enum
{
	Hidemax = 20,
	Cbsize	= 1024,
	Dispbuf = 16384,
	Keybbuf = 2048,
	Mouse_r = 0x4,
	Mouse_m = 0x2,
	Mouse_l = 0x1,
	On,
	Off,
	Read,
	Write,
	Eot	= 0x04,
	View	= 0x80,
	BS	= 0x08,
	CtrlU	= 0x15,
	CtrlW	= 0x17,
	DEL	= 0x7f,
	ESC	= 0x1b,
};

enum
{
	Qdir = 1,
	Qbitblt,
	Qcons,
	Qctl,
	Qmouse,
	Qnbmouse,
	Qsnarf,
	Qwindow,
	Qlabel,
	NQid
};

enum
{
	Cut = 1,
	Paste,
	Snarf,
	Send,
	Swap,
};

aggr Rlist
{
	Rectangle;
	Rlist *next;
};

aggr Ioreq
{
	Fcall	fcall;
	int	file;
	Ioreq	*link;
	char	buf[MAXRPC+UTFmax];
};

typedef aggr Window;
aggr Mesg
{
	char	type;
	uint	time;
	Mesg	*link;
	Mouse;
	union {
		int	sel;
		aggr{
			char		**menu;
			chan(Mesg)	rchan;
		};
		Rune	keyb;
		Rlist	*refresh;
		Ioreq	*io;
	};
};

aggr Buf
{
	Rune	*t;
	int	size;
	int	nr;
	char	part[UTFmax];
	int	np;
};

aggr Window
{
	chan(Mesg)	in;
	chan(Mesg)	out;
	int		id;
	int		ref;
	int		top;
	Window		*next;
	Window		*list;
	Bitmap		*cache;
	Rectangle	win;
	Rectangle	clip;
	Rectangle	scroll;
	Rectangle	active;
	int		hold;
	int		scrollb;
	int		oscrollb;
	int		scrollon;
	int		didscroll;
	int		hidden;
	Mouse		mse;
	int		msedelta;
	int		mouseopen;
	int		reshape;
	int		notefd;
	int		titleb;
	Rectangle	titler;
	int		closed;
	int		bitopen;
	int		bitinit;
	int		dcursor;
	int		chgcurs;
	Cursor;
	Frame		*frame;
	int		origin;
	int		wrt;
	int		input;
	int		raw;
	int		ctlref;
	int		sp0;
	int		sp1;
	int		stick;
	Buf		*screen;
	Buf		*keybuf;
	Ioreq		*rdioqhd;
	Ioreq		*wrioqhd;
	char		label[NAMELEN+1];
	int		maxbit;
	Bitmap		**bits;
	int		rid;
	char		mid;
	char		*window;
	int		wsize;
};

aggr Fid
{
	int	fid;
	int	qid;
	Window	*w;
	Fid	*next;
};

aggr Dirtab
{
	char	*name;
	uint	qid;
	uint	perm;
};

aggr Sweep
{
	int 		doit;
	Point 		start;
	Rectangle	on;
	Rectangle	off;
	Window		*w;
};

Window	*whead;
Window	*wtail;
Window	*wcurr;
Window	*wlist;
Window	*grab;
Sweep	sweep;
int 	mtp[2];
int	mgrstate;
Subfont	*subfont;
Bitmap	*bgrnd;
Bitmap	*ncur;
char 	srv[128];
Buf	*cut;
Buf	*cut2;
int	wtrls;
Cursor	*mgrcurs;
Rlist	*rpend;

chan(Mesg) wmgr;
chan(Mesg) menuserv;
chan(Mesg) dispatch;

void	bitread(Window*, Ioreq*);
void	bitwrite(Window*, Ioreq*);
void	bleft(Mesg*);
void	cleanio(Ioreq**);
void	client(Window*);
void	clntactive(Window*);
void	clntclose(Window*);
void	clntclose(Window*);
void	clntclunk(Window*, int);
void	clntcons(Window*, Ioreq*);
void	clntfill(Window*);
int	clntflush(Window*, Ioreq*);
void	clntkey(Window*, Mesg*);
void	clntmgr(Window*, Mesg*);
void	clntmouse(Window*, Mesg*);
void	clntnote(Window*, char*);
void	clnto(Window*, int);
void	clntrawoff(Window*);
void	clntref(Window*, Mesg*);
void	clntreshape(Window*);
void	clntscroll(Window*, Mesg*);
void	clntsel(Window *w, int, int);
void	clntshow(Window*, int);
void	clntstart(Window*);
int	consread(Window*, Ioreq*);
int	conswrite(Window*, Ioreq*);
void	cutbuf(Rune*, int);
void	cuset(Mesg*);
int	cvtbufcr(Buf*, Rune**, char*, int);
void	delete(Window*);
void	dispatcher(void);
void	doio(Window*);
void	dohides(Mesg*);
int	domenu(Point, char**);
void	error(char*, ...);
int	event(Window*, int);
void	execute(int);
void	files(void);
Fid*	getfid(int);
void	hold(Window*);
void	init(Window*);
Rlist*	intersect(Rectangle, Rectangle);
Rlist*	intersect(Rectangle, Rectangle);
int	iseot(Rune*, int);
void	keybd(chan(Mesg));
void	labio(Window*, Ioreq*, int);
void	menuserver(chan(Mesg));
void	mgrmenu(chan(Mesg), Point);
void	mkclient(Window*);
void	mouse(chan(Mesg));
void	mouseread(Window*, Ioreq*);
Buf*	newbuf(int);
void	popw(Window*);
void	pushw(Window*);
void	qio(Window*, Ioreq*);
void	wraise(Mesg*);
void	refresh(Rectangle);
void	reply(Fcall*, Fcall*, char*);
int	scan(Window*, int, int, Rune);
void	scroll(Window*);
void	scrollb(Window*, int, int, int);
void	select(Window*);
void	setcur(Window*);
void	setsub(void);
void	sweeper(int);
void	textsel(Window *w, int);
int	titlebar(Window*);
void	todel(Window*);
void	tofront(Window*);
Rlist*	update(Rlist*, Bitmap*, int);
void	wdelete(Window*, int, int);
void	windref(Window*);
void	winmgr(void);
void	winread(Window*, Ioreq*);
void	winsert(Window*w, int, Rune*, int);
Window*	wsearch(void);
void	wwrite(Window*w, char*, int);

void*	ALEF_tid();
