/* Tektronix terminal window interface for Metafont.  */

#define	EXTERN	extern
#include "../mfd.h"

#ifdef TEKTRONIXWIN		/* Whole file */

#ifdef SYSV
#define bzero(s,n)	memset (s, 0, n)
#define bcmp(s1,s2,n)	memcmp (s1, s2, n)
#endif

#define FORMFEED	12
#define ESCAPE		27
#define GS		29
#define US		31
#define TRUE		1
#define FALSE		0
#define WIDTH		1024	/* Screen width */
#define HEIGHT		780	/* Screen height */
#define WIDTHINBYTES	(WIDTH/8)   /* Only works if WIDTH % 8 == 0 */
#define SETBIT(row,col) screen_pixel[row*WIDTHINBYTES+col/8] |= 1 << 7-col%8
#define CLEARBIT(row,col) screen_pixel[row*WIDTHINBYTES+col/8] &= \
			    ~(1 << 7-col%8)
#define ISSET(row,col) (screen_pixel[row*WIDTHINBYTES+col/8] & 1 << 7-col%8)

char	screen_pixel[WIDTHINBYTES*HEIGHT];
char	zero_array[WIDTHINBYTES];

/* 
 *	function init_screen: boolean;
 *
 *		Return true if window operations legal.
 *		We always return true.
 */

mf_tektronix_initscreen()
{
    bzero(zero_array, sizeof(zero_array));
    return 1;
}

/*
 *	procedure updatescreen;
 *
 *		Print out the screen bitmap.
 */
mf_tektronix_updatescreen()
{
    int r, c, startc, makingline;

    printf("%c%c", ESCAPE, FORMFEED);
    for (r = 0; r < HEIGHT; r++) {
	makingline = FALSE;
	if (bcmp(&screen_pixel[r*WIDTHINBYTES],zero_array,WIDTHINBYTES) == 0)
	    continue;
	for (c = 0; c < WIDTH; c++) {
	    if (ISSET(r, c)) {
		if (!makingline) {
		    makingline = TRUE;
		    startc = c;
		}
	    } else if (makingline) {
		putchar(GS);
		putchar(0x20|((HEIGHT-1)-r)>>5);
		putchar(0x60|((HEIGHT-1)-r)&0x1F);
		putchar(0x20|startc>>5);
		putchar(0x40|startc&0x1F);
		putchar(0x60|((HEIGHT-1)-r)&0x1F); /* Don't send high y */
		putchar(0x20|c>>5);
		putchar(0x40|c&0x1F);
		makingline = FALSE;
	    }
	}
	if (makingline)  {
	    putchar(GS);
	    putchar(0x20|((HEIGHT-1)-r)>>5);
	    putchar(0x60|((HEIGHT-1)-r)&0x1F);
	    putchar(0x20|startc>>5);
	    putchar(0x40|startc&0x1F);
	    putchar(0x60|((HEIGHT-1)-r)&0x1F); /* Don't send high y */
	    putchar(0x20|c>>5);
	    putchar(0x40|c&0x1F);
	}
    }
    printf("%c%c%c%c%c", GS, 0x23, 0x66, 0x20, 0x40);
    putchar(US);
    fflush(stdout);
}

/*
 *	procedure blankrectangle(left, right, top, bottom: integer);
 *
 *		Blanks out a port of the screen.
 */
mf_tektronix_blankrectangle(left, right, top, bottom)
screencol left, right;
screenrow top, bottom;
{
    int	r, c;

    if (left == 0 && right == WIDTH && top == 0 && bottom == HEIGHT)
	bzero(screen_pixel, sizeof(screen_pixel));
    else 
	for (r = top; r < bottom; r++)
	    for (c = left; c < right; c++)
		CLEARBIT(r, c);
}

/*
 *	procedure paintrow(
 *			row:		screenrow;
 *			init_color:	pixelcolor;
 *		var	trans_vector:	transspec;
 *			vector_size:	screencol);
 *
 *		Paint "row" starting with color "init_color", up to next
 *		transition specified by "transition_vector", switch colors,
 *		and continue for "vector_size" transitions.
 */
mf_tektronix_paintrow(row, init_color, transition_vector, vector_size)
screenrow   row;
pixelcolor  init_color;
transspec   transition_vector;
screencol   vector_size;
{
    int k = 0;
    int c = transition_vector[0];

    do {
	k++;
	do {
	    if (init_color)
		SETBIT(row, c);
	    else
		CLEARBIT(row, c);
	} while (++c != transition_vector[k]);
	init_color = !init_color;
    } while (k != vector_size);
}

#else
int tek_dummy;
#endif	/* TEKTRONIXWIN */
