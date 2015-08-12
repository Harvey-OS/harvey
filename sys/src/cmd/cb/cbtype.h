/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define _U 01
#define _L 02
#define _N 04
#define _S 010
#define _P 020
#define _C 040
#define _X 0100
#define _O 0200

extern unsigned char _cbtype_[]; /* in /usr/src/libc/gen/ctype_.c */

#define isop(c) ((_cbtype_ + 1)[c] & _O)
#define isalpha(c) ((_cbtype_ + 1)[c] & (_U | _L))
#define isupper(c) ((_cbtype_ + 1)[c] & _U)
#define islower(c) ((_cbtype_ + 1)[c] & _L)
#define isdigit(c) ((_cbtype_ + 1)[c] & _N)
#define isxdigit(c) ((_cbtype_ + 1)[c] & (_N | _X))
#define isspace(c) ((_cbtype_ + 1)[c] & _S)
#define ispunct(c) ((_cbtype_ + 1)[c] & _P)
#define isalnum(c) ((_cbtype_ + 1)[c] & (_U | _L | _N))
#define isprint(c) ((_cbtype_ + 1)[c] & (_P | _U | _L | _N))
#define iscntrl(c) ((_cbtype_ + 1)[c] & _C)
#define isascii(c) ((unsigned)(c) <= 0177)
#define toupper(c) ((c) - 'a' + 'A')
#define tolower(c) ((c) - 'A' + 'a')
#define toascii(c) ((c)&0177)
