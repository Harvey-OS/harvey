/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

void
smbpathsplit(char *path, char **dirp, char **namep)
{
	char *dir;
	char *p = strrchr(path, '/');
	if (p == nil) {
		*dirp = smbestrdup("/");
		*namep = smbestrdup(path);
		return;
	}
	if (p == path)
		dir = smbestrdup("/");
	else {
		dir = smbemalloc(p - path + 1);
		memcpy(dir, path, p - path);
		dir[p - path] = 0;
	}
	*dirp = dir;
	*namep = smbestrdup(p + 1);
}
