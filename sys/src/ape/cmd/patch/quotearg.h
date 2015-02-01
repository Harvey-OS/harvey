/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* quote.h -- declarations for quoting system arguments */

#if defined __STDC__ || __GNUC__
# define __QUOTEARG_P(args) args
#else
# define __QUOTEARG_P(args) ()
#endif

size_t quote_system_arg __QUOTEARG_P ((char *, char const *));
