/*
 * HP2627 Terminal Window interface to Unix Metafont.
 */

#define	EXTERN	extern
#include "../mfd.h"

#ifdef	HP2627WIN

#include <stdio.h>

/*
 * HP2627a Color Graphics Terminal: Escape code definitions
 *
 * 	Drawing pen colors
 */
#define	HP2627_BLACK		'0'
#define	HP2627_RED		'1'
#define	HP2627_GREEN		'2'
#define	HP2627_YELLOW		'3'
#define	HP2627_BLUE		'4'
#define	HP2627_MAGENTA		'5'
#define	HP2627_CYAN		'6'
#define	HP2627_WHITE		'7'

/*
 *	Initialization: just do a hard graphics reset
 *		(then we can depend on the defaults
 *		 being as we want them)
 */
#define	HP2627_INITSTRING	"\033*wR"
#define	HP2627_INITSTRINGLEN	4

/*
 *	We want the text to be visible over both background and forground
 *		graphics data; the best color combination I found for this
 *		was to set the background RED and then paint with BLUE,
 *		although the eye doesn't focus on BLUE very well (black 
 *		might be better?  Or green? [to get in the holiday mood])
 */
#define	HP2627_BACKGROUNDPEN	HP2627_RED
#define	HP2627_FOREGROUNDPEN	HP2627_BLUE

static	char	mf_hp2627_pencolors[2] = {
		HP2627_BACKGROUNDPEN,		/* white */
		HP2627_FOREGROUNDPEN		/* black */
};

/*
 * 	Screen dimensions:  Note the origin is at the lower-left corner,
 *		not the upper left as MF expects - hence we need to translate.
 */
#define	HP2627_MAXX		511
#define	HP2627_MAXY		389

/*
 * The actual Graphics routines.  Note that these are highly tty
 *	dependent so I can minimize the number of characters that
 *	need to be sent to paint an image, since we only talk to
 *	the HP at 9.6Kb.
 */

/*
 * 	function init_screen: boolean;
 *
 *		Return true if window operations legal.
 *		We always return true (I suppose we could try to
 *		sense status or something masochistic like that)
 */

mf_hp2627_initscreen()
{
	(void) fflush(stdout);	/* make sure pascal-level output flushed */
	(void) write(fileno(stdout), HP2627_INITSTRING, HP2627_INITSTRINGLEN);
	return(1);
}

/*
 *	procedure updatescreen; 
 *
 *		Just make sure screen is ready to view
 *		(since we do Unix-level WRITE()s,
 *		we don't really need to flush stdio,
 *		but why not?)
 */

mf_hp2627_updatescreen()
{
	(void) fflush(stdout);
}

/*
 *	procedure blankrectangle(left, right, top, bottom: integer);
 *
 *		 reset rectangle bounded by ([left,right],[top,bottom])
 *			to background color
 */

mf_hp2627_blankrectangle(left, right, top, bottom)
	screencol left, right;
	screenrow top, bottom;
{
	char	sprbuf[128];

	(void) sprintf(sprbuf, "\033*m%cx%d,%d %d,%dE", HP2627_BACKGROUNDPEN,
			left, HP2627_MAXY-bottom,
			right, HP2627_MAXY-top);
	(void) write(fileno(stdout), sprbuf, strlen(sprbuf));
}

/*
 *	procedure paintrow(
 *			row:		screenrow;
 *			init_color:	pixelcolor;
 *		var	trans_vector:	transspec;
 *			vector_size:	screencol		);
 *
 *		Paint "row" starting with color "init_color",  up to next
 *		transition specified by "transition_vector", switch colors,
 *		and continue for "vector_size" transitions.
 */

/*
 * 	First some useful definitions:
 */
#define	ASCIILABS	0		/* plot cmd format: ASCII long abs    */
#define	BINLABS		1		/* plot cmd format: BINARY long abs   */
#define	BINSINC		2		/* plot cmd format: BINARY short incr */

#define	ABS(x) ((x>=0)?x:-x)		/* absolute value		      */

/*
 *	An optimized "move to (x,y), with current pen lowered unless UP is
 *		true" function.  Takes advantage of short commands for short
 *		movements to minimize character traffic to term.
 *
 *		Note: the "x -= 1;" fudge is because we only want to DRAW
 *			to the next transition_vector entry - 1, but if we
 *			have the pen raised, we want to move to exactly the
 *			next point.
 */
#define	MOVETO(x,y,up) \
	if (up) *op++ = 'a'; \
	else x -= 1; \
	if (ABS(x-oldx) < 16 && ABS(y-oldy) < 16) { \
		if (currentformat != BINSINC) { \
			*op++ = 'j'; \
			currentformat = BINSINC; \
		} \
		*op++ = (((x-oldx) & 0x1f) + ' '); \
		*op++ = (((y-oldy) & 0x1f) + ' '); \
	} else { \
		if (currentformat != BINLABS) { \
			*op++ = 'i'; \
			currentformat = BINLABS; \
		} \
		*op++ = (((x&0x3e0)>>5)+' '); \
		*op++ =  ((x&0x01f)    +' '); \
		*op++ = (((y&0x3e0)>>5)+' '); \
		*op++ =  ((y&0x01f)    +' '); \
	} \
	oldx = x; \
	oldy = y;

mf_hp2627_paintrow(row, init_color, transition_vector, vector_size)
	screenrow	row;
	pixelcolor	init_color;
	transspec	transition_vector;
	screencol	vector_size;
{
	register	color;
	char		outbuf[512*6];	/* enough to hold an alternate color */
					/* in each column		     */
	register	char	*op;
	register	x, y, oldx, oldy;
	int		currentformat;

	color = (init_color == 0)? 0 : 1;

	/*
	 * We put all escape sequences in a buffer so the write
	 * has a chance of being atomic, and not interrupted by
	 * other independent output to our TTY.  Also to avoid
	 * (literally millions) of stdio calls.
	 */
	op = outbuf;
	/*
	 * Select current pen to be the color of the first segment:
	 *
	 * our strategy here is to paint a long line from the first
	 * transition_vector value (left edge of row) to the last
	 * transition_vector entry (right edge of row).  Then we switch
	 * colors to the contrasting color, and paint alternate
	 * segments with that color.  Would that the HP2627 would provide
	 * a mode to paint the "background" color while the PEN is lifted.
	 * However, this is faster than using rectangular area fills.
	 */

	*op++ = '\033'; *op++ = '*'; *op++ = 'm';
	*op++ = mf_hp2627_pencolors[color];
	*op++ = 'X';

	/*
	 * Reset our "remembered" state for (X,Y) positioning and plot
	 *	command format
	 */
	oldx = oldy = -999;
	currentformat = ASCIILABS;

	/*
	 * Now, paint across the entire width of this row, make it the
	 *	initial segment color.
	 */
	x = *transition_vector;
	y = HP2627_MAXY-row;
	*op++ = '\033'; *op++ = '*'; *op++ = 'p';
	MOVETO(x,y,1);
	x = transition_vector[vector_size];
	MOVETO(x,y,0);
	*op++ = 'Z';

	/*
	 * If there remain other segments (of contrasting color) to paint,
	 *	switch pens colors and draw them
	 */
	if (--vector_size > 0) {
		*op++ = '\033'; *op++ = '*'; *op++ = 'm';
		*op++ = mf_hp2627_pencolors[1-color]; *op++ = 'X';
		color = 1-color;

		oldx = oldy = -999;
		currentformat = ASCIILABS;
		*op++ = '\033'; *op++ = '*'; *op++ = 'p';
		x = *++transition_vector;
		MOVETO(x,y,1);
		while (vector_size-- > 0) {
			x = *++transition_vector;
			MOVETO(x,y,(color==init_color));
			color = 1 - color;
		};
		*op++ = 'Z';
	};

	/*
	 * Write the resulting plot commands, hopefully atomically
	 */
	(void) write(fileno(stdout), outbuf, op-outbuf);
}

#else
int hp2627_dummy;
#endif	/* HP2627WIN */
