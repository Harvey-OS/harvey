/*
 * stylesheet.c
 * Copyright (C) 2001,2002 A.J. van Os; Released under GPL
 *
 * Description:
 * Build, read and destroy a list of stylesheet information
 *
 */

#include <string.h>
#include "antiword.h"


#define SGC_PAP		1
#define SGC_CHP		2

/* Variables needed to describe the stylesheet list */
static style_block_type	*atStyleInfo = NULL;
static font_block_type	*atFontInfo = NULL;
static BOOL		*abFilled = NULL;
static size_t		tStdCount = 0;


/*
 * vDestroyStylesheetList - destroy the stylesheet list
 */
void
vDestroyStylesheetList(void)
{
	DBG_MSG("vDestroyStylesheetList");

	tStdCount = 0;
	atStyleInfo = xfree(atStyleInfo);
	atFontInfo = xfree(atFontInfo);
	abFilled = xfree(abFilled);
} /* end of vDestroyStylesheetList */

/*
 * vGetDefaultStyle - fill the style struct with default values
 */
static void
vGetDefaultStyle(style_block_type *pStyle)
{
	(void)memset(pStyle, 0, sizeof(*pStyle));
	pStyle->usIstd = ISTD_INVALID;
	pStyle->usIstdNext = ISTD_INVALID;
	pStyle->usStartAt = 1;
	pStyle->ucListLevel = 9;
} /* end of vGetDefaultStyle */

/*
 * vGetDefaultFont - fill the font struct with default values
 */
static void
vGetDefaultFont(font_block_type *pFont)
{
	(void)memset(pFont, 0, sizeof(*pFont));
	pFont->usFontSize = DEFAULT_FONT_SIZE;
} /* end of vGetDefaultFont */

/*
 * iGetStyleIndex - get the index of the record with the specified istd
 *
 * returns the index when found, otherwise -1
 */
static int
iGetStyleIndex(USHORT usIstd)
{
	int	iIndex;

	fail(abFilled == NULL);

	if (usIstd == ISTD_INVALID || abFilled == NULL) {
		return -1;
	}

	for (iIndex = 0; iIndex < (int)tStdCount; iIndex++) {
		if (abFilled[iIndex] && atStyleInfo[iIndex].usIstd == usIstd) {
			/* The record is filled and the istd matches */
			return iIndex;
		}
	}
	return -1;
} /* end of iGetStyleIndex */

/*
 * Convert a stylecode (stc) as used by WinWord 2 into a styleindex (istd)
 * as used by Word 6 and up
 */
USHORT
usStc2istd(UCHAR ucStc)
{
	/* Old nil style to new nil style */
	if (ucStc == 222) {
		return STI_NIL;
	}
	/* Heading 1 through 9 must become istd 1 through 9 */
	/* so 254 through 246 must become 1 through 9 and vice versa */
	if ((ucStc >= 1 && ucStc <= 9) ||
	    (ucStc >= 246 && ucStc <= 254)) {
		return 255 - (USHORT)ucStc;
	}
	return (USHORT)ucStc;
} /* end of usStd2istd */

/*
 * Get a build-in style for Winword 1/2
 */
static void
vGetBuildinStyle(UCHAR ucStc, style_block_type *pStyle)
{
	fail(pStyle == NULL);

	/* Start with de defaults */
	vGetDefaultStyle(pStyle);

	/* Add the build-in style */
	switch (ucStc) {
	case 246:
	case 247:
	case 248:
	case 249:
	case 250:
	case 255:
		pStyle->sLeftIndent = 720;
		break;
	case 251:
	case 252:
		pStyle->sLeftIndent = 360;
		break;
	case 253:
		pStyle->usBeforeIndent = 120;
		break;
	case 254:
		pStyle->usBeforeIndent = 240;
		break;
	default:
		if (ucStc >= 233 && ucStc <= 239) {
			pStyle->sLeftIndent = (239 - (short)ucStc) * 360;
		}
		if (ucStc >= 225 && ucStc <= 232) {
			pStyle->sLeftIndent = (232 - (short)ucStc) * 720;
			pStyle->sRightIndent = 720;
		}
		break;
	}
} /* end of vGetBuildinStyle */

/*
 * Get a build-in fontstyle for Winword 1/2
 */
static void
vGetBuildinFont(UCHAR ucStc, font_block_type *pFont)
{
	fail(pFont == NULL);

	/* Start with de defaults */
	vGetDefaultFont(pFont);

	/* Add the build-in fontstyle */
	switch (ucStc) {
	case 223:
	case 244:
		pFont->usFontSize = 16;
		break;
	case 246:
	case 247:
	case 248:
		pFont->usFontStyle |= FONT_ITALIC;
		break;
	case 249:
		pFont->usFontStyle |= FONT_UNDERLINE;
		break;
	case 250:
		pFont->usFontStyle |= FONT_BOLD;
		break;
	case 251:
		pFont->usFontStyle |= FONT_UNDERLINE;
		pFont->usFontSize = 24;
		break;
	case 252:
		pFont->usFontStyle |= FONT_BOLD;
		pFont->usFontSize = 24;
		break;
	case 253:
		pFont->ucFontNumber = 2;
		pFont->usFontStyle |= FONT_BOLD;
		pFont->usFontSize = 24;
		break;
	case 254:
		pFont->ucFontNumber = 2;
		pFont->usFontStyle |= (FONT_BOLD|FONT_UNDERLINE);
		pFont->usFontSize = 24;
		break;
	default:
		break;
	}
} /* end of vGetBuildinFont */

/*
 * Build the lists with Stylesheet Information for WinWord 1/2 files
 */
void
vGet2Stylesheet(FILE *pFile, int iWordVersion, const UCHAR *aucHeader)
{
	style_block_type	*pStyle;
	font_block_type		*pFont;
	UCHAR	*aucBuffer;
	ULONG	ulBeginStshInfo;
	size_t	tStshInfoLen, tName, tChpx, tPapx;
	int	iStIndex, iChpxIndex, iPapxIndex, iMaxIndex, iSt, iChpx, iPapx;
	int	iStd, iIndex, iBaseStyleIndex, iCounter;
	USHORT	usBaseStyle;
	UCHAR	ucStc, ucStcNext, ucStcBase;

	fail(pFile == NULL || aucHeader == NULL);
	fail(iWordVersion != 1 && iWordVersion != 2);

	ulBeginStshInfo = ulGetLong(0x5e, aucHeader); /* fcStshf */
	NO_DBG_HEX(ulBeginStshInfo);
	tStshInfoLen = (size_t)usGetWord(0x62, aucHeader); /* cbStshf */
	NO_DBG_DEC(tStshInfoLen);

	aucBuffer = xmalloc(tStshInfoLen);
	if (!bReadBytes(aucBuffer, tStshInfoLen, ulBeginStshInfo, pFile)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tStshInfoLen);

	iStd = (int)usGetWord(0, aucBuffer);
	tName = (size_t)usGetWord(2, aucBuffer);
	tChpx = (size_t)usGetWord(2 + tName, aucBuffer);
	tPapx = (size_t)usGetWord(2 + tName + tChpx, aucBuffer);
	tStdCount = (size_t)usGetWord(2 + tName + tChpx + tPapx, aucBuffer);

	NO_DBG_HEX(tStdCount);
	fail(2 + tName + tChpx + tPapx + 2 + 2 * tStdCount > tStshInfoLen);

	atStyleInfo = xcalloc(tStdCount, sizeof(style_block_type));
	atFontInfo = xcalloc(tStdCount, sizeof(font_block_type));
	abFilled = xcalloc(tStdCount, sizeof(BOOL));

	do {
		iCounter = 0;
		iStIndex = 2 + 2;
		iChpxIndex = 2 + (int)tName + 2;
		iPapxIndex = 2 + (int)tName + (int)tChpx + 2;
		iMaxIndex = 2 + (int)tName + (int)tChpx + (int)tPapx + 2;
		/* Read the styles one-by-one */
		for (iIndex = 0; iIndex < (int)tStdCount; iIndex++) {
			pStyle = &atStyleInfo[iIndex];
			pFont = &atFontInfo[iIndex];
			iSt = (int)ucGetByte(iStIndex, aucBuffer);
			iChpx = (int)ucGetByte(iChpxIndex, aucBuffer);
			iPapx = (int)ucGetByte(iPapxIndex, aucBuffer);
			NO_DBG_HEX(iSt);
			NO_DBG_HEX(iChpx);
			NO_DBG_HEX(iPapx);
			if (iSt == 0xff) {
				/* Undefined style */
				iStIndex++;
				iChpxIndex++;
				iPapxIndex++;
				iMaxIndex += 2;
				if (!abFilled[iIndex]) {
					DBG_HEX_C(iChpx != 0xff, iChpx);
					DBG_HEX_C(iPapx != 0xff, iPapx);
					vGetDefaultStyle(pStyle);
					vGetDefaultFont(pFont);
					abFilled[iIndex] = TRUE;
				}
				continue;
			}

			NO_DBG_STRN(aucBuffer + iStIndex + 1, iSt);
			iStIndex += iSt + 1;

			ucStcNext = ucGetByte(iMaxIndex, aucBuffer);
			ucStcBase = ucGetByte(iMaxIndex + 1, aucBuffer);
			ucStc = (UCHAR)((iIndex - iStd) & 0xff);
			NO_DBG_DEC(ucStc);

			if (iChpx == 0xff || iPapx == 0xff) {
				/* Use a build-in style */
				iChpxIndex++;
				iPapxIndex++;
				iMaxIndex += 2;
				if (!abFilled[iIndex]) {
					DBG_HEX_C(iChpx != 0xff, iChpx);
					DBG_HEX_C(iPapx != 0xff, iPapx);
					vGetBuildinStyle(ucStc, pStyle);
					pStyle->usIstd = usStc2istd(ucStc);
					pStyle->usIstdNext =
							usStc2istd(ucStcNext);
					vGetBuildinFont(ucStc, pFont);
					abFilled[iIndex] = TRUE;
				}
				continue;
			}

			if (abFilled[iIndex]) {
				/* This record has already been filled */
				iChpxIndex += iChpx + 1;
				iPapxIndex += iPapx + 1;
				iMaxIndex += 2;
				continue;
			}

			usBaseStyle = usStc2istd(ucStcBase);

			if (usBaseStyle == STI_NIL) {
				/* Based on the Nil style */
				vGetDefaultStyle(pStyle);
				vGetDefaultFont(pFont);
			} else {
				iBaseStyleIndex = iGetStyleIndex(usBaseStyle);
				NO_DBG_DEC(iBaseStyleIndex);
				if (iBaseStyleIndex < 0) {
					/* This style is not known yet */
					iChpxIndex += iChpx + 1;
					iPapxIndex += iPapx + 1;
					iMaxIndex += 2;
					continue;
				}
				fail(iBaseStyleIndex >= (int)tStdCount);
				fail(!abFilled[iBaseStyleIndex]);
				/* Based on the specified base style */
				*pStyle = atStyleInfo[iBaseStyleIndex];
				*pFont = atFontInfo[iBaseStyleIndex];
			}
			pStyle->usIstd = usStc2istd(ucStc);
			pStyle->usIstdNext = usStc2istd(ucStcNext);

			abFilled[iIndex] = TRUE;
			iCounter++;

			/* Add the changes if any */
			switch (iChpx) {
			case 0x00:
			case 0xff:
				iChpxIndex++;
				break;
			default:
				NO_DBG_PRINT_BLOCK(aucBuffer + iChpxIndex + 1,
						iChpx);
				if (iWordVersion == 1) {
					vGet1FontInfo(0,
						aucBuffer + iChpxIndex + 1,
						(size_t)iChpx, pFont);
				} else {
					vGet2FontInfo(0,
						aucBuffer + iChpxIndex + 1,
						(size_t)iChpx, pFont);
				}
				iChpxIndex += iChpx + 1;
				break;
			}

			switch (iPapx) {
			case 0x00:
			case 0xff:
				iPapxIndex++;
				break;
			default:
				NO_DBG_PRINT_BLOCK(aucBuffer + iPapxIndex + 8,
						iPapx - 7);
				vGet2StyleInfo(0, aucBuffer + iPapxIndex + 8,
						iPapx - 7, pStyle);
				iPapxIndex += iPapx + 1;
				break;
			}
			iMaxIndex += 2;

		}
		NO_DBG_DEC(iCounter);
	} while (iCounter > 0);

	for (iIndex = 0; iIndex < (int)tStdCount; iIndex++) {
		if (!abFilled[iIndex]) {
			NO_DBG_DEC(iIndex);
			vGetDefaultStyle(&atStyleInfo[iIndex]);
			vGetDefaultFont(&atFontInfo[iIndex]);
		}
	}

	abFilled = xfree(abFilled);
	aucBuffer = xfree(aucBuffer);
} /* end of vGet2Stylesheet */

/*
 * Build the lists with Stylesheet Information for Word 6/7 files
 */
void
vGet6Stylesheet(FILE *pFile, ULONG ulStartBlock,
	const ULONG *aulBBD, size_t tBBDLen, const UCHAR *aucHeader)
{
	style_block_type	*pStyle;
	font_block_type		*pFont;
	UCHAR	*aucBuffer;
	ULONG	ulBeginStshInfo;
	size_t	tStshInfoLen, tOffset, tStdLen, tStdBaseInFile;
	size_t	tPos, tNameLen, tUpxLen;
	int	iIndex, iBaseStyleIndex, iCounter;
	USHORT	usTmp, usUpxCount, usStyleType, usBaseStyle;

	fail(pFile == NULL || aucHeader == NULL);
	fail(ulStartBlock > MAX_BLOCKNUMBER && ulStartBlock != END_OF_CHAIN);
	fail(aulBBD == NULL);

	ulBeginStshInfo = ulGetLong(0x60, aucHeader); /* fcStshf */
	NO_DBG_HEX(ulBeginStshInfo);
	tStshInfoLen = (size_t)ulGetLong(0x64, aucHeader); /* lcbStshf */
	NO_DBG_DEC(tStshInfoLen);

	aucBuffer = xmalloc(tStshInfoLen);
	if (!bReadBuffer(pFile, ulStartBlock,
			aulBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, ulBeginStshInfo, tStshInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tStshInfoLen);

	tStdCount = (size_t)usGetWord(2, aucBuffer);
	NO_DBG_DEC(tStdCount);
	tStdBaseInFile = (size_t)usGetWord(4, aucBuffer);

	atStyleInfo = xcalloc(tStdCount, sizeof(style_block_type));
	atFontInfo = xcalloc(tStdCount, sizeof(font_block_type));
	abFilled = xcalloc(tStdCount, sizeof(BOOL));

	do {
		iCounter = 0;
		/* Read the styles one-by-one */
		for (iIndex = 0, tOffset = 2 + (size_t)usGetWord(0, aucBuffer);
		     iIndex < (int)tStdCount;
		     iIndex++, tOffset += 2 + tStdLen) {
			NO_DBG_DEC(tOffset);
			tStdLen = (size_t)usGetWord(tOffset, aucBuffer);
			NO_DBG_DEC(tStdLen);
			if (abFilled[iIndex]) {
				/* This record has already been filled */
				continue;
			}
			pStyle = &atStyleInfo[iIndex];
			pFont = &atFontInfo[iIndex];
			if (tStdLen == 0) {
				/* Empty record */
				vGetDefaultStyle(pStyle);
				vGetDefaultFont(pFont);
				abFilled[iIndex] = TRUE;
				continue;
			}
			usTmp = usGetWord(tOffset + 4, aucBuffer);
			usStyleType = usTmp % 16;
			usBaseStyle = usTmp / 16;
			NO_DBG_DEC(usStyleType);
			NO_DBG_DEC(usBaseStyle);
			if (usBaseStyle == STI_NIL || usBaseStyle == STI_USER) {
				/* Based on the Nil style */
				vGetDefaultStyle(pStyle);
				vGetDefaultFont(pFont);
			} else {
				iBaseStyleIndex = iGetStyleIndex(usBaseStyle);
				NO_DBG_DEC(iBaseStyleIndex);
				if (iBaseStyleIndex < 0) {
					/* This base style is not known yet */
					continue;
				}
				fail(iBaseStyleIndex >= (int)tStdCount);
				fail(!abFilled[iBaseStyleIndex]);
				/* Based on the specified base style */
				*pStyle = atStyleInfo[iBaseStyleIndex];
				pStyle->usIstd = ISTD_INVALID;
				*pFont = atFontInfo[iBaseStyleIndex];
			}
			abFilled[iIndex] = TRUE;
			iCounter++;
			/* STD */
			usTmp = usGetWord(tOffset + 6, aucBuffer);
			usUpxCount = usTmp % 16;
			pStyle->usIstdNext = usTmp / 16;;
			NO_DBG_DEC(usUpxCount);
			tPos = 2 + tStdBaseInFile;
			NO_DBG_DEC(tPos);
			tNameLen = (size_t)ucGetByte(tOffset + tPos, aucBuffer);
			NO_DBG_DEC(tNameLen);
			NO_DBG_STRN(aucBuffer + tOffset + tPos + 1, tNameLen);
			tNameLen++;	/* Include the ASCII NULL character */
			tPos += 1 + tNameLen;
			if (odd(tPos)) {
				tPos++;
			}
			NO_DBG_DEC(tPos);
			if (tPos >= tStdLen) {
				continue;
			}
			tUpxLen = (size_t)usGetWord(tOffset + tPos, aucBuffer);
			NO_DBG_DEC(tUpxLen);
			if (tPos + tUpxLen > tStdLen) {
				/* UPX length too large to be a record */
				DBG_DEC_C(tPos + tUpxLen > tStdLen,
						tPos + tUpxLen);
				continue;
			}
			if (usStyleType == SGC_PAP && usUpxCount >= 1) {
				if (tUpxLen >= 2) {
					NO_DBG_PRINT_BLOCK(
						aucBuffer + tOffset + tPos + 2,
						tUpxLen);
					pStyle->usIstd = usGetWord(
						tOffset + tPos + 2, aucBuffer);
					NO_DBG_DEC(pStyle->usIstd);
					NO_DBG_DEC(pStyle->usIstdNext);
					vGet6StyleInfo(0,
						aucBuffer + tOffset + tPos + 4,
						tUpxLen - 2, pStyle);
					NO_DBG_DEC(pStyle->sLeftIndent);
					NO_DBG_DEC(pStyle->sRightIndent);
					NO_DBG_HEX(pStyle->ucAlignment);
				}
				tPos += 2 + tUpxLen;
				if (odd(tPos)) {
					tPos++;
				}
				NO_DBG_DEC(tPos);
				tUpxLen = (size_t)usGetWord(
						tOffset + tPos, aucBuffer);
				NO_DBG_DEC(tUpxLen);
			}
			if (tUpxLen == 0 || tPos + tUpxLen > tStdLen) {
				/* Too small or too large to be a record */
				DBG_DEC_C(tPos + tUpxLen > tStdLen,
							tPos + tUpxLen);
				continue;
			}
			if ((usStyleType == SGC_PAP && usUpxCount >= 2) ||
			    (usStyleType == SGC_CHP && usUpxCount >= 1)) {
				NO_DBG_PRINT_BLOCK(
						aucBuffer + tOffset + tPos + 2,
						tUpxLen);
				vGet6FontInfo(0, ISTD_INVALID,
						aucBuffer + tOffset + tPos + 2,
						(int)tUpxLen, pFont);
				NO_DBG_DEC(pFont->usFontSize);
				NO_DBG_DEC(pFont->ucFontcolor);
				NO_DBG_HEX(pFont->usFontStyle);
			}
		}
		NO_DBG_DEC(iCounter);
	} while (iCounter > 0);

	for (iIndex = 0; iIndex < (int)tStdCount; iIndex++) {
		if (!abFilled[iIndex]) {
			NO_DBG_DEC(iIndex);
			vGetDefaultStyle(&atStyleInfo[iIndex]);
			vGetDefaultFont(&atFontInfo[iIndex]);
		}
	}

	abFilled = xfree(abFilled);
	aucBuffer = xfree(aucBuffer);
} /* end of vGet6Stylesheet */

/*
 * Build the lists with Stylesheet Information for Word 8/9/10 files
 */
void
vGet8Stylesheet(FILE *pFile, const pps_info_type *pPPS,
	const ULONG *aulBBD, size_t tBBDLen,
	const ULONG *aulSBD, size_t tSBDLen,
	const UCHAR *aucHeader)
{
	style_block_type	*pStyle;
	font_block_type		*pFont;
	const ULONG	*aulBlockDepot;
	UCHAR	*aucBuffer;
	ULONG	ulBeginStshInfo;
	ULONG	ulTableSize, ulTableStartBlock;
	size_t	tStshInfoLen, tBlockDepotLen, tOffset, tStdLen, tStdBaseInFile;
	size_t	tBlockSize, tPos, tNameLen, tUpxLen;
	int	iIndex, iBaseStyleIndex, iCounter;
	USHORT	usDocStatus, usTmp, usUpxCount, usStyleType, usBaseStyle;

	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(aulBBD == NULL || aulSBD == NULL);

	ulBeginStshInfo = ulGetLong(0xa2, aucHeader); /* fcStshf */
	NO_DBG_HEX(ulBeginStshInfo);
	tStshInfoLen = (size_t)ulGetLong(0xa6, aucHeader); /* lcbStshf */
	NO_DBG_DEC(tStshInfoLen);

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
		DBG_MSG("No stylesheet information");
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
	aucBuffer = xmalloc(tStshInfoLen);
	if (!bReadBuffer(pFile, ulTableStartBlock,
			aulBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, ulBeginStshInfo, tStshInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tStshInfoLen);

	tStdCount = (size_t)usGetWord(2, aucBuffer);
	NO_DBG_DEC(tStdCount);
	tStdBaseInFile = (size_t)usGetWord(4, aucBuffer);

	atStyleInfo = xcalloc(tStdCount, sizeof(style_block_type));
	atFontInfo = xcalloc(tStdCount, sizeof(font_block_type));
	abFilled = xcalloc(tStdCount, sizeof(BOOL));

	do {
		iCounter = 0;
		/* Read the styles one-by-one */
		for (iIndex = 0, tOffset = 2 + (size_t)usGetWord(0, aucBuffer);
		     iIndex < (int)tStdCount;
		     iIndex++, tOffset += 2 + tStdLen) {
			NO_DBG_DEC(tOffset);
			tStdLen = (size_t)usGetWord(tOffset, aucBuffer);
			NO_DBG_DEC(tStdLen);
			if (abFilled[iIndex]) {
				/* This record has already been filled */
				continue;
			}
			pStyle = &atStyleInfo[iIndex];
			pFont = &atFontInfo[iIndex];
			if (tStdLen == 0) {
				/* Empty record */
				vGetDefaultStyle(pStyle);
				vGetDefaultFont(pFont);
				abFilled[iIndex] = TRUE;
				continue;
			}
			usTmp = usGetWord(tOffset + 4, aucBuffer);
			usStyleType = usTmp % 16;
			usBaseStyle = usTmp / 16;
			NO_DBG_DEC(usStyleType);
			NO_DBG_DEC(usBaseStyle);
			if (usBaseStyle == STI_NIL || usBaseStyle == STI_USER) {
				/* Based on the Nil style */
				vGetDefaultStyle(pStyle);
				vGetDefaultFont(pFont);
			} else {
				iBaseStyleIndex = iGetStyleIndex(usBaseStyle);
				NO_DBG_DEC(iBaseStyleIndex);
				if (iBaseStyleIndex < 0) {
					/* This base style is not known yet */
					continue;
				}
				fail(iBaseStyleIndex >= (int)tStdCount);
				fail(!abFilled[iBaseStyleIndex]);
				/* Based on the specified base style */
				*pStyle = atStyleInfo[iBaseStyleIndex];
				pStyle->usIstd = ISTD_INVALID;
				*pFont = atFontInfo[iBaseStyleIndex];
			}
			abFilled[iIndex] = TRUE;
			iCounter++;
			/* STD */
			usTmp = usGetWord(tOffset + 6, aucBuffer);
			usUpxCount = usTmp % 16;
			pStyle->usIstdNext = usTmp / 16;
			NO_DBG_DEC(usUpxCount);
			tPos = 2 + tStdBaseInFile;
			NO_DBG_DEC(tPos);
			tNameLen = (size_t)usGetWord(tOffset + tPos, aucBuffer);
			NO_DBG_DEC(tNameLen);
			tNameLen *= 2;	/* From Unicode characters to bytes */
			NO_DBG_UNICODE_N(aucBuffer + tOffset + tPos + 2,
					tNameLen);
			tNameLen += 2;	/* Include the Unicode NULL character */
			tPos += 2 + tNameLen;
			if (odd(tPos)) {
				tPos++;
			}
			NO_DBG_DEC(tPos);
			if (tPos >= tStdLen) {
				continue;
			}
			tUpxLen = (size_t)usGetWord(tOffset + tPos, aucBuffer);
			NO_DBG_DEC(tUpxLen);
			if (tPos + tUpxLen > tStdLen) {
				/* UPX length too large to be a record */
				DBG_DEC_C(tPos + tUpxLen > tStdLen,
						tPos + tUpxLen);
				continue;
			}
			if (usStyleType == SGC_PAP && usUpxCount >= 1) {
				if (tUpxLen >= 2) {
					NO_DBG_PRINT_BLOCK(
						aucBuffer + tOffset + tPos + 2,
						tUpxLen);
					pStyle->usIstd = usGetWord(
						tOffset + tPos + 2, aucBuffer);
					NO_DBG_DEC(pStyle->usIstd);
					NO_DBG_DEC(pStyle->usIstdNext);
					vGet8StyleInfo(0,
						aucBuffer + tOffset + tPos + 4,
						tUpxLen - 2, pStyle);
					NO_DBG_DEC(pStyle->sLeftIndent);
					NO_DBG_DEC(pStyle->sRightIndent);
					NO_DBG_HEX(pStyle->ucAlignment);
				}
				tPos += 2 + tUpxLen;
				if (odd(tPos)) {
					tPos++;
				}
				NO_DBG_DEC(tPos);
				tUpxLen = (size_t)usGetWord(
						tOffset + tPos, aucBuffer);
				NO_DBG_DEC(tUpxLen);
			}
			if (tUpxLen == 0 || tPos + tUpxLen > tStdLen) {
				/* Too small or too large to be a record */
				DBG_DEC_C(tPos + tUpxLen > tStdLen,
							tPos + tUpxLen);
				continue;
			}
			if ((usStyleType == SGC_PAP && usUpxCount >= 2) ||
			    (usStyleType == SGC_CHP && usUpxCount >= 1)) {
				NO_DBG_PRINT_BLOCK(
						aucBuffer + tOffset + tPos + 2,
						tUpxLen);
				vGet8FontInfo(0, ISTD_INVALID,
						aucBuffer + tOffset + tPos + 2,
						(int)tUpxLen, pFont);
				NO_DBG_DEC(pFont->usFontSize);
				NO_DBG_DEC(pFont->ucFontcolor);
				NO_DBG_HEX(pFont->usFontStyle);
			}
		}
		NO_DBG_DEC(iCounter);
	} while (iCounter > 0);

	for (iIndex = 0; iIndex < (int)tStdCount; iIndex++) {
		if (!abFilled[iIndex]) {
			NO_DBG_DEC(iIndex);
			vGetDefaultStyle(&atStyleInfo[iIndex]);
			vGetDefaultFont(&atFontInfo[iIndex]);
		}
	}

	abFilled = xfree(abFilled);
	aucBuffer = xfree(aucBuffer);
} /* end of vGet8Stylesheet */

/*
 * vFillStyleFromStylesheet - fill a style struct with stylesheet info
 */
void
vFillStyleFromStylesheet(USHORT usIstd, style_block_type *pStyle)
{
	int	iIndex;

	fail(pStyle == NULL);

	if (usIstd != ISTD_INVALID && usIstd != STI_NIL && usIstd != STI_USER) {
		for (iIndex = 0; iIndex < (int)tStdCount; iIndex++) {
			if (atStyleInfo[iIndex].usIstd == usIstd) {
				/* Right index found; return style */
				*pStyle = atStyleInfo[iIndex];
				return;
			}
		}
	}

	vGetDefaultStyle(pStyle);
	pStyle->usIstd = usIstd;
} /* end of vFillStyleFromStylesheet */

/*
 * vFillFontFromStylesheet - fill a font struct with stylesheet info
 */
void
vFillFontFromStylesheet(USHORT usIstd, font_block_type *pFont)
{
	int	iIndex;

	fail(pFont == NULL);

	if (usIstd != ISTD_INVALID && usIstd != STI_NIL && usIstd != STI_USER) {
		for (iIndex = 0; iIndex < (int)tStdCount; iIndex++) {
			if (atStyleInfo[iIndex].usIstd == usIstd) {
				/* Right index found; return font */
				*pFont = atFontInfo[iIndex];
				return;
			}
		}
	}

	vGetDefaultFont(pFont);
} /* end of vFillFontFromStylesheet */
