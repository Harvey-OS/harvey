/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __LIBNET_H
#define __LIBNET_H
#ifndef _NET_EXTENSION
   This header file is not defined in ANSI or POSIX
#endif
#pragma lib "/$M/lib/ape/libnet.a"

#define NETPATHLEN 40

extern	int	accept(int, char*);
extern	int	announce(char*, char*);
extern	int	dial(char*, char*, char*, int*);
extern	int	hangup(int);
extern	int	listen(char*, char*);
extern	char*	netmkaddr(char*, char*, char*);
extern	int	reject(int, char*, char *);

extern char    dialerrstr[64];

#endif /* __LIBNET_H */
