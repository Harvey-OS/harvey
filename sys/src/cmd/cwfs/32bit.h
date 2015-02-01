/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * fundamental constants and types of the implementation
 * changing any of these changes the layout on disk
 */

/*
 * compatible on disk with the old 32-bit file server and can also speak 9P1.
 * this lets people run this file server on their old file systems.
 * DON'T TOUCH or you'll break compatibility.
 */
enum {
	NAMELEN		= 28,		/* max size of file name components */
	NDBLOCK		= 6,		/* number of direct blocks in Dentry */
	NIBLOCK		= 2,		/* max depth of indirect blocks */
};

typedef long Off;	/* file offsets & sizes, in bytes & blocks */

#define COMPAT32
#define swaboff swab4
