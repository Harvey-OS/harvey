/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <disk.h>

char *src[] = { "part", "disk", "guess" };
void
main(int argc, char **argv)
{
	Disk *disk;

	assert(argc == 2);
	disk = opendisk(argv[1], 0, 0);
	print("%d %d %d from %s\n", disk->c, disk->h, disk->s, src[disk->chssrc]);
}
