/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
	local limits
*/

#undef	ARG_MAX
#define	ARG_MAX		16384
#undef	CHILD_MAX
#define	CHILD_MAX	75
#undef	OPEN_MAX
#define	OPEN_MAX	96
#undef	LINK_MAX
#define	LINK_MAX	1
#undef	PATH_MAX
#define	PATH_MAX	1023
#undef	NAME_MAX
#define	NAME_MAX	PATH_MAX
#undef	NGROUPS_MAX
#define	NGROUPS_MAX	32
#undef	MAX_CANON
#define	MAX_CANON	1023
#undef	MAX_INPUT
#define	MAX_INPUT	1023
#undef	PIPE_BUF
#define	PIPE_BUF	8192

#define	_POSIX_SAVED_IDS		1
#define	_POSIX_CHOWN_RESTRICTED		1
#define	_POSIX_NO_TRUNC			1
