/* Link this to sun.c if you are running on a SunView system */

/*
 *  author = "Pierre MacKay (from the original by Paul Richards)"
 *  version = "0.5",
 *  date = "5 May 1991"
 *  filename = "mf_sunview.c",
 *  contact = "Pierre MacKay",
 *  email = "mackay@cs.washington.edu"
 * Graphics window interface to Metafont for Suns running
 *	SunView (Sun Unix release 3.n)
 * This code is essentially a translation from the old
 * sun.c which used <suntool/gfxsw.h>.  The model for the
 * translation is the example in Appendix C (C.2.) of the
 * Sunview 1 Programmer's guide as applied to bouncedemo.c
 * There are nice additional benefits, such as a moveable
 * and scalable graphics window, and the chance to see
 * the controlling window while METAFONT is at work
 *
 * Destroy checking is bogus, and SunView does not clean up the mess
 * if you "quit" from the pulldown menu.  You will be left with
 * an hysterical orphaned process screaming for a window that isn't
 * there.  You will have to go elsewhere ant KILL that process.
 * Otherwise it seems clean.
 * 
 * For the moment this and the SunView interface seem to be
 * mutually exclusive, since they use library routines with
 * the same names, which mightily confuses the loader. 
 * It doesn't help much to change the name of MFTERM since
 * XView wants to name its own terminal "sun" just like
 * the SunView terminal
 */

#define	EXTERN	extern
#include "../mfd.h"

#undef _POSIX_SOURCE
#ifdef	SUNWIN

/* <sundev/cg9reg.h> wants to have a field named `reset'.  Since we
   don't need the WEB(2C) reset, no problem.  */
#undef reset

#include <stdio.h>
#include <sys/types.h>
#include <suntool/sunview.h>
#include <suntool/canvas.h>

static void repaint_proc();
static void resize_proc();

static Notify_value mf_notice_destroy();
extern Notify_error notify_dispatch();

static int destroy_ready;  /* could be used for tighter control */

/*
 * global handle on the graphics subwindow 
 */

struct	MFsubwindow {
	int	mf_flags;
#define	MF_RESTART	0x01
	struct	pixwin *mf_pixwin;
	struct	rect mf_rect;
      } sun_mf_subwin; /* Make sure that names storage is allocated. */

static struct	MFsubwindow	*metafont_sw = &sun_mf_subwin;	/* window handle */

/*
 * Gray background for graphics area
 */

static short	mf_graybackground_image[] = {0x5555, 0xaaaa};
	mpr_static(mf_sunview_gray_bkgr, 2, 2, 1, mf_graybackground_image);


Rect *rect;
Frame frame;
Canvas canvas;
Pixwin *pw;

/*
 * init_screen: boolean;  return true if window operations legal
 */

mf_sunview_initscreen()
{
	frame = window_create(NULL,FRAME,
			      FRAME_LABEL, "METAFONT",
			      FRAME_SHOW_LABEL, TRUE,
			      WIN_ERROR_MSG, "! Window access requires METAFONT to run under suntools\n",
			      0);
	canvas = window_create(frame, CANVAS,
			       CANVAS_RESIZE_PROC, resize_proc,
			       CANVAS_FAST_MONO, TRUE,
			       WIN_ERROR_MSG, "Can't creat canvas",
			       0);
	pw = canvas_pixwin(canvas);

	metafont_sw->mf_pixwin = canvas_pixwin(canvas);

	/* interpose a destroy procedure so we can shut down cleanly */
	(void) notify_interpose_destroy_func(frame, mf_notice_destroy);

	/* 
	 * Instead of using window_main_loop, just show the frame.
	 * Metafont's procedures will be in control, not the notifier.
	 */
	window_set(frame,WIN_SHOW, TRUE, 0);

	rect = (Rect *)window_get(canvas, WIN_RECT);  /* Get current dimensions */
	pw_replrop(pw,
		   0, 0,
		   rect->r_width,
		   rect->r_height,
		   PIX_SRC,
		   &mf_sunview_gray_bkgr, 0, 0);	/* clear subwindow */

	return(1); /* Caller expects a TRUE value */
}

/*
 * updatescreen; -- just make sure screen is ready to view
 */

mf_sunview_updatescreen()
{
	(void)notify_dispatch();
/*	if (destroy_ready != 0) return; */
	rect = (Rect *)window_get(canvas, WIN_RECT);  /* Get current dimensions */
	if (metafont_sw->mf_flags & MF_RESTART) {
		metafont_sw->mf_flags &= ~MF_RESTART;
		pw_replrop(pw,
			   0, 0,
			   rect->r_width,
			   rect->r_height,
			   PIX_SRC,
			   &mf_sunview_gray_bkgr, 0, 0);	/* clear subwindow */
	}
}

/*
 * blankrectangle: reset rectangle bounded by ([left,right],[top,bottom])
 *			to background color
 */

mf_sunview_blankrectangle(left, right, top, bottom)
	screencol left, right;
	screenrow top, bottom;
{
	pw_writebackground(pw, left, top,
				right-left+1, bottom-top+1, PIX_CLR);
}

/*
 * paintrow -- paint "row" starting with color "init_color",  up to next
 *		transition specified by "transition_vector", switch colors,
 *		and continue for "vector_size" transitions.
 */

mf_sunview_paintrow(row, init_color, transition_vector, vector_size)
	screenrow	row;
	pixelcolor	init_color;
	transspec	transition_vector;
	screencol	vector_size;
{
	register	col;
	register	color;

	(void)notify_dispatch();
	color = (init_color == 0)? 0 : 1;

	do {
		col = *transition_vector++;
		pw_vector(pw,
			  col, row, (*transition_vector)-1, row,
			  PIX_SRC, color);
		color = 1 - color;
	} while (--vector_size);
}

static void
repaint_proc( /* Ignore args */ )
{
	/* if repainting is required, just restart */
	metafont_sw->mf_flags |= MF_RESTART;
}

static void
resize_proc( /* Ignore args */ )
{
	metafont_sw->mf_flags |= MF_RESTART;
}

static	Notify_value
mf_notice_destroy(frame, status)
	Frame frame;
	Destroy_status status;
{
	if (status != DESTROY_CHECKING) {
	  destroy_ready = TRUE;
	  (void)notify_stop();
	}
	return(notify_next_destroy_func(frame,status));
}

#else
int sunview_dummy;
#endif /* SUNWIN */
