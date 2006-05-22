#define	Font	XXFont
#define	Screen	XXScreen
#define	Display	XXDisplay

#include <X11/Xlib.h>
/* #include <X11/Xlibint.h> */
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#undef	Font
#undef	Screen
#undef	Display

/*
 * Structure pointed to by X field of Memimage
 */
typedef struct Xmem Xmem;

enum
{
	PMundef	= ~0		/* undefined pixmap id */
};


struct Xmem
{
	int	pmid;	/* pixmap id for screen ldepth instance */
	XImage *xi;	/* local image if we currenty have the data */
	int	dirty;
	Rectangle dirtyr;
	Rectangle r;
	uintptr pc;	/* who wrote into xi */
};

extern	int		xtblbit;
extern	int		x24bitswap;
extern	int		plan9tox11[];
extern  int		x11toplan9[];
extern	int		xscreendepth;
extern	XXDisplay	*xdisplay;
extern	Drawable	xscreenid;
extern	Visual		*xvis;
extern	GC		xgcfill, xgcfill0;
extern	int		xgcfillcolor, xgcfillcolor0;
extern	GC		xgccopy, xgccopy0;
extern	GC		xgczero, xgczero0;
extern	int		xgczeropm, xgczeropm0;
extern	GC		xgcsimplesrc, xgcsimplesrc0;
extern	int		xgcsimplecolor, xgcsimplecolor0, xgcsimplepm, xgcsimplepm0;
extern	GC		xgcreplsrc, xgcreplsrc0;
extern	int		xgcreplsrcpm, xgcreplsrcpm0, xgcreplsrctile, xgcreplsrctile0;
extern	XImage*		allocXdata(Memimage*, Rectangle);
extern	void 		putXdata(Memimage*, Rectangle);
extern	XImage*		getXdata(Memimage*, Rectangle);
extern	void		freeXdata(Memimage*);
extern	void	dirtyXdata(Memimage*, Rectangle);
extern	ulong	xscreenchan;
extern	void	xfillcolor(Memimage*, Rectangle, ulong);
