/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef KSH_TIMES_H
# define KSH_TIMES_H

/* Needed for clock_t on some systems (ie, NeXT in non-posix mode) */
#include "ksh_time.h"

#include <sys/times.h>

#ifdef TIMES_BROKEN
extern clock_t	ksh_times ARGS((struct tms *));
#else /* TIMES_BROKEN */
# define ksh_times times
#endif /* TIMES_BROKEN */

#ifdef HAVE_TIMES
extern clock_t	times ARGS((struct tms *));
#endif /* HAVE_TIMES */
#endif /* KSH_TIMES_H */
