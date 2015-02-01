/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Plotting functions for v8 and v9 systems */
/* This file is an alternative to plot.h */

/* open the plotting output */
#define openpl()  print("o\n")

/* close the plotting output */
#define closepl()  print("cl\n")

/* make sure the page or screen is clear */
#define erase() print("e\n")

/* plot a point at _x,_y, which becomes current */
#define point(_x,_y)  print("poi %d %d\n", _x,_y)

/* coordinates to be assigned to lower left and upper right
   corners of (square) plotting area */
#define range(_x,_y,_X,_Y)  print("ra %d %d %d %d\n", _x,_y,_X,_Y)

/* place text, first letter at current point, which does not change */
#define text(_s)  {if(*(_s) == ' ')print("t \"%s\"\n",_s); else print("t %s\n", _s); }

/* draw line from current point to _x,_y, which becomes current */
#define vec(_x,_y)  print("v %d %d\n", _x,_y)

/* _x,_y becomes current point */
#define move(_x, _y)  print("m %d %d\n", _x, _y)

/* specify style for drawing lines */

#define SOLID "solid"
#define DOTTED "dotted"
#define DASHED "dashed"
#define DOTDASH "dotdash"

#define pen(_s)  print("pe %s\n", _s)

#define BLACK "z"
#define RED "r"
#define YELLOW "y"
#define GREEN "g"
#define BLUE "b"
#define CYAN "c"
#define MAGENTA "m"
#define WHITE "w"

#define colorcode(_s) ((strcmp(_s,"black")==0)?BLACK:_s)

#define colorx(_s) print("co %s\n", _s);	/* funny name is all ken's fault */

#define comment(s,f)
