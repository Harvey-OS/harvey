/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum{
	Undef = 0x80000000,
	Play = 0,
	Record = 1,
};

typedef struct Audiocontrol Audiocontrol;

struct Audiocontrol {
	char	*name;
	uchar	readable;
	uchar	settable;
	uchar	chans;		/* 0 is master, non-zero is bitmap */
	long	value[8];	/* 0 is master; value[0] == Undef -> all values Undef */
	long	min, max, step;
};

extern Audiocontrol controls[2][Ncontrol];
extern int endpt[2];
extern int interface[2];
extern int featureid[2];
extern int selectorid[2];
extern int mixerid[2];
extern int buttonendpt;

int	ctlparse(char *s, Audiocontrol *c, long *v);
void	ctlevent(void);

#pragma	varargck	type	"A"	Audiocontrol*
