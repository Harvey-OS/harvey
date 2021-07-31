/*
X Window System version 11 (release 3 et al.) interface for Metafont.
 
Modified from Tim Morgan's X Version 11 routines by Richard Johnson.
Modified from that by Karl Berry <karl@umb.edu>.  8/3/89

MF can now understand geometry (sort of, at least on a Sun running 3.4
and using uwm) in the resource database, as in the following in
.Xdefaults to put a window of width 500 and height 600 at (200,50):

Metafont*geometry: 500x600+200+200

You cannot give the geometry on the command line (who would want to)?

The width and height specified in the resource must not be larger than
the screenwidth and screendepth defined in ../mf/cmf.ch.
If they are, then I reset them to the maximum.

We don't handle Expose events in general. This means that the window
cannot be moved, resized, or obscured. The problem is that I don't know
when we can look for such events. Adding a check to the main loop of
Metafont was more than I wanted to do. Another problem is that Metafont
does not keep track of the contents of the screen, and so I don't see
how to know what to redraw. The right way to do this is probably to fork
a process, and keep a Pixmap around of the contents of the window.

I could never have done this without David Rosenthal's Hello World
program for X. See $X/mit/doc/HelloWorld.

All section numbers refer to Xlib -- C Language X Interface.  */


#define	EXTERN	extern
#include "../mfd.h"


#ifdef	X11WIN

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>


/* Variables for communicating between the routines we'll write.  */

static Display *my_display;
static int my_screen;
static Window my_window;
static GC my_gc;
static int white, black;


/* Window manager hints.  */

static XWMHints wm_hints = {
   (InputHint|StateHint), /* flags telling which values are set */
   False, /* We don't expect input. */
   NormalState, /* Initial state. */
   0, /* icon pixmap */
   0, /* icon window */
   0, 0, /* icon location (should get from resource?) */
   0, /* icon mask */
   0 /* window group */
};


/* Some constants for the resource database, etc.  */
#define PROGRAM_NAME "Metafont"
#define ARG_GEOMETRY "geometry"
#define BORDER_WIDTH 1 /* Should get this from resource. */
#define DEFAULT_X_POSITION 0
#define DEFAULT_Y_POSITION 0


/* Return 1 (i.e., true) if display opened successfully, else 0.  */

int
mf_x11_initscreen()
{
    char *geometry;
    int geometry_found = 0;
    char default_geometry[100];
    XSizeHints sizehints;
    XGCValues gcvalues;

    /* We want the default display. (section 2.1 Opening the display)  */
    my_display = XOpenDisplay(NULL);
    if (my_display == NULL) return 0;
    
    /* Given a display, we can get the screen and the ``black'' and
       ``white'' pixels.  (section 2.2.1 Display macros)  */
    my_screen = DefaultScreen(my_display);     
    white = WhitePixel(my_display, my_screen);
    black = BlackPixel(my_display, my_screen);
    
    
    sizehints.x = DEFAULT_X_POSITION;
    sizehints.y = DEFAULT_Y_POSITION;
    sizehints.width = screenwidth;
    sizehints.height = screendepth;
    sizehints.flags = PPosition|PSize;

    sprintf (default_geometry, "%ux%u+%u+%u", screenwidth, screendepth,
                               DEFAULT_X_POSITION, DEFAULT_Y_POSITION);

    /* Look up the geometry for this window. (Section 10.2 Obtaining X
       environment defaults)  */
    geometry = XGetDefault(my_display, PROGRAM_NAME, ARG_GEOMETRY);
    
    if (geometry != NULL) {
       /* (section 10.3 Parsing window geometry) */
       int bitmask = XGeometry(my_display, my_screen,
			       geometry, default_geometry,
                               BORDER_WIDTH,
                               1, 1, /* ``Font'' width and height. */
                               0, 0, /* Interior padding. */
                               &(sizehints.x), &(sizehints.y),
                               &(sizehints.width), &(sizehints.height));
       
       /* (section 9.1.6 Setting and getting window sizing hints)  */
       if (bitmask & (XValue|YValue)) {
          sizehints.flags |= USPosition;
          geometry_found = 1;
       }
       
       if (bitmask & (WidthValue|HeightValue)) {
          sizehints.flags |= USSize;
          if (sizehints.width > screenwidth) sizehints.width = screenwidth;
          if (sizehints.height > screendepth) sizehints.height = screendepth;
          geometry_found = 1;
       }
    }
    
    
    /* Our window is pretty simple. (section 3.3 Creating windows)  */
    my_window = XCreateSimpleWindow(my_display,
                          DefaultRootWindow(my_display), /* parent */
                          sizehints.x, sizehints.y, /* upper left */
                          sizehints.width, sizehints.height,
                          BORDER_WIDTH,
			  black, /* border color */
                          white); /* background color */
    
    /* (section 9.1.1 Setting standard properties)  */
    XSetStandardProperties(my_display, my_window, 
                           PROGRAM_NAME,  /* window name */
                           PROGRAM_NAME,  /* icon name */
			   None,  /* pixmap for icon */
                           0, 0,  /* argv and argc for restarting */
                           &sizehints);
    XSetWMHints(my_display, my_window, &wm_hints);
    
    
    /* We need a graphics context if we're going to draw anything.
       (section 5.3 Manipulating graphics context/state)  */
    gcvalues.foreground = black;
    gcvalues.background = white;
    /* A ``thin'' line.  This is much faster than a line of length 1,
       although the manual cautions that the results might be less
       consistent across screens.  */
    gcvalues.line_width = 0;
    
    my_gc = XCreateGC(my_display, my_window,
		      GCForeground|GCBackground|GCLineWidth,
		      &gcvalues);
    
    /* (section 3.5 Mapping windows)  This is the confusing part of the
    program, at least to me. If no geometry spec was found, then the
    window manager puts up the blinking rectangle, and the user clicks,
    all before the following call returns. But if a geometry spec was
    found, then we want to do a whole mess of other things, because the
    window manager is going to send us an expose event so that we can
    bring our window up -- and this is one expose event we have to
    handle.  */
    XMapWindow(my_display, my_window);

    if (geometry_found) {
       /* The window manager sends us an Expose event. Yuck.
       */
       XEvent my_event;
       /* We certainly don't want to handle anything else.
          (section 8.5 Selecting events)
       */
       XSelectInput(my_display, my_window, ExposureMask);

       /* We also want to do this right now. This is the confusion. From
          stepping through the program under the debugger, it appears
          that it is this call to XSync (given the previous call to
          XSelectInput) that actually brings the window up -- and yet
          without the remaining code, the thing doesn't work right. Very
          strange. (section 8.6 Handling the output buffer)  
       */
       XSync(my_display, 0);

       /* Now get the event. (section 8.8.1 Returning the next event)
       */
       XNextEvent(my_display, &my_event);
       
       /* Ignore all but the last of the Expose events.
          (section 8.4.5.1 Expose event processing)
       */
       if (my_event.type == Expose && my_event.xexpose.count == 0) {
          /* Apparently the network might STILL have my_events coming in.
             Let's throw away Expose my_events again. (section 8.8.3
             Selecting my_events using a window or my_event mask)
          */
          while (XCheckTypedEvent(my_display, Expose, &my_event)) ;

          /* Finally, let's draw the blank screen.
          */
          XClearWindow(my_display, my_window);
       }
   }
    
    /* That's it.  */
    return 1;
}


/* Make sure the screen is up to date. (section 8.6 Handling the output
buffer)  */

mf_x11_updatescreen()
{
    XFlush(my_display);
}


/* Blank the rectangular inside the given coordinates. We don't need to
reset the foreground to black because we always set it at the beginning
of paintrow (below).  */

mf_x11_blankrectangle(left, right, top, bottom)
    screencol left, right;
    screenrow top, bottom;
{
    XSetForeground(my_display, my_gc, white);
    XFillRectangle(my_display, my_window, my_gc,
                   (int) left,
                   (int) top,
   	           (unsigned) (right - left + 1),
                   (unsigned) (bottom - top + 1));
}


/* Paint a row with the given ``transition specifications''. We might be
able to do something here with drawing many lines.  */

mf_x11_paintrow(row, init_color, tvect, vector_size)
    screenrow row;
    pixelcolor init_color;
    transspec tvect;
    register screencol vector_size;
{
    register int color, col;

    color = (init_color == 0) ? white : black;

    do {
	col = *tvect++;
	XSetForeground(my_display, my_gc, color);
        
        /* (section 6.3.2 Drawing single and multiple lines)
        */
	XDrawLine(my_display, my_window, my_gc, col, (int) row,
		  (int) (*tvect-1), (int) row);
                  
        color = (color == white) ? black : white;
    } while (--vector_size > 0);
}

#else
int x11_dummy;
#endif /* X11WIN */
