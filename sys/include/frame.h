#pragma	src	"/sys/src/libframe"
#pragma	lib	"libframe.a"

typedef struct Frbox Frbox;
typedef struct Frame Frame;

struct Frbox
{
	long		wid;		/* in pixels */
	long		nrune;		/* <0 ==> negate and treat as break char */
	union{
		uchar	*ptr;
		struct{
			short	bc;	/* break char */
			short	minwid;
		};
	};
};

struct Frame
{
	Font		*font;		/* of chars in the frame */
	Bitmap		*b;		/* on which frame appears */
	Rectangle	r;		/* in which text appears */
	Rectangle	entire;		/* of full frame */
	Frbox		*box;
	ulong		p0, p1;		/* selection */
	short		left;		/* left edge of text */
	ushort		nbox, nalloc;
	ushort		maxtab;		/* max size of tab, in pixels */
	ushort		nchars;		/* # runes in frame */
	ushort		nlines;		/* # lines with text */
	ushort		maxlines;	/* total # lines in frame */
	ushort		lastlinefull;	/* last line fills frame */
	ushort		modified;	/* changed since frselect() */
};

ulong	frcharofpt(Frame*, Point);
Point	frptofchar(Frame*, ulong);
int	frdelete(Frame*, ulong, ulong);
void	frinsert(Frame*, Rune*, Rune*, ulong);
void	frselect(Frame*, Mouse*);
void	frselectp(Frame*, Fcode);
void	frselectf(Frame*, Point, Point, Fcode);
void	frinit(Frame*, Rectangle, Font*, Bitmap*);
void	frsetrects(Frame*, Rectangle, Bitmap*);
void	frclear(Frame*);
void	frgetmouse(void);

uchar	*_frallocstr(unsigned);
void	_frinsure(Frame*, int, unsigned);
Point	_frdraw(Frame*, Point);
void	_frgrowbox(Frame*, int);
void	_frfreebox(Frame*, int, int);
void	_frmergebox(Frame*, int);
void	_frdelbox(Frame*, int, int);
void	_frsplitbox(Frame*, int, int);
int	_frfindbox(Frame*, int, ulong, ulong);
void	_frclosebox(Frame*, int, int);
int	_frcanfit(Frame*, Point, Frbox*);
void	_frcklinewrap(Frame*, Point*, Frbox*);
void	_frcklinewrap0(Frame*, Point*, Frbox*);
void	_fradvance(Frame*, Point*, Frbox*);
int	_frnewwid(Frame*, Point, Frbox*);
void	_frclean(Frame*, Point, int, int);
void	_frredraw(Frame*, Point);
void	_fraddbox(Frame*, int, int);
Point	_frptofcharptb(Frame*, ulong, Point, int);
Point	_frptofcharnb(Frame*, ulong, int);
int	_frstrlen(Frame*, int);

#define	NRUNE(b)	((b)->nrune<0? 1 : (b)->nrune)
#define	NBYTE(b)	strlen((char*)(b)->ptr)
