/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Somehow <errno.h> has been included on Mac OS X
 */
#undef EIO

extern char ENoDir[];
extern char EBadDir[];
extern char EBadMeta[];
extern char ENilBlock[];
extern char ENotDir[];
extern char ENotFile[];
extern char EIO[];
extern char EBadOffset[];
extern char ETooBig[];
extern char EReadOnly[];
extern char ERemoved[];
extern char ENotEmpty[];
extern char EExists[];
extern char ERoot[];
extern char ENoFile[];
extern char EBadPath[];
