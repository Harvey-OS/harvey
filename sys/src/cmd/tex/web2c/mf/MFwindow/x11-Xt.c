/*
 * X11 window interface for Metafont, using Xt.
 * (rusty@garnet.berkeley.edu)
 */

#define	EXTERN	extern
#include "../mfd.h"

#ifdef	X11WIN
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

# define PLANE	0

static int		mf_defwidth =	0;
static int		mf_defheight =	0;

static Display		*mf_display;
static Window		mf_window;

static Pixmap		mf_pixmap;

static XtAppContext	mf_app;

static GC		mf_dgc;		/* draw gc */
static GC		mf_egc;		/* erase gc */
static GC		mf_cgc;		/* copy plane gc */

static Pixel		mf_fg, mf_bg;
static unsigned int	mf_width, mf_height;

static Boolean		mf_mapped;

static int		mf_max_x, mf_max_y;
static int		mf_min_x, mf_min_y;

static XtResource mf_resources[] = {
	{ "width", "Width", XtRInt, sizeof(int),
		  (Cardinal) &mf_width, XtRInt, (caddr_t) &mf_defwidth },
	{ "height", "Height", XtRInt, sizeof(int),
		  (Cardinal) &mf_height, XtRInt, (caddr_t) &mf_defheight },
	{ "foreground", "Foreground", XtRPixel, sizeof(Pixel),
		  (Cardinal) &mf_fg, XtRString, (caddr_t) "Black" },
	{ "background", "Background", XtRPixel, sizeof(Pixel),
		  (Cardinal) &mf_bg, XtRString, (caddr_t) "White" },
};

static XrmOptionDescRec mf_optiondesclist[] = {
{"-width",	"width",	XrmoptionSepArg, 	(caddr_t) NULL},
{"-height",	"height",	XrmoptionSepArg,	(caddr_t) NULL},
{"-fg",		"foreground",	XrmoptionSepArg,	(caddr_t) NULL},
{"-bg",		"background",	XrmoptionSepArg,	(caddr_t) NULL},
};

static void mf_events();
static void mf_newpixmap ();
static void mf_repaint();
static void mf_mapstatus();
static void mf_redraw();


/* return 1 if display opened successfully, else 0 */
int
mf_x11_initscreen() {
	XSetWindowAttributes	xwa;
	Widget			mf_toplevel;
	Widget			mf_canvas;
	XGCValues		gcv;
	Arg			args[1];
	int			mf_argc;
	char			*mf_argv[2];

	mf_argv[0] = "mf";
	mf_argv[1] = NULL;
	mf_argc = 1;

	mf_toplevel = XtInitialize("mf", "Metafont",
				   mf_optiondesclist,
				   XtNumber(mf_optiondesclist),
				   &mf_argc, mf_argv);

	XtGetApplicationResources(mf_toplevel, 0,
				  mf_resources, XtNumber(mf_resources),
				  NULL, 0 );


	if (mf_argc != 1) {
		(void) fprintf(stderr, "Usage: %s\n", mf_argv[0]);
		exit(1);
	}

	/*
	 * if nothing specified in their resources/.Xdefaults
	 * then use the values of metafont's "screen".
	 */
	if (mf_width == 0)
		mf_width = screenwidth;
	if (mf_height == 0)
		mf_height = screendepth;

	mf_canvas = XtCreateManagedWidget("canvas", widgetClass, mf_toplevel,
					  NULL, 0);

	XtSetArg(args[0], XtNwidth, mf_width);
	XtSetValues(mf_canvas, args, 1);
	XtSetArg(args[0], XtNheight, mf_height);
	XtSetValues(mf_canvas, args, 1);

	/* for mf_x11_updatescreen() */
	mf_app = XtWidgetToApplicationContext(mf_canvas);

	XtAddEventHandler(mf_canvas, (Cardinal) ExposureMask, True, 
			  mf_repaint, NULL);
	XtAddEventHandler(mf_canvas, (Cardinal) StructureNotifyMask, True, 
			  mf_mapstatus, NULL);

	XtRealizeWidget(mf_toplevel);

	mf_display = XtDisplay(mf_canvas);
	mf_window = XtWindow(mf_canvas);

	/*
	 * since metafont isn't your typical x window program that
	 * sits in xt_main_loop, if the server supports backing store
	 * and save unders this will help keep the output looking
	 * nice.
	 */
	xwa.backing_store = Always;
	xwa.save_under = True;
	XChangeWindowAttributes(mf_display, mf_window,
				CWBackingStore|CWSaveUnder, &xwa);

	gcv.background = mf_bg;
	gcv.foreground = mf_fg;
	gcv.function = GXcopy;

	/* copy plane gc */
	mf_cgc = XCreateGC(mf_display, mf_window,
			   GCForeground|GCBackground|GCFunction, &gcv);

	mf_newpixmap(screenwidth > mf_width ? screenwidth : mf_width,
		     screendepth > mf_height ? screendepth : mf_height);

	return(1);
}

/* make sure the screen is up to date */
void
mf_x11_updatescreen() {
	XEvent		event;

	mf_events();
	mf_redraw();

#ifdef notdef
	printf("max_x=%d, min_x=%d, max_y=%d, min_y=%d\n",
	       mf_max_x, mf_min_x,
	       mf_max_y, mf_min_y);
#endif
}

void
mf_x11_blankrectangle(left, right, top, bottom)
	screencol	left, right;
	screenrow	top, bottom;
{
	extern void	mf_events();

	XFillRectangle(mf_display, mf_pixmap, mf_egc, (int) left, (int) top,
		       (int) (right - left + 1), (int) (bottom - top + 1));

	mf_events();
}

void
mf_x11_paintrow(row, init_color, tvect, vector_size)
	screenrow		row;
	pixelcolor		init_color;
	transspec		tvect;
	register screencol	vector_size;
{
	extern void		mf_checkextent();
	Pixel			color;
	GC			gc;
	int			col;

	gc = (init_color == 0) ? mf_egc : mf_dgc;

	do {
		col = *tvect++;

# ifdef MF_XT_DEBUG
		mf_checkextent(col, (*tvect -1), row);
# endif /* MF_XT_DEBUG */

		XDrawLine(mf_display, mf_pixmap, gc, col, (int) row,
			  (int) (*tvect - 1), (int) row);

		gc = (gc == mf_egc) ? mf_dgc : mf_egc;
	} while (--vector_size > 0);

	mf_events();
}

# ifdef MF_XT_DEBUG
static void
mf_checkextent(x1, x2, y) {
	if (x1 < mf_min_x)
		mf_min_x = x1;
	if (x1 > mf_max_x)
		mf_max_x = x1;

	if (x2 < mf_min_x)
		mf_min_x = x2;
	if (x2 > mf_max_x)
		mf_max_x = x2;

	if (y > mf_max_y)
		mf_max_y = y;
	if (y < mf_min_y)
		mf_min_y = y;
}
# endif /* MF_XT_DEBUG */

static void
mf_events() {
	XEvent	event;

	if (XtAppPending(mf_app) != 0) {
		while (XtAppPending(mf_app) != 0) {
			XtAppNextEvent(mf_app, &event);
			XtDispatchEvent(&event);
		}
		/* mf_redraw(); */
	}
}

static void
mf_newpixmap(width, height)
	unsigned int		width, height;
{
	XGCValues		gcv;
	Pixmap			newpixmap;

	/*
	 * width == mf_width and height == mf_height
	 * the first time mf_newpixmap() is called.
	 */
	if ((width < mf_width) && (height < mf_height))
		return;

	newpixmap = XCreatePixmap(mf_display, mf_window,
				  width, height, 1);

	gcv.background = 0;
	gcv.foreground = 1;

	if (mf_dgc != 0)
		XFreeGC(mf_display, mf_dgc);

	/* draw gc */
	gcv.line_width = 1;
	mf_dgc = XCreateGC(mf_display, newpixmap,
			   GCForeground|GCBackground|GCLineWidth, &gcv);

	if (mf_egc != 0)
		XFreeGC(mf_display, mf_egc);

	/* erase gc */
	gcv.foreground = 0;
	mf_egc = XCreateGC(mf_display, newpixmap,
			   GCForeground|GCBackground|GCLineWidth, &gcv);

	XFillRectangle(mf_display, newpixmap, mf_egc, 0, 0, width, height);

	if (mf_pixmap != 0) {
		XCopyArea(mf_display, mf_pixmap, newpixmap, mf_dgc, 0, 0,
			  mf_width, mf_height, 0, 0);

		XFreePixmap(mf_display, mf_pixmap);
	}

	mf_pixmap = newpixmap;

	mf_width = width;
	mf_height = height;
}

/*ARGSUSED*/
static void 
mf_repaint(w, data, ev)
	Widget		w;
	caddr_t		data;
	XEvent		*ev;
{
	extern void	mf_redraw();

	if (! mf_mapped)
		return;

	if (ev == NULL)
		return;

	if (ev->type != Expose)
		return;

	/* we are a "simple application" */
	if (ev->xexpose.count == 0) {
		XEvent	event;

		/* skip all excess redraws */
		while (XCheckTypedEvent(mf_display, Expose, &event) != False)
			continue;

		mf_redraw();
	}
}

/*ARGSUSED*/
static void 
mf_mapstatus(w, data, ev)
	Widget		w;
	caddr_t		data;
	XEvent		*ev;
{
	extern void	mf_redraw();
	extern void	mf_newpixmap();

	if (ev->type == MapNotify) {
		mf_mapped = True;

		return;
	}

	if (ev->type == UnmapNotify) {
		mf_mapped = False;

		return;
	}

	if (ev->type == ConfigureNotify) {
		mf_newpixmap(ev->xconfigure.width, ev->xconfigure.height);
		mf_redraw();

		return;
	}
}

static void
mf_redraw() {
	XCopyPlane(mf_display, mf_pixmap, mf_window, mf_cgc, 0, 0,
		   mf_width, mf_height,
		   0, 0, (unsigned long) 1);

	XFlush(mf_display);
}

#else
int x11_dummy;
#endif /* X11WIN */
