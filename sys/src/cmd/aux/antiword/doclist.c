/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * doclist.c
 * Copyright (C) 2004 A.J. van Os; Released under GNU GPL
 *
 * Description:
 * Build, read and destroy list(s) of Word document information
 *
 * Note:
 * There is no real list there is always one document per document
 */

#include "antiword.h"

#define HALF_INCH	36000L  /* In millipoints */

/* Variables needed to write the Document Information List */
static document_block_type *pAnchor = NULL;
static document_block_type tInfo;


/*
 * vDestroyDocumentInfoList - destroy the Document Information List
 */
void
vDestroyDocumentInfoList(void)
{
        DBG_MSG("vDestroyDocumentInfoList");

	pAnchor = NULL;
} /* end of vDestoryDocumentInfoList */

/*
 * vCreateDocumentInfoList - create the Document Information List
 */
void
vCreateDocumentInfoList(const document_block_type *pDocument)
{
	fail(pDocument == NULL);
	fail(pAnchor != NULL);

	tInfo = *pDocument;
	pAnchor = &tInfo;
} /* end of vCreateDocumentInfoList */

/*
 * lGetDefaultTabWidth - get the default tabwidth in millipoints
 */
long
lGetDefaultTabWidth(void)
{
	long	lDefaultTabWidth;
	USHORT	usTmp;

	if (pAnchor == NULL) {
		DBG_FIXME();
		return HALF_INCH;
	}
	usTmp = pAnchor->usDefaultTabWidth;
	lDefaultTabWidth = usTmp == 0 ? HALF_INCH : lTwips2MilliPoints(usTmp);
	NO_DBG_DEC(lDefaultTabWidth);
	return lDefaultTabWidth;
} /* end of lGetDefaultTabWidth */

/*
 * ucGetDopHdrFtrSpecification - get the Heder/footer specification
 */
UCHAR
ucGetDopHdrFtrSpecification(void)
{
	if (pAnchor == NULL) {
		DBG_FIXME();
		return 0x00;
	}
	return pAnchor->ucHdrFtrSpecification;
} /* end of ucGetDopHdrFtrSpecification */
