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

extern	int	accept(int, int8_t*);
extern	int	announce(int8_t*, int8_t*);
extern	int	dial(int8_t*, int8_t*, int8_t*, int*);
extern	int	hangup(int);
extern	int	listen(int8_t*, int8_t*);
extern	int8_t*	netmkaddr(int8_t*, int8_t*, int8_t*);
extern	int	reject(int, int8_t*, int8_t *);

extern int8_t    dialerrstr[64];

#endif /* __LIBNET_H */
