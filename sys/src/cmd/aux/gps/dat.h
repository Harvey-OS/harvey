/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Place Place;

struct Place {
	double	lon;
	double 	lat;
};
#pragma	varargck	type	"L"	Place

enum {
	Undef		= 0x80000000,
	Baud=		4800,		/* 4800 is NMEA standard speed */
};

extern Place nowhere;
extern int debug;

int placeconv(Fmt*);
Place strtopos(char*, char**);
int strtolatlon(char*, char**, Place*);
