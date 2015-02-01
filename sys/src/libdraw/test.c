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
#include <draw.h>


void
main(int argc, char **argv)
{
		print("%dn", wordsperline(Rect(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4])), atoi(argv[5])));
}
