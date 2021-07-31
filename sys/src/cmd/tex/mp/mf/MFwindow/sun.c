/*
 * Graphics window interface to Metafont for Suns running
 *	SunWindows (Sun Unix release 1.2 or later?)
 */

#define	EXTERN	extern
#include "../mfd.h"

#ifdef	SUNWIN

#include <stdio.h>
#include <signal.h>
#include <suntool/gfx_hs.h>

/*
 * global handle on the graphics subwindow (if run inside gfxtool)
 */

struct	gfxsubwindow	*mf_gfxwin;	/* window handle */

/*
 * Gray background for graphics area
 */

short	mf_graybackground_image[] = {0x5555, 0xaaaa};
	mpr_static(mf_graybackground, 2, 2, 1, mf_graybackground_image);


/*
 * sigwinch(): handle SIGWINCH signal to refresh graphics
 */

static
sigwinch() {
	gfxsw_handlesigwinch(mf_gfxwin);
}


/*
 * init_screen: boolean;  return true if window operations legal
 */

mf_sun_initscreen()
{
	if ((mf_gfxwin = gfxsw_init(0, (char **)NULL)) == NULL) {
		fprintf(stderr, "! Window access requires METAFONT to run under gfxtool\n");
		return(0);
	}
	gfxsw_getretained(mf_gfxwin);	/* let sunwindows repair damage */
	pw_replrop(mf_gfxwin->gfx_pixwin,
				0, 0,
				mf_gfxwin->gfx_rect.r_width,
				mf_gfxwin->gfx_rect.r_height,
				PIX_SRC,
				&mf_graybackground, 0, 0);	/* clear subwindow */
	(void) signal(SIGWINCH, sigwinch);
	return(1);
}

/*
 * updatescreen; -- just make sure screen is ready to view
 */

mf_sun_updatescreen()
{
	if (mf_gfxwin->gfx_flags & GFX_DAMAGED)
		gfxsw_handlesigwinch(mf_gfxwin);
	if (mf_gfxwin->gfx_flags & GFX_RESTART) {
		mf_gfxwin->gfx_flags &= ~GFX_RESTART;
		pw_replrop(mf_gfxwin->gfx_pixwin,
					0, 0,
					mf_gfxwin->gfx_rect.r_width,
					mf_gfxwin->gfx_rect.r_height,
					PIX_SRC,
					&mf_graybackground, 0, 0);	/* clear subwindow */
	}
}

/*
 * blankrectangle: reset rectangle bounded by ([left,right],[top,bottom])
 *			to background color
 */

mf_sun_blankrectangle(left, right, top, bottom)
	screencol left, right;
	screenrow top, bottom;
{
	pw_writebackground(mf_gfxwin->gfx_pixwin, left, top,
				right-left+1, bottom-top+1, PIX_CLR);
}

/*
 * paintrow -- paint "row" starting with color "init_color",  up to next
 *		transition specified by "transition_vector", switch colors,
 *		and continue for "vector_size" transitions.
 */

mf_sun_paintrow(row, init_color, transition_vector, vector_size)
	screenrow	row;
	pixelcolor	init_color;
	transspec	transition_vector;
	screencol	vector_size;
{
	register	col;
	register	color;

	color = (init_color == 0)? 0 : 1;

	do {
		col = *transition_vector++;
		pw_vector(mf_gfxwin->gfx_pixwin,
				col, row, (*transition_vector)-1, row,
				PIX_SRC, color);
		color = 1 - color;
	} while (--vector_size);
}

#else
int sunview_dummy;
#endif /* SUNWIN */
