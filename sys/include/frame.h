/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


typedef struct Frbox Frbox;
typedef struct Frame Frame;

enum{
	BACK,
	HIGH,
	BORD,
	TEXT,
	HTEXT,
	NCOL
};

#define	FRTICKW	3

struct Frbox
{
	i32		wid;		/* in pixels */
	i32		nrune;		/* <0 ==> negate and treat as break char */
	union{
		u8	*ptr;
		struct{
			i16	bc;	/* break char */
			i16	minwid;
		};
	};
};

struct Frame
{
	Font		*font;		/* of chars in the frame */
	Display		*display;	/* on which frame appears */
	Image		*b;		/* on which frame appears */
	Image		*cols[NCOL];	/* text and background colors */
	Rectangle	r;		/* in which text appears */
	Rectangle	entire;		/* of full frame */
	void			(*scroll)(Frame*, int);	/* scroll function provided by application */
	Frbox		*box;
	u32		p0, p1;		/* selection */
	u16		nbox, nalloc;
	u16		maxtab;		/* max size of tab, in pixels */
	u16		nchars;		/* # runes in frame */
	u16		nlines;		/* # lines with text */
	u16		maxlines;	/* total # lines in frame */
	u16		lastlinefull;	/* last line fills frame */
	u16		modified;	/* changed since frselect() */
	Image		*tick;	/* typing tick */
	Image		*tickback;	/* saved image under tick */
	int			ticked;	/* flag: is tick onscreen? */
};

u32	frcharofpt(Frame*, Point);
Point	frptofchar(Frame*, u32);
int	frdelete(Frame*, u32, u32);
void	frinsert(Frame*, Rune*, Rune*, u32);
void	frselect(Frame*, Mousectl*);
void	frselectpaint(Frame*, Point, Point, Image*);
void	frdrawsel(Frame*, Point, u32, u32, int);
Point frdrawsel0(Frame*, Point, u32, u32, Image*, Image*);
void	frinit(Frame*, Rectangle, Font*, Image*, Image**);
void	frsetrects(Frame*, Rectangle, Image*);
void	frclear(Frame*, int);

u8	*_frallocstr(Frame*, unsigned);
void	_frinsure(Frame*, int, unsigned);
Point	_frdraw(Frame*, Point);
void	_frgrowbox(Frame*, int);
void	_frfreebox(Frame*, int, int);
void	_frmergebox(Frame*, int);
void	_frdelbox(Frame*, int, int);
void	_frsplitbox(Frame*, int, int);
int	_frfindbox(Frame*, int, u32, u32);
void	_frclosebox(Frame*, int, int);
int	_frcanfit(Frame*, Point, Frbox*);
void	_frcklinewrap(Frame*, Point*, Frbox*);
void	_frcklinewrap0(Frame*, Point*, Frbox*);
void	_fradvance(Frame*, Point*, Frbox*);
int	_frnewwid(Frame*, Point, Frbox*);
int	_frnewwid0(Frame*, Point, Frbox*);
void	_frclean(Frame*, Point, int, int);
void	_frdrawtext(Frame*, Point, Image*, Image*);
void	_fraddbox(Frame*, int, int);
Point	_frptofcharptb(Frame*, u32, Point, int);
Point	_frptofcharnb(Frame*, u32, int);
int	_frstrlen(Frame*, int);
void	frtick(Frame*, Point, int);
void	frinittick(Frame*);
void	frredraw(Frame*);

#define	NRUNE(b)	((b)->nrune<0? 1 : (b)->nrune)
#define	NBYTE(b)	strlen((char*)(b)->ptr)
