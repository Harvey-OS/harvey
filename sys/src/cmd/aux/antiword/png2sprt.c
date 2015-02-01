/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * png2sprt.c
 * Copyright (C) 2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to translate png pictures into sprites
 */

#include <stdio.h>
#include "antiword.h"


/*
 * bTranslatePNG - translate a PNG picture
 *
 * This function translates a picture from png to sprite
 *
 * return TRUE when sucessful, otherwise FALSE
 */
BOOL
bTranslatePNG(diagram_type *pDiag, FILE *pFile,
	ULONG ulFileOffset, size_t tPictureLen, const imagedata_type *pImg)
{
	/* PNG is not supported yet */
	return bAddDummyImage(pDiag, pImg);
} /* end of bTranslatePNG */
