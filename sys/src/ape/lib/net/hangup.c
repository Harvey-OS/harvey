/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <libnet.h>

/*
 *  force a connection to hangup
 */
int
hangup(int ctl)
{
	return write(ctl, "hangup", sizeof("hangup") - 1) != sizeof("hangup") - 1;
}
