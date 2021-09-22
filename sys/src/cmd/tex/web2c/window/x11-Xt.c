/* x11.c: X11 window interface for Metafont, using Xt.  Original by
   rusty@garnet.berkeley.edu.  */

#define	EXTERN extern
#include "../mfd.h"

#ifdef X11WIN			/* almost whole file */

/* For wchar_t et al., that the X files might want. */
#include <kpathsea/systypes.h>

/* See xdvik/xdvi.h for the purpose of the FOIL...  */
#ifdef FOIL_X_WCHAR_T
#define wchar_t foil_x_defining_wchar_t
#define X_WCHAR
#endif
#undef input /* the XWMHints structure has a field named `input' */
#undef output
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#undef wchar_t

#define PLANE 0

static unsigned int mf_defwidth = 0;
static unsigned int mf_defheight = 0;

static Display *mf_display;
static Window mf_window;

static Pixmap mf_pixmap;

static XtAppContext mf_app;

static GC mf_dgc;		/* draw gc */
static GC mf_egc;		/* erase gc */
static GC mf_cgc;		/* copy plane gc */

typedef struct
{
  unsigned int mf_width, mf_height;
  Pixel mf_fg, mf_bg;
} mf_resources_struct;

static mf_resources_struct mf_x11_resources;


/* Don't paint anything until we're mapped.  */
static Boolean mf_mapped;

#ifdef MF_XT_DEBUG
static int mf_max_x, mf_max_y;
static int mf_min_x, mf_min_y;

static void mf_checkextent ();
#endif

static XtResource mf_resources[]
  = { { "width", "Width", XtRInt, sizeof (int),
        XtOffset (mf_resources_struct *, mf_width), XtRInt,
        (XtPointer) & mf_defwidth },

      { "height", "Height", XtRInt, sizeof (int),
        XtOffset (mf_resources_struct *, mf_height), XtRInt,
        (XtPointer) &mf_defheight },

      { "foreground", "Foreground", XtRPixel, sizeof (Pixel),
        XtOffset (mf_resources_struct *, mf_fg),
        XtRString, (XtPointer) "Black" },

      { "background", "Background", XtRPixel, sizeof (Pixel),
        XtOffset (mf_resources_struct *, mf_bg), XtRString,
        (XtPointer) "White" },
};

/* Maybe someday we'll read options, until then, this is just here for
   the resources.  */
static XrmOptionDescRec mf_optiondesclist[]
= { { "-width",   "width", XrmoptionSepArg, (XtPointer) NULL },
    { "-height", "height", XrmoptionSepArg, (XtPointer) NULL },
    { "-fg", "foreground", XrmoptionSepArg, (XtPointer) NULL },
    { "-bg", "background", XrmoptionSepArg, (XtPointer) NULL },
};

static void mf_events ();
static void mf_mapstatus ();
static void mf_newpixmap ();
static void mf_redraw ();
static void mf_repaint ();

/* Return 1 if display opened successfully, else 0.  */

int
mf_x11_initscreen ()
{
  XSetWindowAttributes xwa;
  Widget mf_toplevel;
  Widget mf_canvas;
  XGCValues gcv;
  Arg args[1];
  int mf_argc;
  char *mf_argv[2];

  mf_argv[0] = "mf";
  mf_argv[1] = NULL;
  mf_argc = 1;

  mf_toplevel = XtInitialize ("mf", "Metafont",
			      mf_optiondesclist,
			      XtNumber (mf_optiondesclist),
			      &mf_argc, mf_argv);

  XtGetApplicationResources (mf_toplevel, (XtPointer) & mf_x11_resources,
			     mf_resources, XtNumber (mf_resources),
			     NULL, 0);

  if (mf_argc != 1)
    {
      fprintf (stderr, "Usage: %s\n", mf_argv[0]);
      return 0;
    }

  /* If nothing specified in their resources (e.g., .Xdefaults)
     then use the values of Metafont's "screen".  */
  if (mf_x11_resources.mf_width == 0)
    mf_x11_resources.mf_width = screenwidth;
  if (mf_x11_resources.mf_height == 0)
    mf_x11_resources.mf_height = screendepth;

  mf_canvas = XtCreateManagedWidget ("canvas", widgetClass, mf_toplevel,
				     NULL, 0);

  XtSetArg (args[0], XtNwidth, mf_x11_resources.mf_width);
  XtSetValues (mf_canvas, args, 1);
  XtSetArg (args[0], XtNheight, mf_x11_resources.mf_height);
  XtSetValues (mf_canvas, args, 1);

  /* for mf_x11_updatescreen() */
  mf_app = XtWidgetToApplicationContext (mf_canvas);

  XtAddEventHandler (mf_canvas, (Cardinal) ExposureMask, True,
		     mf_repaint, NULL);
  XtAddEventHandler (mf_canvas, (Cardinal) StructureNotifyMask, True,
		     mf_mapstatus, NULL);

  XtRealizeWidget (mf_toplevel);

  mf_display = XtDisplay (mf_canvas);
  mf_window = XtWindow (mf_canvas);

  /* Since Metafont isn't your typical x window program that
     sits in XTMainLoop, if the server supports backing store
     and save unders this will help keep the output looking
     nice.  */
  xwa.backing_store = Always;
  xwa.save_under = True;
  XChangeWindowAttributes (mf_display, mf_window,
			   CWBackingStore | CWSaveUnder, &xwa);

  gcv.background = mf_x11_resources.mf_bg;
  gcv.foreground = mf_x11_resources.mf_fg;
  gcv.function = GXcopy;

  /* copy plane gc */
  mf_cgc = XCreateGC (mf_display, mf_window,
		      GCForeground | GCBackground | GCFunction, &gcv);

  mf_newpixmap (screenwidth > mf_x11_resources.mf_width
                ? screenwidth : mf_x11_resources.mf_width,
		screendepth > mf_x11_resources.mf_height
	        ? screendepth : mf_x11_resources.mf_height);

  return 1;
}

void
mf_x11_updatescreen ()
{
  mf_events ();
  mf_redraw ();

#ifdef MF_XT_DEBUG
  printf ("max_x=%d, min_x=%d, max_y=%d, min_y=%d\n",
	  mf_max_x, mf_min_x,
	  mf_max_y, mf_min_y);
#endif
}


void
mf_x11_blankrectangle P4C(screencol, left,
                          screencol, right,
                          screenrow, top,
                          screenrow, bottom)
{
  XFillRectangle (mf_display, mf_pixmap, mf_egc, (int) left, (int) top,
		  (int) (right - left + 1), (int) (bottom - top + 1));
  mf_events ();
}

void
mf_x11_paintrow P4C(screenrow, row,
                    pixelcolor, init_color,
                    transspec, tvect,
                    register screencol, vector_size)
{
  GC gc;
  int col;

  gc = (init_color == 0) ? mf_egc : mf_dgc;

  do
    {
      col = *tvect++;

#ifdef MF_XT_DEBUG
      mf_checkextent (col, *tvect, row);
#endif /* MF_XT_DEBUG */

      XDrawLine (mf_display, mf_pixmap, gc, col, (int) row,
		 (int) *tvect, (int) row);

      gc = (gc == mf_egc) ? mf_dgc : mf_egc;
    }
  while (--vector_size > 0);

  mf_events ();
}

#ifdef MF_XT_DEBUG
static void
mf_checkextent (x1, x2, y)
{
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
#endif /* MF_XT_DEBUG */

static void
mf_events ()
{
  XEvent event;

  if (XtAppPending (mf_app) != 0)
    {
      while (XtAppPending (mf_app) != 0)
	{
	  XtAppNextEvent (mf_app, &event);
	  XtDispatchEvent (&event);
	}
    }
}

static void
mf_newpixmap (width, height)
     unsigned int width, height;
{
  XGCValues gcv;
  Pixmap newpixmap;

  /* width == mf_width and height == mf_height
     the first time mf_newpixmap() is called.
   */
  if (width < mf_x11_resources.mf_width && height < mf_x11_resources.mf_height)
    return;

  newpixmap = XCreatePixmap (mf_display, mf_window, width, height, 1);

  gcv.background = 0;
  gcv.foreground = 1;

  if (mf_dgc != 0)
    XFreeGC (mf_display, mf_dgc);

  /* draw gc */
  gcv.line_width = 1;
  mf_dgc = XCreateGC (mf_display, newpixmap,
		      GCForeground | GCBackground | GCLineWidth, &gcv);

  if (mf_egc != 0)
    XFreeGC (mf_display, mf_egc);

  /* erase gc */
  gcv.foreground = 0;
  mf_egc = XCreateGC (mf_display, newpixmap,
		      GCForeground | GCBackground | GCLineWidth, &gcv);

  XFillRectangle (mf_display, newpixmap, mf_egc, 0, 0, width, height);

  if (mf_pixmap != 0)
    {
      XCopyArea (mf_display, mf_pixmap, newpixmap, mf_dgc, 0, 0,
		 mf_x11_resources.mf_width,
		 mf_x11_resources.mf_height, 0, 0);

      XFreePixmap (mf_display, mf_pixmap);
    }

  mf_pixmap = newpixmap;

  mf_x11_resources.mf_width = width;
  mf_x11_resources.mf_height = height;
}

static void
mf_repaint (w, data, ev)
     Widget w;
     XtPointer data;
     XEvent *ev;
{
  if (!mf_mapped || !ev || ev->type != Expose)
    return;

  /* We are a ``simple application''. */
  if (ev->xexpose.count == 0)
    {
      XEvent event;

      /* skip all excess redraws */
      while (XCheckTypedEvent (mf_display, Expose, &event) != False)
	continue;

      mf_redraw ();
    }
}


static void
mf_mapstatus (w, data, ev)
     Widget w;
     XtPointer data;
     XEvent *ev;
{
  switch (ev->type)
    {
    case MapNotify:
      mf_mapped = True;
      break;
    
    case UnmapNotify:
      mf_mapped = False;
      break;

    case ConfigureNotify:
      mf_newpixmap (ev->xconfigure.width, ev->xconfigure.height);
      mf_redraw ();
      break;
    }
}


static void
mf_redraw ()
{
  XCopyPlane (mf_display, mf_pixmap, mf_window, mf_cgc, 0, 0,
	      mf_x11_resources.mf_width, mf_x11_resources.mf_height,
	      0, 0, (unsigned long) 1);

  XFlush (mf_display);
}

#else
int x11_dummy;
#endif /* X11WIN */
