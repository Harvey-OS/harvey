/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define	NPAGES	500
#define NFONT	33
#define NSIZE	40
#define MINSIZE 4
#define	DEFMAG	(10.0/11.0)	/* was (10.0/11.0), then 1 */
#define MAXVIEW 40

#define	ONES	~0

extern	char	devname[];
extern	double	mag;
extern	int	nview;
extern	int	hpos, vpos, curfont, cursize;
extern	int	DIV, res;
extern	int	Mode;

extern	Point	offset;		/* for small pages within big page */
extern	Point	xyoffset;	/* for explicit x,y move */
extern	Cursor	deadmouse;

extern	char	libfont[];

void	mapscreen(void);
void	clearscreen(void);
char	*getcmdstr(void);

void	readmapfile(char *);
void	dochar(Rune*);
void	bufput(void);
void	loadfontname(int, char *);
void	allfree(void);
void	readpage(void);
int	isspace(int);
extern	int	getc(void);
extern	int	getrune(void);
extern	void	ungetc(void);
extern	uint32_t	offsetc(void);
extern	uint32_t	seekc(uint32_t);
extern	char*	rdlinec(void);


#define	dprint	if (dbg) fprint

extern	int	dbg;
extern	int	resized;
