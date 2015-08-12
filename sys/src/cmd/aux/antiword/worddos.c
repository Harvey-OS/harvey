/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * worddos.c
 * Copyright (C) 2002-2005 A.J. van Os; Released under GNU GPL
 *
 * Description:
 * Deal with the DOS internals of a MS Word file
 */

#include "antiword.h"

/*
 * bGetDocumentText - make a list of the text blocks of a Word document
 *
 * Return TRUE when succesful, otherwise FALSE
 */
static BOOL
bGetDocumentText(FILE* pFile, int32_t lFilesize, const UCHAR* aucHeader)
{
	text_block_type tTextBlock;
	ULONG ulTextLen;
	BOOL bFastSaved;
	UCHAR ucDocStatus, ucVersion;

	fail(pFile == NULL);
	fail(lFilesize < 128);
	fail(aucHeader == NULL);

	/* Get the status flags from the header */
	ucDocStatus = ucGetByte(0x75, aucHeader);
	DBG_HEX(ucDocStatus);
	bFastSaved = (ucDocStatus & BIT(1)) != 0;
	DBG_MSG_C(bFastSaved, "This document is Fast Saved");
	ucVersion = ucGetByte(0x74, aucHeader);
	DBG_DEC(ucVersion);
	DBG_MSG_C(ucVersion == 0, "Written by Word 4.0 or earlier");
	DBG_MSG_C(ucVersion == 3, "Word 5.0 format, but not written by Word");
	DBG_MSG_C(ucVersion == 4, "Written by Word 5.x");
	if(bFastSaved) {
		werr(0, "Word for DOS: autosave documents are not supported");
		return FALSE;
	}

	/* Get length information */
	ulTextLen = ulGetLong(0x0e, aucHeader);
	DBG_HEX(ulTextLen);
	ulTextLen -= 128;
	DBG_DEC(ulTextLen);
	tTextBlock.ulFileOffset = 128;
	tTextBlock.ulCharPos = 128;
	tTextBlock.ulLength = ulTextLen;
	tTextBlock.bUsesUnicode = FALSE;
	tTextBlock.usPropMod = IGNORE_PROPMOD;
	if(!bAdd2TextBlockList(&tTextBlock)) {
		DBG_HEX(tTextBlock.ulFileOffset);
		DBG_HEX(tTextBlock.ulCharPos);
		DBG_DEC(tTextBlock.ulLength);
		DBG_DEC(tTextBlock.bUsesUnicode);
		DBG_DEC(tTextBlock.usPropMod);
		return FALSE;
	}
	return TRUE;
} /* end of bGetDocumentText */

/*
 * iInitDocumentDOS - initialize an DOS document
 *
 * Returns the version of Word that made the document or -1
 */
int
iInitDocumentDOS(FILE* pFile, int32_t lFilesize)
{
	int iWordVersion;
	BOOL bSuccess;
	USHORT usIdent;
	UCHAR aucHeader[128];

	fail(pFile == NULL);

	if(lFilesize < 128) {
		return -1;
	}

	/* Read the headerblock */
	if(!bReadBytes(aucHeader, 128, 0x00, pFile)) {
		return -1;
	}
	/* Get the "magic number" from the header */
	usIdent = usGetWord(0x00, aucHeader);
	DBG_HEX(usIdent);
	fail(usIdent != 0xbe31); /* Word for DOS */
	iWordVersion = iGetVersionNumber(aucHeader);
	if(iWordVersion != 0) {
		werr(0, "This file is not from 'Word for DOS'.");
		return -1;
	}
	bSuccess = bGetDocumentText(pFile, lFilesize, aucHeader);
	if(bSuccess) {
		vGetPropertyInfo(pFile, NULL, NULL, 0, NULL, 0, aucHeader,
		                 iWordVersion);
		vSetDefaultTabWidth(pFile, NULL, NULL, 0, NULL, 0, aucHeader,
		                    iWordVersion);
		vGetNotesInfo(pFile, NULL, NULL, 0, NULL, 0, aucHeader,
		              iWordVersion);
	}
	return bSuccess ? iWordVersion : -1;
} /* end of iInitDocumentDOS */
