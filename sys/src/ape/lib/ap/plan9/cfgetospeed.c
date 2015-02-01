/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <termios.h>

speed_t
cfgetospeed(const struct termios *)
{
	return B0;
}

int
cfsetospeed(struct termios *, speed_t)
{
	return 0;
}

speed_t
cfgetispeed(const struct termios *)
{
	return B0;
}

int
cfsetispeed(struct termios *, speed_t)
{
	return 0;
}

