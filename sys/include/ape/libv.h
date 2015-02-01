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
extern long	lrand(void);
extern double	frand(void);

extern char	*getpass(char *);
extern int	tty_echoon(int);
extern int	tty_echooff(int);

extern int	min(int, int);
extern int	max(int, int);

extern void	_perror(char *);
extern char	*_progname;

extern int	nap(int);

extern char	*setfields(char *);
extern int	getfields(char *, char **, int);
extern int	getmfields(char *, char **, int);


#ifdef __cplusplus
};
#endif

#endif /* __LIBV_H */
