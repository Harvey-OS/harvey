/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

#include <u.h>
#include <libc.h>

#include "boot.h"

void
configuroot(Method *m)
{
	print("configuroot\n");

	if (bind("#@", "/", MBEFORE) < 0) {
		fatal("bind #@");
	}

	// I think we need to extract the uroot archive into the ramdisk here

}

int
connecturoot(void)
{
	print("connecturoot\n");

	/*int fd = open("#@", OREAD);
	if (fd < 0) {
		werrstr("open /: %r");
	}

	return fd;*/

	return -1;
}
