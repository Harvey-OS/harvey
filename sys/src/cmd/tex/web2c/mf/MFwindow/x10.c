/*
 * X Version 10 Window interface for Metafont
 *
 * Tim Morgan  2/13/88
 */

#define	EXTERN	extern
#include "../mfd.h"

#ifdef	X10WIN
#include <X/Xlib.h>

static Display *my_display;
static Window my_window;

/* Return 1 if display opened successfully, else 0 */
mf_x10_initscreen()
{
    char default_geometry[50];
    OpaqueFrame outer_frame;

    my_display = XOpenDisplay("");
    if (!my_display) return(0);
    (void) sprintf(default_geometry, "=%dx%d", screenwidth, screendepth);
    outer_frame.bdrwidth = 2;
    outer_frame.border = BlackPixmap;
    outer_frame.background = BlackPixmap;
    my_window = XCreate("Metafont", "mf", (char *) 0,
	default_geometry, &outer_frame, screenwidth, screendepth);
    XMapWindow(my_window);
    return(1);
}

/* Make sure the screen is up to date */
mf_x10_updatescreen()
{
    XFlush();
}

mf_x10_blankrectangle(left, right, top, bottom)
screencol left, right;
screenrow top, bottom;
{
    XPixSet(my_window, (int) left, (int) top, (int) (right-left+1),
	(int) (bottom-top+1), WhitePixel);
}

mf_x10_paintrow(row, init_color, tvect, vector_size)
screenrow row;
pixelcolor init_color;
transspec tvect;
register screencol vector_size;
{
    register int color, col;

    color = (init_color == 0) ? WhitePixel : BlackPixel;

    do {
	col = *tvect++;
	XLine(my_window, col, (int) row, (int) (*tvect-1), (int) row,
		1, 1, color, GXcopy, AllPlanes);
	if (color == WhitePixel) color = BlackPixel;
	else color = WhitePixel;
    } while (--vector_size > 0);
}

#else
int x10_dummy;
#endif /* X10WIN */
