/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef enum Vis{
	None=0,
	Some,
	All,
}Vis;

enum{
	Clicktime=1000,		/* one second */
};

typedef struct Flayer Flayer;

struct Flayer
{
	Frame		f;
	long		origin;	/* offset of first char in flayer */
	long		p0, p1;
	long		click;	/* time at which selection click occurred, in HZ */
	Rune		*(*textfn)(Flayer*, long, ulong*);
	int		user0;
	void		*user1;
	Rectangle	entire;
	Rectangle	scroll;
	Rectangle	lastsr;	/* geometry of scrollbar when last drawn */
	Vis		visible;
};

void	flborder(Flayer*, int);
void	flclose(Flayer*);
void	fldelete(Flayer*, long, long);
void	flfp0p1(Flayer*, ulong*, ulong*);
void	flinit(Flayer*, Rectangle, Font*, Image**);
void	flinsert(Flayer*, Rune*, Rune*, long);
void	flnew(Flayer*, Rune *(*fn)(Flayer*, long, ulong*), int, void*);
int	flprepare(Flayer*);
Rectangle flrect(Flayer*, Rectangle);
void	flrefresh(Flayer*, Rectangle, int);
void	flresize(Rectangle);
int	flselect(Flayer*);
void	flsetselect(Flayer*, long, long);
void	flstart(Rectangle);
void	flupfront(Flayer*);
Flayer	*flwhich(Point);

#define	FLMARGIN	4
#define	FLSCROLLWID	12
#define	FLGAP		4

extern	Image	*maincols[NCOL];
extern	Image	*cmdcols[NCOL];
