typedef struct Proc Proc;
typedef struct Text Text;
typedef struct Window Window;
typedef struct IOQ IOQ;

#define	NSTACK	600
#define	NSLOT	64

struct Proc
{
	jmp_buf	label;
	void	*arg;			/* pointer to generic argument */
	Proc	*nextrun;		/* next in runqueue order */
	Proc	*next;			/* next allocated */
	Mouse	mouse;
	int	dead;			/* flag */
	ulong	stack[NSTACK];
};

struct IOQ{
	IOQ	*next;
	int	tag;
	int	cnt;
	int	fid;
};

struct Text{
	Rune	*s;		/* saved text */
	ulong	n;		/* size of saved text */
	ulong	nalloc;		/* allocated size of text */
};

struct Window{
	int	ref;		/* # open files in this window */
	int	busy;
	int	slot;
	Rectangle r;
	Rectangle clipr;
	Rectangle scrollr;
	Layer	*l;
	Proc	*p;
	Cursor	*cursorp;
	Cursor	cursor;
	Frame	f;
	Rectangle lastbar;	/* last scrollbar position */
	long	org;		/* origin of frame */
	long	q0, q1;		/* position in absolute coords */
	long	qh;		/* host point */
	Text	text;		/* contents of window */
	Text	rawbuf;		/* unread raw characters */
	char	*wbuf;		/* data written to /dev/cons */
	int	wcnt;		/* #bytes in buf (not incl null) */
	int	woff;		/* offset into wbuf for termwrune */
	char	wpart[4];	/* incomplete rune pending from last write, with NUL */
	int	nwpart;		/* number of bytes in wpart */
	int	wtag;		/* tag of write mesg */
	int	wfid;		/* fid doing the write */
	int	rtag;		/* tag of read msg */
	int	rfid;		/* fid to read from */
	int	rcnt;		/* #bytes in pending read */
	char	rpart[4];	/* incomplete rune pending from last read, with NUL */
	int	nrpart;		/* number of bytes in rpart */
	int	mtag;		/* tag of mouse read msg */
	int	mfid;		/* fid to read mouse from */
int MFID;
	IOQ	*rq;		/* queue of pending read requests */
	IOQ	*wq;		/* queue of pending write requests */
	int	kbdc;		/* character typed on keyboard */
	int	send;		/* send the contents of the snarf buffer */
	char	wqid[8];	/* unique window serial number */
	Rectangle reshape;	/* reshape to this size */
	Rectangle onscreen;
	char	scroll;		/* whether to scroll automatically */
	char	hold;		/* whether reads are held */
	char	raw;		/* whether rcons is open */
	char	ctlopen;	/* count of opens of consctl */
	char	bitopen;	/* /dev/bitblt is open */
	char	bitinit;	/* /dev/bitblt has pending init message */
	char	mouseopen;	/* /dev/mouse is open */
	char	mousechanged;	/* mouse moved in this window */
	char	reshaped;	/* this window reshaped; tell client */
	char	dodelete;	/* this window was hit by Delete button */
	char	deleted;	/* this window has been deleted */
	long	bitid;		/* allocated bitmap # */
	long	fontid;		/* allocated font # */
	long	subfontid;	/* allocated subfont # */
	long	cachesfid;	/* cache lookup subfont # */
	int	hidemenu;	/* 1 if hiding, -1 if returning */
	uchar	*rdbmbuf;	/* buffer for rdbitmap */
	uchar	*rdwindow;	/* buffer for /dev/window */
	long	rdbml;		/* size of buffer for rdbitmap */
	long	rdwl;		/* size of buffer for /dev/window */
	int	nsubf;		/* number of subfonts */
	short	*subf;		/* array of subfont ref counts */
	int	pgrpfd;		/* file descriptor for notes */
	char	label[NAMELEN];	/* name when hidden */
};

struct{
	Proc	*p;		/* running process */
	Proc	*head;		/* next to run */
	Proc	*tail;		/* tail of run queue */
}proc;

struct{
	Mouse;
	uchar	data[14];
}mouse;

enum
{
	Enoerr,
	Eauth,
	Enotdir,
	Eisdir,
	Enonexist,
	Eperm,
	Einuse,
	Eio,
	Eslot,
	Erbit,
	Erwin,
	Esmall,
	Eshutdown,
	Ebit,
	Esbit,
	Ebitmap,
	Esubfont,
	Efont,
	Eunimpl,
	Estring,
	Ebadrect,
	Enores,
	Ebadctl,
	Epartial,
	Eoffend,
	Esfnotcached,
	Eshort,
	Eballoc,
	Egreg		/* must be last */
};


#define	SCROLLWID	12
#define	SCROLLGAP	4
#define	NKBDC		64

extern	uchar	kbdc[];
extern	Window	window[];
extern  char	*menu3str[];

extern	Proc	*pmouse;
extern	Proc	*pkbd;
extern	Window	*input;
extern	int	kbdcnt;
extern	int	sfd;
extern	int	defscroll;
extern	Cover	cover;
extern	Bitmap	*darkgrey;
extern	int	clockfd;
extern	Cursor	whitearrow;
extern	char	*bitbuf;
extern	Bitmap	*lightgrey;
extern	Bitmap	*darkgrey;
extern	int	cfd;
extern	int	boxup;
extern	Rectangle box;
extern	ulong	wqid;
extern	Subfont	*subfont;

#define	SNARF	0
#define	BOXWID	2
