/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma src "/sys/src/libscribble"
#pragma lib "libscribble.a"

#pragma incomplete struct graffiti

typedef struct Scribble Scribble;
typedef struct graffiti Graffiti;

typedef struct pen_point {
	Point;
	long	chaincode;
} pen_point;

typedef struct Stroke {
	uint			npts;	/*Number of pen_point in array.*/
	pen_point*	pts;	/*Array of points.*/
} Stroke;

#define CS_LETTERS     0
#define CS_DIGITS      1
#define CS_PUNCTUATION 2

struct Scribble {
	/* private state */
	Point		*pt;
	int			ppasize;
	Stroke	    	ps;
	Graffiti	*graf;
	int			capsLock;
	int			puncShift;
	int			tmpShift;
	int			ctrlShift;
	int			curCharSet;
};

Rune		recognize(Scribble *);
Scribble *	scribblealloc(void);

extern int ScribbleDebug;
