/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#if defined(V9) || defined(BSD4_2) || defined(plan9)
#define _BSD_EXTENSION

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

char *
tempnam(char *dir, char *pfx)
{
	int pid;
	char *tnm;
	struct stat stb;
	static int seq = 0;

	if (dir == NULL)
		dir = ".";
#ifdef plan9
	/* our access emulation has a race when checking for write access */
	if (access(dir, R_OK|X_OK) == -1)
#else
	if (access(dir, R_OK|W_OK|X_OK) == -1)
#endif
		return NULL;
	pid = getpid();
	tnm = malloc(strlen(dir) + 1 + strlen(pfx) + 2*20 + 1);
	if (tnm == NULL)
		return NULL;
	do {
		sprintf(tnm, "%s/%s.%d.%d", dir, pfx, pid, seq++);
		errno = 0;
	} while (stat(tnm, &stb) >= 0 && seq < 256);
	return tnm;
}
#endif
