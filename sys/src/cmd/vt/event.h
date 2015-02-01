/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define BSIZE	4000
#define MOUSE	0
#define KBD	1
#define	HOST	2
#define HOST_BLOCKED	1
#define KBD_BLOCKED	2

typedef struct IOEvent {
	short	key;
	short	size;
	uchar	data[BSIZE];
} IOEvent;

