#pragma lib "/$M/lib/alef/libframe.a"

aggr Frbox
{
	int		wid;		/* in pixels */
	int		nrune;		/* <0 ==> negate and treat as break char */
	union{
		char	*ptr;
		aggr {
			sint	bc;	/* break char */
			sint	minwid;
		};
	};
};

aggr Frame
{
	Font		*font;		/* of chars in the frame */
	Bitmap		*b;		/* on which frame appears */
	Rectangle	r;		/* in which text appears */
	Rectangle	entire;		/* of full frame */
	Frbox		*box;
	uint		p0;
	uint		p1;		/* selection */
	sint		left;		/* left edge of text */
	usint		nbox, nalloc;
	usint		maxtab;		/* max size of tab, in pixels */
	usint		nchars;		/* # runes in frame */
	usint		nlines;		/* # lines with text */
	usint		maxlines;	/* total # lines in frame */
	usint		lastlinefull;	/* last line fills frame */
	usint		modified;	/* changed since frselect() */
};

uint	frcharofpt(Frame*, Point);
Point	frptofchar(Frame*, uint);
int	frdelete(Frame*, uint, uint);
void	frinsert(Frame*, Rune*, Rune*, uint);
void	frselect(Frame*, Mouse*);
void	frselectp(Frame*, Fcode);
void	frselectf(Frame*, Point, Point, Fcode);
void	frinit(Frame*, Rectangle, Font*, Bitmap*);
void	frsetrects(Frame*, Rectangle, Bitmap*);
void	frclear(Frame*);
void	frgetmouse(void);

char	*_frallocstr(uint);
void	_frinsure(Frame*, int, uint);
Point	_frdraw(Frame*, Point);
void	_frgrowbox(Frame*, int);
void	_frfreebox(Frame*, int, int);
void	_frmergebox(Frame*, int);
void	_frdelbox(Frame*, int, int);
void	_frsplitbox(Frame*, int, int);
int	_frfindbox(Frame*, int, uint, uint);
void	_frclosebox(Frame*, int, int);
int	_frcanfit(Frame*, Point, Frbox*);
void	_frcklinewrap(Frame*, Point*, Frbox*);
void	_frcklinewrap0(Frame*, Point*, Frbox*);
void	_fradvance(Frame*, Point*, Frbox*);
int	_frnewwid(Frame*, Point, Frbox*);
void	_frclean(Frame*, Point, int, int);
void	_frredraw(Frame*, Point);
void	_fraddbox(Frame*, int, int);
Point	_frptofcharptb(Frame*, uint, Point, int);
Point	_frptofcharnb(Frame*, uint, int);
int	_frstrlen(Frame*, int);

#define	NRUNE(b)	((b)->nrune<0? 1 : (b)->nrune)
#define	NBYTE(b)	strlen((char*)(b)->ptr)
