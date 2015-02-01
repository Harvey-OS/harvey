/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __LIBL_H
#define __LIBL_H
#ifndef _RESEARCH_SOURCE
   This header file is not defined in ANSI or POSIX
#endif
#pragma lib "/$M/lib/ape/libl.a"

extern int printable(int);
extern void allprint(char);
extern int yyracc(int);
extern int yyreject(void);
extern void yyless(int);
extern int yywrap(void);

#endif /* __LIBV_L */
