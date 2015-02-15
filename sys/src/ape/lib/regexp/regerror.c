/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "regexp.h"

void
regerror(char *s)
{
	char buf[132];

	strcpy(buf, "regerror: ");
	strcat(buf, s);
	strcat(buf, "\n");
	fwrite(buf, 1, strlen(buf), stderr);
	exit(1);
}
