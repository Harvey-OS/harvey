/*
 * notes.c
 * Copyright (C) 1998-2003 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to tell the difference between footnotes and endnotes
 */

#include "antiword.h"

/* Variables needed to write the Footnote and Endnote Lists */
static ULONG	*aulFootnoteList = NULL;
static size_t	tFootnoteListLength = 0;
static ULONG	*aulEndnoteList = NULL;
static size_t	tEndnoteListLength = 0;


/*
 * Destroy the lists with footnote and endnote information
 */
void
vDestroyNotesInfoLists(void)
{
	DBG_MSG("vDestroyNotesInfoLists");

	/* Free the lists and reset all control variables */
	aulEndnoteList = xfree(aulEndnoteList);
	aulFootnoteList = xfree(aulFootnoteList);
	tEndnoteListLength = 0;
	tFootnoteListLength = 0;
} /* end of vDestroyNotesInfoLists */

/*
 * Build the list with footnote information for Word 6/7 files
 */
static void
vGet6FootnotesInfo(FILE *pFile, ULONG ulStartBlock,
	const ULONG *aulBBD, size_t tBBDLen,
	const UCHAR *aucHeader)
{
	UCHAR	*aucBuffer;
	ULONG	ulFileOffset, ulBeginOfText, ulOffset, ulBeginFootnoteInfo;
	size_t	tFootnoteInfoLen;
	int	iIndex;

	fail(pFile == NULL || aucHeader == NULL);
	fail(ulStartBlock > MAX_BLOCKNUMBER && ulStartBlock != END_OF_CHAIN);
	fail(aulBBD == NULL);

	ulBeginOfText = ulGetLong(0x18, aucHeader);
	NO_DBG_HEX(ulBeginOfText);
	ulBeginFootnoteInfo = ulGetLong(0x68, aucHeader);
	NO_DBG_HEX(ulBeginFootnoteInfo);
	tFootnoteInfoLen = (size_t)ulGetLong(0x6c, aucHeader);
	NO_DBG_DEC(tFootnoteInfoLen);

	if (tFootnoteInfoLen < 10) {
		DBG_MSG("No Footnotes in this document");
		return;
	}

	aucBuffer = xmalloc(tFootnoteInfoLen);
	if (!bReadBuffer(pFile, ulStartBlock,
			aulBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, ulBeginFootnoteInfo, tFootnoteInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tFootnoteInfoLen);

	fail(tFootnoteListLength != 0);
	tFootnoteListLength = (tFootnoteInfoLen - 4) / 6;
	fail(tFootnoteListLength == 0);

	fail(aulFootnoteList != NULL);
	aulFootnoteList = xcalloc(tFootnoteListLength, sizeof(ULONG));

	for (iIndex = 0; iIndex < (int)tFootnoteListLength; iIndex++) {
		ulOffset = ulGetLong(iIndex * 4, aucBuffer);
		NO_DBG_HEX(ulOffset);
		ulFileOffset = ulCharPos2FileOffset(ulBeginOfText + ulOffset);
		NO_DBG_HEX(ulFileOffset);
		aulFootnoteList[iIndex] = ulFileOffset;
	}
	aucBuffer = xfree(aucBuffer);
} /* end of vGet6FootnotesInfo */

/*
 * Build the list with endnote information for Word 6/7 files
 */
static void
vGet6EndnotesInfo(FILE *pFile, ULONG ulStartBlock,
	const ULONG *aulBBD, size_t tBBDLen,
	const UCHAR *aucHeader)
{
	UCHAR	*aucBuffer;
	ULONG	ulFileOffset, ulBeginOfText, ulOffset, ulBeginEndnoteInfo;
	size_t	tEndnoteInfoLen;
	int	iIndex;

	fail(pFile == NULL || aucHeader == NULL);
	fail(ulStartBlock > MAX_BLOCKNUMBER && ulStartBlock != END_OF_CHAIN);
	fail(aulBBD == NULL);

	ulBeginOfText = ulGetLong(0x18, aucHeader);
	NO_DBG_HEX(ulBeginOfText);
	ulBeginEndnoteInfo = ulGetLong(0x1d2, aucHeader);
	NO_DBG_HEX(ulBeginEndnoteInfo);
	tEndnoteInfoLen = (size_t)ulGetLong(0x1d6, aucHeader);
	NO_DBG_DEC(tEndnoteInfoLen);

	if (tEndnoteInfoLen < 10) {
		DBG_MSG("No Endnotes in this document");
		return;
	}

	aucBuffer = xmalloc(tEndnoteInfoLen);
	if (!bReadBuffer(pFile, ulStartBlock,
			aulBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, ulBeginEndnoteInfo, tEndnoteInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tEndnoteInfoLen);

	fail(tEndnoteListLength != 0);
	tEndnoteListLength = (tEndnoteInfoLen - 4) / 6;
	fail(tEndnoteListLength == 0);

	fail(aulEndnoteList != NULL);
	aulEndnoteList = xcalloc(tEndnoteListLength, sizeof(ULONG));

	for (iIndex = 0; iIndex < (int)tEndnoteListLength; iIndex++) {
		ulOffset = ulGetLong(iIndex * 4, aucBuffer);
		NO_DBG_HEX(ulOffset);
		ulFileOffset = ulCharPos2FileOffset(ulBeginOfText + ulOffset);
		NO_DBG_HEX(ulFileOffset);
		aulEndnoteList[iIndex] = ulFileOffset;
	}
	aucBuffer = xfree(aucBuffer);
} /* end of vGet6EndnotesInfo */

/*
 * Build the lists note information for Word 6/7 files
 */
static void
vGet6NotesInfo(FILE *pFile, ULONG ulStartBlock,
	const ULONG *aulBBD, size_t tBBDLen,
	const UCHAR *aucHeader)
{
	vGet6FootnotesInfo(pFile, ulStartBlock,
			aulBBD, tBBDLen, aucHeader);
	vGet6EndnotesInfo(pFile, ulStartBlock,
			aulBBD, tBBDLen, aucHeader);
} /* end of vGet6NotesInfo */

/*
 * Build the list with footnote information for Word 8/9/10 files
 */
static void
vGet8FootnotesInfo(FILE *pFile, const pps_info_type *pPPS,
	const ULONG *aulBBD, size_t tBBDLen,
	const ULONG *aulSBD, size_t tSBDLen,
	const UCHAR *aucHeader)
{
	const ULONG	*aulBlockDepot;
	UCHAR	*aucBuffer;
	ULONG	ulFileOffset, ulBeginOfText, ulOffset, ulBeginFootnoteInfo;
	ULONG	ulTableSize, ulTableStartBlock;
	size_t	tFootnoteInfoLen, tBlockDepotLen, tBlockSize;
	int	iIndex;
	USHORT	usDocStatus;

	ulBeginOfText = ulGetLong(0x18, aucHeader);
	NO_DBG_HEX(ulBeginOfText);
	ulBeginFootnoteInfo = ulGetLong(0xaa, aucHeader);
	NO_DBG_HEX(ulBeginFootnoteInfo);
	tFootnoteInfoLen = (size_t)ulGetLong(0xae, aucHeader);
	NO_DBG_DEC(tFootnoteInfoLen);

	if (tFootnoteInfoLen < 10) {
		DBG_MSG("No Footnotes in this document");
		return;
	}

	/* Use 0Table or 1Table? */
	usDocStatus = usGetWord(0x0a, aucHeader);
	if (usDocStatus & BIT(9)) {
		ulTableStartBlock = pPPS->t1Table.ulSB;
		ulTableSize = pPPS->t1Table.ulSize;
	} else {
		ulTableStartBlock = pPPS->t0Table.ulSB;
		ulTableSize = pPPS->t0Table.ulSize;
	}
	NO_DBG_DEC(ulTableStartBlock);
	if (ulTableStartBlock == 0) {
		DBG_MSG("No notes information");
		return;
	}
	NO_DBG_HEX(ulTableSize);
	if (ulTableSize < MIN_SIZE_FOR_BBD_USE) {
	  	/* Use the Small Block Depot */
		aulBlockDepot = aulSBD;
		tBlockDepotLen = tSBDLen;
		tBlockSize = SMALL_BLOCK_SIZE;
	} else {
	  	/* Use the Big Block Depot */
		aulBlockDepot = aulBBD;
		tBlockDepotLen = tBBDLen;
		tBlockSize = BIG_BLOCK_SIZE;
	}
	aucBuffer = xmalloc(tFootnoteInfoLen);
	if (!bReadBuffer(pFile, ulTableStartBlock,
			aulBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, ulBeginFootnoteInfo, tFootnoteInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tFootnoteInfoLen);

	fail(tFootnoteListLength != 0);
	tFootnoteListLength = (tFootnoteInfoLen - 4) / 6;
	fail(tFootnoteListLength == 0);

	fail(aulFootnoteList != NULL);
	aulFootnoteList = xcalloc(tFootnoteListLength, sizeof(ULONG));

	for (iIndex = 0; iIndex < (int)tFootnoteListLength; iIndex++) {
		ulOffset = ulGetLong(iIndex * 4, aucBuffer);
		NO_DBG_HEX(ulOffset);
		ulFileOffset = ulCharPos2FileOffset(ulBeginOfText + ulOffset);
		NO_DBG_HEX(ulFileOffset);
		aulFootnoteList[iIndex] = ulFileOffset;
	}
	aucBuffer = xfree(aucBuffer);
} /* end of vGet8FootnotesInfo */

/*
 * Build the list with endnote information for Word 8/9/10 files
 */
static void
vGet8EndnotesInfo(FILE *pFile, const pps_info_type *pPPS,
	const ULONG *aulBBD, size_t tBBDLen,
	const ULONG *aulSBD, size_t tSBDLen,
	const UCHAR *aucHeader)
{
	const ULONG	*aulBlockDepot;
	UCHAR	*aucBuffer;
	ULONG	ulFileOffset, ulBeginOfText, ulOffset, ulBeginEndnoteInfo;
	ULONG	ulTableSize, ulTableStartBlock;
	size_t	tEndnoteInfoLen, tBlockDepotLen, tBlockSize;
	int	iIndex;
	USHORT	usDocStatus;

	ulBeginOfText = ulGetLong(0x18, aucHeader);
	NO_DBG_HEX(ulBeginOfText);
	ulBeginEndnoteInfo = ulGetLong(0x20a, aucHeader);
	NO_DBG_HEX(ulBeginEndnoteInfo);
	tEndnoteInfoLen = (size_t)ulGetLong(0x20e, aucHeader);
	NO_DBG_DEC(tEndnoteInfoLen);

	if (tEndnoteInfoLen < 10) {
		DBG_MSG("No Endnotes in this document");
		return;
	}

	/* Use 0Table or 1Table? */
	usDocStatus = usGetWord(0x0a, aucHeader);
	if (usDocStatus & BIT(9)) {
		ulTableStartBlock = pPPS->t1Table.ulSB;
		ulTableSize = pPPS->t1Table.ulSize;
	} else {
		ulTableStartBlock = pPPS->t0Table.ulSB;
		ulTableSize = pPPS->t0Table.ulSize;
	}
	NO_DBG_DEC(ulTableStartBlock);
	if (ulTableStartBlock == 0) {
		DBG_MSG("No notes information");
		return;
	}
	NO_DBG_HEX(ulTableSize);
	if (ulTableSize < MIN_SIZE_FOR_BBD_USE) {
	  	/* Use the Small Block Depot */
		aulBlockDepot = aulSBD;
		tBlockDepotLen = tSBDLen;
		tBlockSize = SMALL_BLOCK_SIZE;
	} else {
	  	/* Use the Big Block Depot */
		aulBlockDepot = aulBBD;
		tBlockDepotLen = tBBDLen;
		tBlockSize = BIG_BLOCK_SIZE;
	}
	aucBuffer = xmalloc(tEndnoteInfoLen);
	if (!bReadBuffer(pFile, ulTableStartBlock,
			aulBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, ulBeginEndnoteInfo, tEndnoteInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tEndnoteInfoLen);

	fail(tEndnoteListLength != 0);
	tEndnoteListLength = (tEndnoteInfoLen - 4) / 6;
	fail(tEndnoteListLength == 0);

	fail(aulEndnoteList != NULL);
	aulEndnoteList = xcalloc(tEndnoteListLength, sizeof(ULONG));

	for (iIndex = 0; iIndex < (int)tEndnoteListLength; iIndex++) {
		ulOffset = ulGetLong(iIndex * 4, aucBuffer);
		NO_DBG_HEX(ulOffset);
		ulFileOffset = ulCharPos2FileOffset(ulBeginOfText + ulOffset);
		NO_DBG_HEX(ulFileOffset);
		aulEndnoteList[iIndex] = ulFileOffset;
	}
	aucBuffer = xfree(aucBuffer);
} /* end of vGet8EndnotesInfo */

/*
 * Build the lists with footnote and endnote information for Word 8/9/10 files
 */
static void
vGet8NotesInfo(FILE *pFile, const pps_info_type *pPPS,
	const ULONG *aulBBD, size_t tBBDLen,
	const ULONG *aulSBD, size_t tSBDLen,
	const UCHAR *aucHeader)
{
	vGet8FootnotesInfo(pFile, pPPS,
			aulBBD, tBBDLen, aulSBD, tSBDLen, aucHeader);
	vGet8EndnotesInfo(pFile, pPPS,
			aulBBD, tBBDLen, aulSBD, tSBDLen, aucHeader);
} /* end of vGet8NotesInfo */

/*
 * Build the lists with footnote and endnote information
 */
void
vGetNotesInfo(FILE *pFile, const pps_info_type *pPPS,
	const ULONG *aulBBD, size_t tBBDLen,
	const ULONG *aulSBD, size_t tSBDLen,
	const UCHAR *aucHeader, int iWordVersion)
{
	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(iWordVersion < 6 || iWordVersion > 8);
	fail(aulBBD == NULL || aulSBD == NULL);

	switch (iWordVersion) {
	case 6:
	case 7:
		vGet6NotesInfo(pFile, pPPS->tWordDocument.ulSB,
			aulBBD, tBBDLen, aucHeader);
		break;
	case 8:
		vGet8NotesInfo(pFile, pPPS,
			aulBBD, tBBDLen, aulSBD, tSBDLen, aucHeader);
		break;
	default:
		werr(0, "Sorry, no notes information");
		break;
	}
} /* end of vGetNotesInfo */

/*
 * Get the notetype of the note at the given fileoffset
 */
notetype_enum
eGetNotetype(ULONG ulFileOffset)
{
	size_t	tIndex;

	fail(aulFootnoteList == NULL && tFootnoteListLength != 0);
	fail(aulEndnoteList == NULL && tEndnoteListLength != 0);

	/* Go for the easy answers first */
	if (tFootnoteListLength == 0 && tEndnoteListLength == 0) {
		return notetype_is_unknown;
	}
	if (tEndnoteListLength == 0) {
		return notetype_is_footnote;
	}
	if (tFootnoteListLength == 0) {
		return notetype_is_endnote;
	}
	/* No easy answer, so we search */
	for (tIndex = 0; tIndex < tFootnoteListLength; tIndex++) {
		if (aulFootnoteList[tIndex] == ulFileOffset) {
			return notetype_is_footnote;
		}
	}
	for (tIndex = 0; tIndex < tEndnoteListLength; tIndex++) {
		if (aulEndnoteList[tIndex] == ulFileOffset) {
			return notetype_is_endnote;
		}
	}
	/* Not found */
	return notetype_is_unknown;
} /* end of eGetNotetype */
