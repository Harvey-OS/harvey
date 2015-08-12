/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma src "/sys/src/libframe"
#pragma lib "libframe.a"

typedef struct Frbox Frbox;
typedef struct Frame Frame;

enum {
	BACK,
	HIGH,
	BORD,
	TEXT,
	HTEXT,
	NCOL
};

#define FRTICKW 3

struct Frbox {
	int32_t wid;   /* in pixels */
	int32_t nrune; /* <0 ==> negate and treat as break char */
	union {
		uint8_t *ptr;
		struct {
			int16_t bc; /* break char */
			int16_t minwid;
		};
	};
};

struct Frame {
	Font *font;		      /* of chars in the frame */
	Display *display;	     /* on which frame appears */
	Image *b;		      /* on which frame appears */
	Image *cols[NCOL];	    /* text and background colors */
	Rectangle r;		      /* in which text appears */
	Rectangle entire;	     /* of full frame */
	void (*scroll)(Frame *, int); /* scroll function provided by application */
	Frbox *box;
	uint32_t p0, p1; /* selection */
	uint16_t nbox, nalloc;
	uint16_t maxtab;       /* max size of tab, in pixels */
	uint16_t nchars;       /* # runes in frame */
	uint16_t nlines;       /* # lines with text */
	uint16_t maxlines;     /* total # lines in frame */
	uint16_t lastlinefull; /* last line fills frame */
	uint16_t modified;     /* changed since frselect() */
	Image *tick;	   /* typing tick */
	Image *tickback;       /* saved image under tick */
	int ticked;	    /* flag: is tick onscreen? */
};

uint32_t frcharofpt(Frame *, Point);
Point frptofchar(Frame *, uint32_t);
int frdelete(Frame *, uint32_t, uint32_t);
void frinsert(Frame *, Rune *, Rune *, uint32_t);
void frselect(Frame *, Mousectl *);
void frselectpaint(Frame *, Point, Point, Image *);
void frdrawsel(Frame *, Point, uint32_t, uint32_t, int);
Point frdrawsel0(Frame *, Point, uint32_t, uint32_t, Image *, Image *);
void frinit(Frame *, Rectangle, Font *, Image *, Image **);
void frsetrects(Frame *, Rectangle, Image *);
void frclear(Frame *, int);

uint8_t *_frallocstr(Frame *, unsigned);
void _frinsure(Frame *, int, unsigned);
Point _frdraw(Frame *, Point);
void _frgrowbox(Frame *, int);
void _frfreebox(Frame *, int, int);
void _frmergebox(Frame *, int);
void _frdelbox(Frame *, int, int);
void _frsplitbox(Frame *, int, int);
int _frfindbox(Frame *, int, uint32_t, uint32_t);
void _frclosebox(Frame *, int, int);
int _frcanfit(Frame *, Point, Frbox *);
void _frcklinewrap(Frame *, Point *, Frbox *);
void _frcklinewrap0(Frame *, Point *, Frbox *);
void _fradvance(Frame *, Point *, Frbox *);
int _frnewwid(Frame *, Point, Frbox *);
int _frnewwid0(Frame *, Point, Frbox *);
void _frclean(Frame *, Point, int, int);
void _frdrawtext(Frame *, Point, Image *, Image *);
void _fraddbox(Frame *, int, int);
Point _frptofcharptb(Frame *, uint32_t, Point, int);
Point _frptofcharnb(Frame *, uint32_t, int);
int _frstrlen(Frame *, int);
void frtick(Frame *, Point, int);
void frinittick(Frame *);
void frredraw(Frame *);

#define NRUNE(b) ((b)->nrune < 0 ? 1 : (b)->nrune)
#define NBYTE(b) strlen((char *)(b)->ptr)
