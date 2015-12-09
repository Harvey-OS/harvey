/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */



enum {
	Net20DCVid =	0x0525,	/* Ajays usb debug cable */
	Net20DCDid =	0x127a,
};

int	uconsmatch(char *info);
extern Serialops uconsops;
