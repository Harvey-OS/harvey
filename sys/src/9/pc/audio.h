/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum
{
	Bufsize	= 1024,	/* 5.8 ms each, must be power of two */
	Nbuf		= 128,	/* .74 seconds total */
	Dma		= 6,
	IrqAUDIO	= 7,
	SBswab	= 0,
};

#define seteisadma(a, b)	dmainit(a, Bufsize);
#define UNCACHED(type, v)	(type*)((ulong)(v))

#define Int0vec
#define setvec(v, f, a)		intrenable(v, f, a, BUSUNKNOWN, "audio")
