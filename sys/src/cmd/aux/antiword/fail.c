/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * fail.c
 * Copyright (C) 1998 A.J. van Os
 *
 * Description:
 * An alternative form of assert()
 */

#include <stdlib.h>
#include "antiword.h"

#if !defined(NDEBUG)
void
__fail(char *szExpression, char *szFilename, int iLineNumber)
{
	if (szExpression == NULL || szFilename == NULL) {
		werr(1, "Internal error: no expression");
	}
#if defined(DEBUG)
	fprintf(stderr, "%s[%3d]: Internal error in '%s'\n",
		szFilename, iLineNumber, szExpression);
#endif /* DEBUG */
	werr(1, "Internal error in '%s' in file %s at line %d",
		szExpression, szFilename, iLineNumber);
} /* end of __fail */
#endif /* !NDEBUG */
