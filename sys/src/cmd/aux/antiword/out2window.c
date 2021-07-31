/*
 * out2window.c
 * Copyright (C) 1998-2003 A.J. van Os; Released under GPL
 *
 * Description:
 * Output to a text window
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "antiword.h"


/* Used for numbering the chapters */
static unsigned int	auiHdrCounter[9];


/*
 * vString2Diagram - put a string into a diagram
 */
static void
vString2Diagram(diagram_type *pDiag, output_type *pAnchor)
{
	output_type	*pOutput;
	long		lWidth;
	USHORT		usMaxFontSize;

	fail(pDiag == NULL);
	fail(pAnchor == NULL);

	/* Compute the maximum fontsize in this string */
	usMaxFontSize = MIN_FONT_SIZE;
	for (pOutput = pAnchor; pOutput != NULL; pOutput = pOutput->pNext) {
		if (pOutput->usFontSize > usMaxFontSize) {
			usMaxFontSize = pOutput->usFontSize;
		}
	}

	/* Goto the next line */
	vMove2NextLine(pDiag, pAnchor->tFontRef, usMaxFontSize);

	/* Output all substrings */
	for (pOutput = pAnchor; pOutput != NULL; pOutput = pOutput->pNext) {
		lWidth = lMilliPoints2DrawUnits(pOutput->lStringWidth);
		vSubstring2Diagram(pDiag, pOutput->szStorage,
			pOutput->tNextFree, lWidth, pOutput->ucFontColor,
			pOutput->usFontStyle, pOutput->tFontRef,
			pOutput->usFontSize, usMaxFontSize);
	}

	/* Goto the start of the line */
	pDiag->lXleft = 0;
} /* end of vString2Diagram */

/*
 * vSetLeftIndentation - set the left indentation of the specified diagram
 */
void
vSetLeftIndentation(diagram_type *pDiag, long lLeftIndentation)
{
	long	lX;

	fail(pDiag == NULL);
	fail(lLeftIndentation < 0);

	lX = lMilliPoints2DrawUnits(lLeftIndentation);
	if (lX > 0) {
		pDiag->lXleft = lX;
	} else {
		pDiag->lXleft = 0;
	}
} /* end of vSetLeftIndentation */

/*
 * lComputeNetWidth - compute the net string width
 */
static long
lComputeNetWidth(output_type *pAnchor)
{
	output_type	*pTmp;
	long		lNetWidth;

	fail(pAnchor == NULL);

	/* Step 1: Count all but the last sub-string */
	lNetWidth = 0;
	for (pTmp = pAnchor; pTmp->pNext != NULL; pTmp = pTmp->pNext) {
		fail(pTmp->lStringWidth < 0);
		lNetWidth += pTmp->lStringWidth;
	}
	fail(pTmp == NULL);
	fail(pTmp->pNext != NULL);

	/* Step 2: remove the white-space from the end of the string */
	while (pTmp->tNextFree != 0 &&
	       isspace((int)(UCHAR)pTmp->szStorage[pTmp->tNextFree - 1])) {
		pTmp->szStorage[pTmp->tNextFree - 1] = '\0';
		pTmp->tNextFree--;
		NO_DBG_DEC(pTmp->lStringWidth);
		pTmp->lStringWidth = lComputeStringWidth(
						pTmp->szStorage,
						pTmp->tNextFree,
						pTmp->tFontRef,
						pTmp->usFontSize);
		NO_DBG_DEC(pTmp->lStringWidth);
	}

	/* Step 3: Count the last sub-string */
	lNetWidth += pTmp->lStringWidth;
	return lNetWidth;
} /* end of lComputeNetWidth */

/*
 * iComputeHoles - compute number of holes
 * (A hole is a number of whitespace characters followed by a
 *  non-whitespace character)
 */
static int
iComputeHoles(output_type *pAnchor)
{
	output_type	*pTmp;
	size_t	tIndex;
	int	iCounter;
	BOOL	bWasSpace, bIsSpace;

	fail(pAnchor == NULL);

	iCounter = 0;
	bIsSpace = FALSE;
	/* Count the holes */
	for (pTmp = pAnchor; pTmp != NULL; pTmp = pTmp->pNext) {
		fail(pTmp->tNextFree != strlen(pTmp->szStorage));
		for (tIndex = 0; tIndex <= pTmp->tNextFree; tIndex++) {
			bWasSpace = bIsSpace;
			bIsSpace = isspace((int)(UCHAR)pTmp->szStorage[tIndex]);
			if (bWasSpace && !bIsSpace) {
				iCounter++;
			}
		}
	}
	return iCounter;
} /* end of iComputeHoles */

/*
 * Align a string and insert it into the text
 */
void
vAlign2Window(diagram_type *pDiag, output_type *pAnchor,
	long lScreenWidth, UCHAR ucAlignment)
{
	long	lNetWidth, lLeftIndentation;

	fail(pDiag == NULL || pAnchor == NULL);
	fail(lScreenWidth < lChar2MilliPoints(MIN_SCREEN_WIDTH));

	NO_DBG_MSG("vAlign2Window");

	lNetWidth = lComputeNetWidth(pAnchor);

	if (lScreenWidth > lChar2MilliPoints(MAX_SCREEN_WIDTH) ||
	    lNetWidth <= 0) {
		/*
		 * Screenwidth is "infinite", so no alignment is possible
		 * Don't bother to align an empty line
		 */
		vString2Diagram(pDiag, pAnchor);
		return;
	}

	switch (ucAlignment) {
	case ALIGNMENT_CENTER:
		lLeftIndentation = (lScreenWidth - lNetWidth) / 2;
		DBG_DEC_C(lLeftIndentation < 0, lLeftIndentation);
		if (lLeftIndentation > 0) {
			vSetLeftIndentation(pDiag, lLeftIndentation);
		}
		break;
	case ALIGNMENT_RIGHT:
		lLeftIndentation = lScreenWidth - lNetWidth;
		DBG_DEC_C(lLeftIndentation < 0, lLeftIndentation);
		if (lLeftIndentation > 0) {
			vSetLeftIndentation(pDiag, lLeftIndentation);
		}
		break;
	case ALIGNMENT_JUSTIFY:
	case ALIGNMENT_LEFT:
	default:
		break;
	}
	vString2Diagram(pDiag, pAnchor);
} /* end of vAlign2Window */

/*
 * vJustify2Window
 */
void
vJustify2Window(diagram_type *pDiag, output_type *pAnchor,
	long lScreenWidth, long lRightIndentation, UCHAR ucAlignment)
{
	output_type	*pTmp;
	char	*pcNew, *pcOld, *szStorage;
	long	lNetWidth, lSpaceWidth, lToAdd;
	int	iFillerLen, iHoles;

	fail(pDiag == NULL || pAnchor == NULL);
	fail(lScreenWidth < MIN_SCREEN_WIDTH);
	fail(lRightIndentation > 0);

	NO_DBG_MSG("vJustify2Window");

	if (ucAlignment != ALIGNMENT_JUSTIFY) {
		vAlign2Window(pDiag, pAnchor, lScreenWidth, ucAlignment);
		return;
	}

	lNetWidth = lComputeNetWidth(pAnchor);

	if (lScreenWidth > lChar2MilliPoints(MAX_SCREEN_WIDTH) ||
	    lNetWidth <= 0) {
		/*
		 * Screenwidth is "infinite", so justify is not possible
		 * Don't bother to align an empty line
		 */
		vString2Diagram(pDiag, pAnchor);
		return;
	}

	/* Justify */
	fail(ucAlignment != ALIGNMENT_JUSTIFY);
	lSpaceWidth = lComputeStringWidth(" ", 1,
				pAnchor->tFontRef, pAnchor->usFontSize);
	lToAdd = lScreenWidth -
			lNetWidth -
			lDrawUnits2MilliPoints(pDiag->lXleft) +
			lRightIndentation;
#if defined(DEBUG)
	if (lToAdd / lSpaceWidth < -1) {
		DBG_DEC(lSpaceWidth);
		DBG_DEC(lToAdd);
		DBG_DEC(lScreenWidth);
		DBG_DEC(lNetWidth);
		DBG_DEC(lDrawUnits2MilliPoints(pDiag->lXleft));
		DBG_DEC(pDiag->lXleft);
		DBG_DEC(lRightIndentation);
	}
#endif /* DEBUG */
	lToAdd /= lSpaceWidth;
	DBG_DEC_C(lToAdd < 0, lToAdd);
	if (lToAdd <= 0) {
		vString2Diagram(pDiag, pAnchor);
		return;
	}

	iHoles = iComputeHoles(pAnchor);
	/* Justify by adding spaces */
	for (pTmp = pAnchor; pTmp != NULL; pTmp = pTmp->pNext) {
		fail(pTmp->tNextFree != strlen(pTmp->szStorage));
		fail(lToAdd < 0);
		szStorage = xmalloc(pTmp->tNextFree + (size_t)lToAdd + 1);
		pcNew = szStorage;
		for (pcOld = pTmp->szStorage; *pcOld != '\0'; pcOld++) {
			*pcNew++ = *pcOld;
			if (*pcOld == ' ' &&
			    *(pcOld + 1) != ' ' &&
			    iHoles > 0) {
				iFillerLen = (int)(lToAdd / iHoles);
				lToAdd -= iFillerLen;
				iHoles--;
				for (; iFillerLen > 0; iFillerLen--) {
					*pcNew++ = ' ';
				}
			}
		}
		*pcNew = '\0';
		pTmp->szStorage = xfree(pTmp->szStorage);
		pTmp->szStorage = szStorage;
		pTmp->tStorageSize = pTmp->tNextFree + (size_t)lToAdd + 1;
		pTmp->lStringWidth +=
			(pcNew - szStorage - (long)pTmp->tNextFree) *
			lSpaceWidth;
		fail(pcNew < szStorage);
		pTmp->tNextFree = (size_t)(pcNew - szStorage);
		fail(pTmp->tNextFree != strlen(pTmp->szStorage));
	}
	DBG_DEC_C(lToAdd != 0, lToAdd);
	vString2Diagram(pDiag, pAnchor);
} /* end of vJustify2Window */

/*
 * vResetStyles - reset the style information variables
 */
void
vResetStyles(void)
{
	(void)memset(auiHdrCounter, 0, sizeof(auiHdrCounter));
} /* end of vResetStyles */

/*
 * Add the style characters to the line
 *
 * Returns the length of the resulting string
 */
size_t
tStyle2Window(char *szLine, const style_block_type *pStyle,
	const section_block_type *pSection)
{
	char	*pcTxt;
	int	iIndex;
	BOOL	bNeedPrevLvl;
	level_type_enum	eNumType;
	USHORT	usStyleIndex;
	UCHAR	ucNFC;

	fail(szLine == NULL || pStyle == NULL || pSection == NULL);

	if (pStyle->usIstd == 0 || pStyle->usIstd > 9) {
		szLine[0] = '\0';
		return 0;
	}

	usStyleIndex = pStyle->usIstd - 1;
	/* Set the numbers */
	for (iIndex = 0; iIndex < 9; iIndex++) {
		if (iIndex == (int)usStyleIndex) {
			auiHdrCounter[iIndex]++;
		} else if (iIndex > (int)usStyleIndex) {
			auiHdrCounter[iIndex] = 0;
		} else if (auiHdrCounter[iIndex] == 0) {
			auiHdrCounter[iIndex] = 1;
		}
	}

	eNumType = eGetNumType(pStyle->ucNumLevel);
	if (eNumType != level_type_outline) {
		szLine[0] = '\0';
		return 0;
	}

	pcTxt = szLine;
	bNeedPrevLvl = (pSection->usNeedPrevLvl & BIT(usStyleIndex)) != 0;
	/* Print the numbers */
	for (iIndex = 0; iIndex <= (int)usStyleIndex; iIndex++) {
		if (iIndex == (int)usStyleIndex ||
		    (bNeedPrevLvl && iIndex < (int)usStyleIndex)) {
			ucNFC = pSection->aucNFC[iIndex];
			switch(ucNFC) {
			case LIST_ARABIC_NUM:
				pcTxt += sprintf(pcTxt, "%u",
					auiHdrCounter[iIndex]);
				break;
			case LIST_UPPER_ROMAN:
			case LIST_LOWER_ROMAN:
				pcTxt += tNumber2Roman(
					auiHdrCounter[iIndex],
					ucNFC == LIST_UPPER_ROMAN,
					pcTxt);
				break;
			case LIST_UPPER_ALPHA:
			case LIST_LOWER_ALPHA:
				pcTxt += tNumber2Alpha(
					auiHdrCounter[iIndex],
					ucNFC == LIST_UPPER_ALPHA,
					pcTxt);
				break;
			default:
				DBG_DEC(ucNFC);
				DBG_FIXME();
				pcTxt += sprintf(pcTxt, "%u",
					auiHdrCounter[iIndex]);
				break;
			}
			if (iIndex < (int)usStyleIndex) {
				*pcTxt++ = '.';
			} else if (iIndex == (int)usStyleIndex) {
				*pcTxt++ = ' ';
			}
		}
	}
	*pcTxt = '\0';
	NO_DBG_MSG_C((int)pStyle->usIstd >= 1 &&
		(int)pStyle->usIstd <= 9 &&
		eNumType != level_type_none &&
		eNumType != level_type_outline, szLine);
	NO_DBG_MSG_C(szLine[0] != '\0', szLine);
	fail(pcTxt < szLine);
	return (size_t)(pcTxt - szLine);
} /* end of tStyle2Window */

/*
 * vRemoveRowEnd - remove the end of table row indicator
 *
 * Remove the double TABLE_SEPARATOR characters from the end of the string.
 * Special: remove the TABLE_SEPARATOR, 0x0a sequence
 */
static void
vRemoveRowEnd(char *szRowTxt)
{
	int	iLastIndex;

	fail(szRowTxt == NULL || szRowTxt[0] == '\0');

	iLastIndex = (int)strlen(szRowTxt) - 1;

	if (szRowTxt[iLastIndex] == TABLE_SEPARATOR ||
	    szRowTxt[iLastIndex] == '\n') {
		szRowTxt[iLastIndex] = '\0';
		iLastIndex--;
	} else {
		DBG_HEX(szRowTxt[iLastIndex]);
	}

	if (iLastIndex >= 0 && szRowTxt[iLastIndex] == '\n') {
		szRowTxt[iLastIndex] = '\0';
		iLastIndex--;
	}

	if (iLastIndex >= 0 && szRowTxt[iLastIndex] == TABLE_SEPARATOR) {
		szRowTxt[iLastIndex] = '\0';
		return;
	}

	DBG_DEC(iLastIndex);
	DBG_HEX(szRowTxt[iLastIndex]);
	DBG_MSG(szRowTxt);
} /* end of vRemoveRowEnd */

/*
 * tComputeStringLengthMax - max string length in relation to max column width
 *
 * Return the maximum string length
 */
static size_t
tComputeStringLengthMax(const char *szString, size_t tColumnWidthMax)
{
	const char	*pcTmp;
	size_t	tLengthMax, tLenPrev, tLen, tWidth;

	fail(szString == NULL);
	fail(tColumnWidthMax == 0);

	pcTmp = strchr(szString, '\n');
	if (pcTmp != NULL) {
		tLengthMax = (size_t)(pcTmp - szString + 1);
	} else {
		tLengthMax = strlen(szString);
	}
	if (tLengthMax == 0) {
		return 0;
	}

	tLen = 0;
	tWidth = 0;
	for (;;) {
		tLenPrev = tLen;
		tLen += tGetCharacterLength(szString + tLen);
		DBG_DEC_C(tLen > tLengthMax, tLen);
		DBG_DEC_C(tLen > tLengthMax, tLengthMax);
		fail(tLen > tLengthMax);
		tWidth = tCountColumns(szString, tLen);
		if (tWidth > tColumnWidthMax) {
			return tLenPrev;
		}
		if (tLen >= tLengthMax) {
			return tLengthMax;
		}
	}
} /* end of tComputeStringLengthMax */

/*
 * tGetBreakingPoint - get the number of bytes that fit the column
 *
 * Returns the number of bytes that fit the column
 */
static size_t
tGetBreakingPoint(const char *szString,
	size_t tLen, size_t tWidth, size_t tColumnWidthMax)
{
	int	iIndex;

	fail(szString == NULL);
	fail(tLen > strlen(szString));
	fail(tWidth > tColumnWidthMax);

	if (tWidth < tColumnWidthMax ||
	    (tWidth == tColumnWidthMax &&
	     (szString[tLen] == ' ' ||
	      szString[tLen] == '\n' ||
	      szString[tLen] == '\0'))) {
		/* The string already fits, do nothing */
		return tLen;
	}
	/* Search for a breaking point */
	for (iIndex = (int)tLen - 1; iIndex >= 0; iIndex--) {
		if (szString[iIndex] == ' ') {
			return (size_t)iIndex;
		}
	}
	/* No breaking point found, just fill the column */
	return tLen;
} /* end of tGetBreakingPoint */

/*
 * vTableRow2Window - put a table row into a diagram
 */
void
vTableRow2Window(diagram_type *pDiag, output_type *pOutput,
		const row_block_type *pRowInfo)
{
	output_type	tRow;
	char	*aszColTxt[TABLE_COLUMN_MAX];
	char	*szLine, *pcTxt;
	long	lCharWidthLarge, lCharWidthSmall;
	size_t	tSize, tColumnWidthMax, tWidth, tLen;
	int	iIndex, iNbrOfColumns, iTmp;
	BOOL	bNotReady;

	fail(pDiag == NULL || pOutput == NULL || pRowInfo == NULL);
	fail(pOutput->szStorage == NULL);
	fail(pOutput->pNext != NULL);

	/* Character sizes */
	lCharWidthLarge = lComputeStringWidth("W", 1,
				pOutput->tFontRef, pOutput->usFontSize);
	NO_DBG_DEC(lCharWidthLarge);
	lCharWidthSmall = lComputeStringWidth("i", 1,
				pOutput->tFontRef, pOutput->usFontSize);
	NO_DBG_DEC(lCharWidthSmall);
	/* For the time being: use a fixed width font */
	fail(lCharWidthLarge != lCharWidthSmall);

	vRemoveRowEnd(pOutput->szStorage);

	/* Split the row text into a set of column texts */
	aszColTxt[0] = pOutput->szStorage;
	for (iNbrOfColumns = 1;
	     iNbrOfColumns < TABLE_COLUMN_MAX;
	     iNbrOfColumns++) {
		aszColTxt[iNbrOfColumns] =
				strchr(aszColTxt[iNbrOfColumns - 1],
					TABLE_SEPARATOR);
		if (aszColTxt[iNbrOfColumns] == NULL) {
			break;
		}
		*aszColTxt[iNbrOfColumns] = '\0';
		aszColTxt[iNbrOfColumns]++;
		NO_DBG_DEC(iNbrOfColumns);
		NO_DBG_MSG(aszColTxt[iNbrOfColumns]);
	}

	DBG_DEC_C(iNbrOfColumns != (int)pRowInfo->ucNumberOfColumns,
		iNbrOfColumns);
	DBG_DEC_C(iNbrOfColumns != (int)pRowInfo->ucNumberOfColumns,
		pRowInfo->ucNumberOfColumns);
	if (iNbrOfColumns != (int)pRowInfo->ucNumberOfColumns) {
		werr(0, "Skipping an unmatched table row");
		return;
	}

	if (bAddTableRow(pDiag, aszColTxt, iNbrOfColumns,
			pRowInfo->asColumnWidth, pRowInfo->ucBorderInfo)) {
		/* All work has been done */
		return;
	}

	/*
	 * Get enough space for the row.
	 * Worst case: three bytes per UTF-8 character
	 */
	tSize = 3 * (size_t)(lTwips2MilliPoints(pRowInfo->iColumnWidthSum) /
				lCharWidthSmall +
				(long)pRowInfo->ucNumberOfColumns + 3);
	szLine = xmalloc(tSize);

	do {
		/* Print one line of a table row */
		bNotReady = FALSE;
		pcTxt = szLine;
		*pcTxt++ = TABLE_SEPARATOR_CHAR;
		for (iIndex = 0; iIndex < iNbrOfColumns; iIndex++) {
			tColumnWidthMax =
				(size_t)(lTwips2MilliPoints(
					pRowInfo->asColumnWidth[iIndex]) /
					lCharWidthLarge);
			if (tColumnWidthMax == 0) {
				/* Minimum column width */
				tColumnWidthMax = 1;
			} else if (tColumnWidthMax > 1) {
				/* Make room for the TABLE_SEPARATOR_CHAR */
				tColumnWidthMax--;
			}
			NO_DBG_DEC(tColumnWidthMax);
			if (aszColTxt[iIndex] == NULL) {
				/* Add an empty column */
				for (iTmp = 0;
				     iTmp < (int)tColumnWidthMax;
				     iTmp++) {
					*pcTxt++ = (char)FILLER_CHAR;
				}
				*pcTxt++ = TABLE_SEPARATOR_CHAR;
				*pcTxt = '\0';
				continue;
			}
			/* Compute the length and width of the column text */
			tLen = tComputeStringLengthMax(
					aszColTxt[iIndex], tColumnWidthMax);
			NO_DBG_DEC(tLen);
			while (tLen != 0 &&
					(aszColTxt[iIndex][tLen - 1] == '\n' ||
					 aszColTxt[iIndex][tLen - 1] == ' ')) {
				aszColTxt[iIndex][tLen - 1] = ' ';
				tLen--;
			}
			tWidth = tCountColumns(aszColTxt[iIndex], tLen);
			fail(tWidth > tColumnWidthMax);
			tLen = tGetBreakingPoint(aszColTxt[iIndex],
					tLen, tWidth, tColumnWidthMax);
			tWidth = tCountColumns(aszColTxt[iIndex], tLen);
			if (tLen == 0 && *aszColTxt[iIndex] == '\0') {
				/* No text at all */
				aszColTxt[iIndex] = NULL;
			} else {
				/* Add the text */
				pcTxt += sprintf(pcTxt,
					"%.*s", (int)tLen, aszColTxt[iIndex]);
				if (tLen == 0 && *aszColTxt[iIndex] != ' ') {
					tLen = tGetCharacterLength(
							aszColTxt[iIndex]);
					DBG_CHR(*aszColTxt[iIndex]);
					DBG_FIXME();
					fail(tLen == 0);
				}
				aszColTxt[iIndex] += tLen;
				while (*aszColTxt[iIndex] == ' ') {
					aszColTxt[iIndex]++;
				}
				if (*aszColTxt[iIndex] == '\0') {
					/* This row is now complete */
					aszColTxt[iIndex] = NULL;
				} else {
					/* This row needs more lines */
					bNotReady = TRUE;
				}
			}
			/* Fill up the rest */
			for (iTmp = 0;
			     iTmp < (int)tColumnWidthMax - (int)tWidth;
			     iTmp++) {
				*pcTxt++ = (char)FILLER_CHAR;
			}
			/* End of column */
			*pcTxt++ = TABLE_SEPARATOR_CHAR;
			*pcTxt = '\0';
		}
		/* Output the table row line */
		*pcTxt = '\0';
		tRow = *pOutput;
		tRow.szStorage = szLine;
		fail(pcTxt < szLine);
		tRow.tNextFree = (size_t)(pcTxt - szLine);
		tRow.lStringWidth = lComputeStringWidth(
					tRow.szStorage,
					tRow.tNextFree,
					tRow.tFontRef,
					tRow.usFontSize);
		vString2Diagram(pDiag, &tRow);
	} while (bNotReady);
	/* Clean up before you leave */
	szLine = xfree(szLine);
} /* end of vTableRow2Window */
