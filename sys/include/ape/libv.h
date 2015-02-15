/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __LIBV_H
#define __LIBV_H
#ifndef _RESEARCH_SOURCE
   This header file is not defined in ANSI or POSIX
#endif
#pragma lib "/$M/lib/ape/libv.a"

#ifdef __cplusplus
extern "C" {
#endif

extern void	srand(unsigned int);
extern int	rand(void);
extern int	nrand(int);
extern int32_t	lrand(void);
extern double	frand(void);

extern int8_t	*getpass(int8_t *);
extern int	tty_echoon(int);
extern int	tty_echooff(int);

extern int	min(int, int);
extern int	max(int, int);

extern void	_perror(int8_t *);
extern int8_t	*_progname;

extern int	nap(int);

extern int8_t	*setfields(int8_t *);
extern int	getfields(int8_t *, int8_t **, int);
extern int	getmfields(int8_t *, int8_t **, int);


#ifdef __cplusplus
};
#endif

#endif /* __LIBV_H */
