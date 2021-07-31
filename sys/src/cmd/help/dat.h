typedef struct Addr	Addr;
typedef struct Client	Client;
typedef struct Page	Page;
typedef struct Proc	Proc;
typedef struct String	String;
typedef struct Text	Text;

#define	NSTACK	1024

struct Proc
{
	jmp_buf	label;
	Proc	*nextrun;		/* next in runqueue order */
	Proc	*next;			/* next allocated */
	Mouse	mouse;
	int	dead;			/* flag */
	void	*arg;
	ulong	stack[NSTACK];
};

struct String
{
	ushort	*s;		/* saved text */
	ulong	n;		/* size of saved text */
	ulong	nmax;		/* allocated size of text */
};

struct Text
{
	Frame;
	String;
	long	org;		/* posn of first visible char */
	long	q0;		/* selection */
	long	q1;
	int	outline;	/* selection should be outlined */
	Page	*page;		/* containing page */
	Rectangle scrollr;	/* locn of scroll bar */
};

struct Page
{
	Page	*next;		/* in current list */
	Page	*down;		/* head of list at next level down */
	int	id;		/* unique identifier */
	int	n;		/* tab number */
	int	ntab;		/* next tab for sublist */
	int	mod;		/* has been modified */
	Rectangle r;
	Rectangle subr;		/* rectangle containing subpages */
	Rectangle tab;
	Rectangle tabsr;	/* location of tabs for subpages */
	Page	*parent;	/* p is in list at parent->down */
	int	visible;	/* not hidden by other pages in list */
	Text	tag;
	Text	body;
	Text	*text;		/* which Text mouse is inside */
	int	ver;		/* parent of vertical list */
	int	bdry;		/* y coord of division */
	int	line1;		/* y coord of first line of text in body */
};

struct Client
{
	int	ref;
	int	slot;
	int	busy;
	Proc	*p;
	char	*argv0;		/* name of process */
	int	close;		/* shut down now */
	uchar	*wbuf;		/* written data */
	int	wcnt;
	int	wfid;
	String	*index;
};

struct Addr
{
	int	valid;
	long	q0;
	long	q1;
};

enum
{
	Ebadaddr,
	Ebadctl,
	Eclosed,
	Eio,
	Eisdir,
	Enonexist,
	Enopage,
	Enotdir,
	Eperm,
	Eshutdown,
	Eslot,
	Egreg
};

extern	Bitmap	*lightgrey;
extern	Bitmap	*darkgrey;
extern	Cursor	box;

extern	struct proc{
	Proc	*p;		/* running process */
	Proc	*head;		/* next to run */
	Proc	*tail;		/* tail of run queue */
}proc;

extern	int	cfd;		/* client end of pipe */
extern	int	sfd;		/* service end of pipe */
extern	int	clockfd;	/* /dev/time */
extern	int	timefd;		/* /dev/cputime */

#define	NSLOT	32

#define	TABSZ		16
#define	SCROLLWID	12
#define	SCROLLGAP	(TABSZ-SCROLLWID)

extern	Client	client[NSLOT];
extern	Proc	*pmouse;
extern	Proc	*pkbd;
extern	int	kbdc;
extern	struct mouse{
	Mouse;
	uchar	data[14];
}mouse;

extern	Page	*rootpage;
extern	Page	*page;
extern	Text	*curt;		/* current text with selection */
extern	Text	*prevt;		/* previous text with selection */
extern	char	user[NAMELEN];
#define	GENBUFSIZE	4096
extern	Rune	genbuf[GENBUFSIZE];
extern	int	pageid;
extern	long	clicktime;	/* ms since last click */
