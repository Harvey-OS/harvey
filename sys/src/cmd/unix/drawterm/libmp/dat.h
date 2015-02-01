/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define	mpdighi  (mpdigit)(1U<<(Dbits-1))
#define DIGITS(x) ((Dbits - 1 + (x))/Dbits)

// for converting between int's and mpint's
#define MAXUINT ((uint)-1)
#define MAXINT (MAXUINT>>1)
#define MININT (MAXINT+1)

// for converting between vlongs's and mpint's
// #define MAXUVLONG (~0ULL)
// #define MAXVLONG (MAXUVLONG>>1)
// #define MINVLONG (MAXVLONG+1ULL)
#define MAXUVLONG ((uvlong) ~0)
#define MAXVLONG (MAXUVLONG>>1)
#define MINVLONG (MAXVLONG+((uvlong) 1))
