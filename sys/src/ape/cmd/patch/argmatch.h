/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* argmatch.h -- declarations for matching arguments against option lists */

#if defined __STDC__ || __GNUC__
# define __ARGMATCH_P(args) args
#else
# define __ARGMATCH_P(args) ()
#endif

int argmatch __ARGMATCH_P ((const char *, const char * const *));
void invalid_arg __ARGMATCH_P ((const char *, const char *, int));

extern char const program_name[];
