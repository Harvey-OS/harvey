/*
 * fonts.c
 * Copyright (C) 1998-2002 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to deal with fonts (generic)
 */

#include <ctype.h>
#include <string.h>
#include "antiword.h"

/* Maximum line length in the font file */
#define FONT_LINE_LENGTH	81

/* Pitch */
#define PITCH_UNKNOWN		0
#define PITCH_FIXED		1
#define PITCH_VARIABLE		2

/* Font Family */
#define FAMILY_UNKNOWN		0
#define FAMILY_ROMAN		1
#define FAMILY_SWISS		2
#define FAMILY_MODERN		3
#define FAMILY_SCRIPT		4
#define FAMILY_DECORATIVE	5

/* Font Translation Table */
static size_t		tFontTableRecords = 0;
static font_table_type	*pFontTable = NULL;

/*
 * Find the given font in the font table
 *
 * returns the index into the FontTable, -1 if not found
 */
int
iGetFontByNumber(UCHAR ucWordFontNumber, USHORT usFontStyle)
{
	int	iIndex;

	for (iIndex = 0; iIndex < (int)tFontTableRecords; iIndex++) {
		if (ucWordFontNumber == pFontTable[iIndex].ucWordFontNumber &&
		    usFontStyle == pFontTable[iIndex].usFontStyle &&
		    pFontTable[iIndex].szOurFontname[0] != '\0') {
			return iIndex;
		}
	}
	DBG_DEC(ucWordFontNumber);
	DBG_HEX(usFontStyle);
	return -1;
} /* end of iGetFontByNumber */

/*
 * szGetOurFontname - Get our font name
 *
 * return our font name from the given index, NULL if not found
 */
const char *
szGetOurFontname(int iIndex)
{
	if (iIndex < 0 || iIndex >= (int)tFontTableRecords) {
		return NULL;
	}
	return pFontTable[iIndex].szOurFontname;
} /* end of szGetOurFontname */

/*
 * Find the given font in the font table
 *
 * returns the Word font number, -1 if not found
 */
int
iFontname2Fontnumber(const char *szOurFontname, USHORT usFontStyle)
{
	int	iIndex;

	for (iIndex = 0; iIndex < (int)tFontTableRecords; iIndex++) {
		if (pFontTable[iIndex].usFontStyle == usFontStyle &&
		    STREQ(pFontTable[iIndex].szOurFontname, szOurFontname)) {
			return (int)pFontTable[iIndex].ucWordFontNumber;
		}
	}
	return -1;
} /* end of iFontname2Fontnumber */

/*
 * See if the fontname from the Word file matches the fontname from the
 * font translation file.
 * If iBytesPerChar is one than aucWord is in ISO-8859-x (Word 2/6/7),
 * if iBytesPerChar is two than aucWord is in Unicode (Word 8/9/10).
 */
static BOOL
bFontEqual(const UCHAR *aucWord, const char *szTable, int iBytesPerChar)
{
	const UCHAR	*pucTmp;
	const char	*pcTmp;

	fail(aucWord == NULL || szTable == NULL);
	fail(iBytesPerChar != 1 && iBytesPerChar != 2);

	for (pucTmp = aucWord, pcTmp = szTable;
	     *pucTmp != 0;
	     pucTmp += iBytesPerChar, pcTmp++) {
		if (ulToUpper((ULONG)*pucTmp) !=
		    ulToUpper((ULONG)(UCHAR)*pcTmp)) {
			return FALSE;
		}
	}
	return *pcTmp == '\0';
} /* end of bFontEqual */

/*
 *
 */
static void
vFontname2Table(const UCHAR *aucFont, const UCHAR *aucAltFont,
	int iBytesPerChar, int iEmphasis, UCHAR ucFFN,
	const char *szWordFont, const char *szOurFont,
	font_table_type *pFontTableRecord)
{
	BOOL	bMatchFound;
	UCHAR	ucPrq, ucFf;

	fail(aucFont == NULL || aucFont[0] == 0);
	fail(aucAltFont != NULL && aucAltFont[0] == 0);
	fail(iBytesPerChar != 1 && iBytesPerChar != 2);
	fail(iEmphasis < 0 || iEmphasis > 3);
	fail(szWordFont == NULL || szWordFont[0] == '\0');
	fail(szOurFont == NULL || szOurFont[0] == '\0');
	fail(pFontTableRecord == NULL);

	bMatchFound = bFontEqual(aucFont, szWordFont, iBytesPerChar);

	if (!bMatchFound && aucAltFont != NULL) {
		bMatchFound = bFontEqual(aucAltFont, szWordFont, iBytesPerChar);
	}

	if (!bMatchFound &&
	    pFontTableRecord->szWordFontname[0] == '\0' &&
	    szWordFont[0] == '*' &&
	    szWordFont[1] == '\0') {
		/*
		 * szWordFont contains a "*", so szOurFont will contain the
		 * "default default" font. See if we can do better than that.
		 */
		ucPrq = ucFFN & 0x03;
		ucFf = (ucFFN & 0x70) >> 4;
		NO_DBG_DEC(ucPrq);
		NO_DBG_DEC(ucFf);
		if (ucPrq == PITCH_FIXED) {
			/* Set to the default monospaced font */
			switch (iEmphasis) {
			case 0: szOurFont = FONT_MONOSPACED_PLAIN; break;
			case 1: szOurFont = FONT_MONOSPACED_BOLD; break;
			case 2: szOurFont = FONT_MONOSPACED_ITALIC; break;
			case 3: szOurFont = FONT_MONOSPACED_BOLDITALIC; break;
			default: break;
			}
		} else if (ucFf == FAMILY_ROMAN) {
			/* Set to the default serif font */
			switch (iEmphasis) {
			case 0: szOurFont = FONT_SERIF_PLAIN; break;
			case 1: szOurFont = FONT_SERIF_BOLD; break;
			case 2: szOurFont = FONT_SERIF_ITALIC; break;
			case 3: szOurFont = FONT_SERIF_BOLDITALIC; break;
			default: break;
			}
		} else if (ucFf == FAMILY_SWISS) {
			/* Set to the default sans serif font */
			switch (iEmphasis) {
			case 0: szOurFont = FONT_SANS_SERIF_PLAIN; break;
			case 1: szOurFont = FONT_SANS_SERIF_BOLD; break;
			case 2: szOurFont = FONT_SANS_SERIF_ITALIC; break;
			case 3: szOurFont = FONT_SANS_SERIF_BOLDITALIC; break;
			default: break;
			}
		}
		bMatchFound = TRUE;
	}

	if (bMatchFound) {
		switch (iBytesPerChar) {
		case 1:
			(void)strncpy(pFontTableRecord->szWordFontname,
				(const char *)aucFont,
				sizeof(pFontTableRecord->szWordFontname) - 1);
			break;
		case 2:
			(void)unincpy(pFontTableRecord->szWordFontname,
				aucFont,
				sizeof(pFontTableRecord->szWordFontname) - 1);
			break;
		default:
			DBG_FIXME();
			pFontTableRecord->szWordFontname[0] = '\0';
			break;
		}
		pFontTableRecord->szWordFontname[
			sizeof(pFontTableRecord->szWordFontname) - 1] = '\0';
		(void)strncpy(pFontTableRecord->szOurFontname, szOurFont,
			sizeof(pFontTableRecord->szOurFontname) - 1);
		pFontTableRecord->szOurFontname[
			sizeof(pFontTableRecord->szOurFontname) - 1] = '\0';
		NO_DBG_MSG(pFontTableRecord->szWordFontname);
		NO_DBG_MSG(pFontTableRecord->szOurFontname);
	}
} /* end of vFontname2Table */

/*
 * vCreateFontTable - Create and initialize the internal font table
 */
static void
vCreateFontTable(void)
{
	font_table_type	*pTmp;
	int	iNbr;

	if (tFontTableRecords == 0) {
		pFontTable = xfree(pFontTable);
		return;
	}

	/* Create the font table */
	pFontTable = xcalloc(tFontTableRecords, sizeof(*pFontTable));

	/* Initialize the font table */
	for (iNbr = 0, pTmp = pFontTable;
	     pTmp < pFontTable + tFontTableRecords;
	     iNbr++, pTmp++) {
		pTmp->ucWordFontNumber = (UCHAR)(iNbr / 4);
		switch (iNbr % 4) {
		case 0:
			pTmp->usFontStyle = FONT_REGULAR;
			break;
		case 1:
			pTmp->usFontStyle = FONT_BOLD;
			break;
		case 2:
			pTmp->usFontStyle = FONT_ITALIC;
			break;
		case 3:
			pTmp->usFontStyle = FONT_BOLD|FONT_ITALIC;
			break;
		default:
			DBG_DEC(iNbr);
			break;
		}
	}
} /* end of vCreateFontTable */

/*
 * vMinimizeFontTable - make the font table as small as possible
 */
static void
vMinimizeFontTable(void)
{
	font_block_type		tFontNext;
	const style_block_type	*pStyle;
	const font_block_type	*pFont;
	font_table_type		*pTmp;
	int	iUnUsed;
	BOOL	bMustAddTableFont;

	NO_DBG_MSG("vMinimizeFontTable");

	if (tFontTableRecords == 0) {
		pFontTable = xfree(pFontTable);
		return;
	}

#if 0
	DBG_MSG("Before");
	DBG_DEC(tFontTableRecords);
	for (pTmp = pFontTable;
	     pTmp < pFontTable + tFontTableRecords;
	     pTmp++) {
		DBG_DEC(pTmp->ucWordFontNumber);
		DBG_HEX(pTmp->usFontStyle);
		DBG_MSG(pTmp->szWordFontname);
		DBG_MSG(pTmp->szOurFontname);
	}
#endif /* DEBUG */

	/* Default font/style is by definition in use */
	pFontTable[0].ucInUse = 1;

	/* See which fonts/styles are really being used */
	bMustAddTableFont = TRUE;

	/* The fonts/styles that will be used */
	pFont = NULL;
	while((pFont = pGetNextFontInfoListItem(pFont)) != NULL) {
		pTmp = pFontTable + 4 * (int)pFont->ucFontNumber;
		if (bIsBold(pFont->usFontStyle)) {
			pTmp++;
		}
		if (bIsItalic(pFont->usFontStyle)) {
			pTmp += 2;
		}
		if (pTmp >= pFontTable + tFontTableRecords) {
			continue;
		}
		if (STREQ(pTmp->szOurFontname, TABLE_FONT)) {
			/* The table font is already present */
			bMustAddTableFont = FALSE;
		}
		pTmp->ucInUse = 1;
	}

	/* The fonts/styles that might be used */
	pStyle = NULL;
	while((pStyle = pGetNextStyleInfoListItem(pStyle)) != NULL) {
		vFillFontFromStylesheet(pStyle->usIstdNext, &tFontNext);
		vCorrectFontValues(&tFontNext);
		pTmp = pFontTable + 4 * (int)tFontNext.ucFontNumber;
		if (bIsBold(tFontNext.usFontStyle)) {
			pTmp++;
		}
		if (bIsItalic(tFontNext.usFontStyle)) {
			pTmp += 2;
		}
		if (pTmp >= pFontTable + tFontTableRecords) {
			continue;
		}
		if (STREQ(pTmp->szOurFontname, TABLE_FONT)) {
			/* The table font is already present */
			bMustAddTableFont = FALSE;
		}
		pTmp->ucInUse = 1;
	}

	/* Remove the unused font entries from the font table */
	iUnUsed = 0;
	for (pTmp = pFontTable;
	     pTmp < pFontTable + tFontTableRecords;
	     pTmp++) {
		if (pTmp->ucInUse == 0) {
			iUnUsed++;
			continue;
		}
		if (iUnUsed > 0) {
			fail(pTmp - iUnUsed <= pFontTable);
			*(pTmp - iUnUsed) = *pTmp;
		}
	}
	tFontTableRecords -= (size_t)iUnUsed;
	fail(tFontTableRecords == 0);

	if (bMustAddTableFont) {
		pTmp = pFontTable + tFontTableRecords;
		pTmp->ucWordFontNumber = (pTmp - 1)->ucWordFontNumber + 1;
		pTmp->usFontStyle = FONT_REGULAR;
		pTmp->ucInUse = 1;
		strcpy(pTmp->szWordFontname, "Extra Table Font");
		strcpy(pTmp->szOurFontname, TABLE_FONT);
		tFontTableRecords++;
		iUnUsed--;
	}
	if (iUnUsed > 0) {
		/* Resize the font table */
		pFontTable = xrealloc(pFontTable,
				tFontTableRecords * sizeof(*pFontTable));
	}
#if defined(DEBUG)
	DBG_MSG("After");
	DBG_DEC(tFontTableRecords);
	for (pTmp = pFontTable;
	     pTmp < pFontTable + tFontTableRecords;
	     pTmp++) {
		DBG_DEC(pTmp->ucWordFontNumber);
		DBG_HEX(pTmp->usFontStyle);
		DBG_MSG(pTmp->szWordFontname);
		DBG_MSG(pTmp->szOurFontname);
	}
#endif /* DEBUG */
} /* end of vMinimizeFontTable */

/*
 * bReadFontFile - read and check a line from the font translation file
 *
 * returns TRUE when a correct line has been read, otherwise FALSE
 */
static BOOL
bReadFontFile(FILE *pFontTableFile, char *szWordFont,
	int *piItalic, int *piBold, char *szOurFont, int *piSpecial)
{
	char	*pcTmp;
	int	iFields;
	char	szLine[FONT_LINE_LENGTH];

	fail(szWordFont == NULL || szOurFont == NULL);
	fail(piItalic == NULL || piBold == NULL || piSpecial == NULL);

	while (fgets(szLine, (int)sizeof(szLine), pFontTableFile) != NULL) {
		if (szLine[0] == '#' ||
		    szLine[0] == '\n' ||
		    szLine[0] == '\r') {
			continue;
		}
		iFields = sscanf(szLine, "%[^,],%d,%d,%1s%[^,],%d",
			szWordFont, piItalic, piBold,
			&szOurFont[0], &szOurFont[1], piSpecial);
		if (iFields != 6) {
			pcTmp = strchr(szLine, '\r');
			if (pcTmp != NULL) {
				*pcTmp = '\0';
			}
			pcTmp = strchr(szLine, '\n');
			if (pcTmp != NULL) {
				*pcTmp = '\0';
			}
			DBG_DEC(iFields);
			werr(0, "Syntax error in: '%s'", szLine);
			continue;
		}
		if (strlen(szWordFont) >=
				sizeof(pFontTable[0].szWordFontname)) {
			werr(0, "Word fontname too long: '%s'", szWordFont);
			continue;
		}
		if (strlen(szOurFont) >=
				sizeof(pFontTable[0].szOurFontname)) {
			werr(0, "Local fontname too long: '%s'", szOurFont);
			continue;
		}
		/* The current line passed all the tests */
		return TRUE;
	}
	return FALSE;
} /* end of bReadFontFile */

/*
 * vCreate0FontTable - create a font table from Word for DOS
 */
void
vCreate0FontTable(void)
{
	FILE	*pFontTableFile;
	font_table_type	*pTmp;
	UCHAR	*aucFont;
	int	iBold, iItalic, iSpecial, iEmphasis, iFtc;
	UCHAR	ucPrq, ucFf, ucFFN;
	char	szWordFont[FONT_LINE_LENGTH], szOurFont[FONT_LINE_LENGTH];

	tFontTableRecords = 0;
	pFontTable = xfree(pFontTable);

	pFontTableFile = pOpenFontTableFile();
	if (pFontTableFile == NULL) {
		/* No translation table file, no translation table */
		return;
	}

	/* Get the maximum number of entries in the font table */
	tFontTableRecords = 64;
	tFontTableRecords *= 4;	/* Plain, Bold, Italic and Bold/italic */
	tFontTableRecords++;	/* One extra for the table-font */
	vCreateFontTable();

	/* Read the font translation file */
	while (bReadFontFile(pFontTableFile, szWordFont,
			&iItalic, &iBold, szOurFont, &iSpecial)) {
		iEmphasis = 0;
		if (iBold != 0) {
			iEmphasis++;
		}
		if (iItalic != 0) {
			iEmphasis += 2;
		}
		for (iFtc = 0, pTmp = pFontTable + iEmphasis;
		     pTmp < pFontTable + tFontTableRecords;
		     iFtc++, pTmp += 4) {
			if (iFtc >= 16 && iFtc <= 55) {
				ucPrq = PITCH_VARIABLE;
				ucFf = FAMILY_ROMAN;
				aucFont = (UCHAR *)"Times";
			} else {
				ucPrq = PITCH_FIXED;
				ucFf = FAMILY_MODERN;
				aucFont = (UCHAR *)"Courier";
			}
			ucFFN = (ucFf << 4) | ucPrq;
			vFontname2Table(aucFont, NULL, 1, iEmphasis,
					ucFFN, szWordFont, szOurFont, pTmp);
		}
	}
	(void)fclose(pFontTableFile);
	vMinimizeFontTable();
} /* end of vCreate0FontTable */

/*
 * vCreate2FontTable - create a font table from WinWord 1/2
 */
void
vCreate2FontTable(FILE *pFile, const UCHAR *aucHeader)
{
	FILE	*pFontTableFile;
	font_table_type	*pTmp;
	UCHAR	*aucFont;
	UCHAR	*aucBuffer;
	ULONG	ulBeginFontInfo;
	size_t	tFontInfoLen;
	int	iPos, iRecLen;
	int	iBold, iItalic, iSpecial, iEmphasis;
	UCHAR	ucFFN;
	char	szWordFont[FONT_LINE_LENGTH], szOurFont[FONT_LINE_LENGTH];

	fail(pFile == NULL || aucHeader == NULL);

	tFontTableRecords = 0;
	pFontTable = xfree(pFontTable);

	pFontTableFile = pOpenFontTableFile();
	if (pFontTableFile == NULL) {
		/* No translation table file, no translation table */
		return;
	}

	ulBeginFontInfo = ulGetLong(0xb2, aucHeader); /* fcSttbfffn */
	DBG_HEX(ulBeginFontInfo);
	tFontInfoLen = (size_t)usGetWord(0xb6, aucHeader); /* cbSttbfffn */
	DBG_DEC(tFontInfoLen);
	fail(tFontInfoLen < 6);

	if (ulBeginFontInfo > (ULONG)LONG_MAX) {
		/* Don't ask me why this is needed */
		DBG_HEX(ulBeginFontInfo);
		(void)fclose(pFontTableFile);
		return;
	}

	aucBuffer = xmalloc(tFontInfoLen);
	if (!bReadBytes(aucBuffer, tFontInfoLen, ulBeginFontInfo, pFile)) {
		aucBuffer = xfree(aucBuffer);
		(void)fclose(pFontTableFile);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tFontInfoLen);
	DBG_DEC(usGetWord(0, aucBuffer));

	/* Compute the maximum number of entries in the font table */
	tFontTableRecords = 0;
	iPos = 2;
	while (iPos + 3 < (int)tFontInfoLen) {
		iRecLen = (int)ucGetByte(iPos, aucBuffer);
		NO_DBG_DEC(iRecLen);
		NO_DBG_MSG(aucBuffer + iPos + 3);
		iPos += iRecLen + 1;
		tFontTableRecords++;
	}
	tFontTableRecords *= 4;	/* Plain, Bold, Italic and Bold/italic */
	tFontTableRecords++;	/* One extra for the table-font */
	vCreateFontTable();

	/* Read the font translation file */
	while (bReadFontFile(pFontTableFile, szWordFont,
			&iItalic, &iBold, szOurFont, &iSpecial)) {
		iEmphasis = 0;
		if (iBold != 0) {
			iEmphasis++;
		}
		if (iItalic != 0) {
			iEmphasis += 2;
		}
		pTmp = pFontTable + iEmphasis;
		iPos = 2;
		while (iPos + 3 < (int)tFontInfoLen) {
			iRecLen = (int)ucGetByte(iPos, aucBuffer);
			ucFFN = ucGetByte(iPos + 1, aucBuffer);
			aucFont = aucBuffer + iPos + 3;
			vFontname2Table(aucFont, NULL, 1, iEmphasis,
					ucFFN, szWordFont, szOurFont, pTmp);
			pTmp += 4;
			iPos += iRecLen + 1;
		}
	}
	(void)fclose(pFontTableFile);
	aucBuffer = xfree(aucBuffer);
	vMinimizeFontTable();
} /* end of vCreate2FontTable */

/*
 * vCreate6FontTable - create a font table from Word 6/7
 */
void
vCreate6FontTable(FILE *pFile, ULONG ulStartBlock,
	const ULONG *aulBBD, size_t tBBDLen,
	const UCHAR *aucHeader)
{
	FILE	*pFontTableFile;
	font_table_type	*pTmp;
	UCHAR	*aucFont, *aucAltFont;
	UCHAR	*aucBuffer;
	ULONG	ulBeginFontInfo;
	size_t	tFontInfoLen;
	int	iPos, iRecLen, iOffsetAltName;
	int	iBold, iItalic, iSpecial, iEmphasis;
	UCHAR	ucFFN;
	char	szWordFont[FONT_LINE_LENGTH], szOurFont[FONT_LINE_LENGTH];

	fail(pFile == NULL || aucHeader == NULL);
	fail(ulStartBlock > MAX_BLOCKNUMBER && ulStartBlock != END_OF_CHAIN);
	fail(aulBBD == NULL);

	tFontTableRecords = 0;
	pFontTable = xfree(pFontTable);

	pFontTableFile = pOpenFontTableFile();
	if (pFontTableFile == NULL) {
		/* No translation table file, no translation table */
		return;
	}

	ulBeginFontInfo = ulGetLong(0xd0, aucHeader); /* fcSttbfffn */
	DBG_HEX(ulBeginFontInfo);
	tFontInfoLen = (size_t)ulGetLong(0xd4, aucHeader); /* lcbSttbfffn */
	DBG_DEC(tFontInfoLen);
	fail(tFontInfoLen < 9);

	aucBuffer = xmalloc(tFontInfoLen);
	if (!bReadBuffer(pFile, ulStartBlock,
			aulBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, ulBeginFontInfo, tFontInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		(void)fclose(pFontTableFile);
		return;
	}
	DBG_DEC(usGetWord(0, aucBuffer));

	/* Compute the maximum number of entries in the font table */
	tFontTableRecords = 0;
	iPos = 2;
	while (iPos + 6 < (int)tFontInfoLen) {
		iRecLen = (int)ucGetByte(iPos, aucBuffer);
		NO_DBG_DEC(iRecLen);
		iOffsetAltName = (int)ucGetByte(iPos + 5, aucBuffer);
		NO_DBG_MSG(aucBuffer + iPos + 6);
		NO_DBG_MSG_C(iOffsetAltName > 0,
				aucBuffer + iPos + 6 + iOffsetAltName);
		iPos += iRecLen + 1;
		tFontTableRecords++;
	}
	tFontTableRecords *= 4;	/* Plain, Bold, Italic and Bold/italic */
	tFontTableRecords++;	/* One extra for the table-font */
	vCreateFontTable();

	/* Read the font translation file */
	while (bReadFontFile(pFontTableFile, szWordFont,
			&iItalic, &iBold, szOurFont, &iSpecial)) {
		iEmphasis = 0;
		if (iBold != 0) {
			iEmphasis++;
		}
		if (iItalic != 0) {
			iEmphasis += 2;
		}
		pTmp = pFontTable + iEmphasis;
		iPos = 2;
		while (iPos + 6 < (int)tFontInfoLen) {
			iRecLen = (int)ucGetByte(iPos, aucBuffer);
			ucFFN = ucGetByte(iPos + 1, aucBuffer);
			aucFont = aucBuffer + iPos + 6;
			iOffsetAltName = (int)ucGetByte(iPos + 5, aucBuffer);
			if (iOffsetAltName <= 0) {
				aucAltFont = NULL;
			} else {
				aucAltFont = aucFont + iOffsetAltName;
				NO_DBG_MSG(aucFont);
				NO_DBG_MSG(aucAltFont);
			}
			vFontname2Table(aucFont, aucAltFont, 1, iEmphasis,
					ucFFN, szWordFont, szOurFont, pTmp);
			pTmp += 4;
			iPos += iRecLen + 1;
		}
	}
	(void)fclose(pFontTableFile);
	aucBuffer = xfree(aucBuffer);
	vMinimizeFontTable();
} /* end of vCreate6FontTable */

/*
 * vCreate8FontTable - create a font table from Word 8/9/10
 */
void
vCreate8FontTable(FILE *pFile, const pps_info_type *pPPS,
	const ULONG *aulBBD, size_t tBBDLen,
	const ULONG *aulSBD, size_t tSBDLen,
	const UCHAR *aucHeader)
{
	FILE	*pFontTableFile;
	font_table_type	*pTmp;
	const ULONG	*aulBlockDepot;
	UCHAR	*aucFont, *aucAltFont;
	UCHAR	*aucBuffer;
	ULONG	ulBeginFontInfo;
	ULONG	ulTableSize, ulTableStartBlock;
	size_t	tFontInfoLen, tBlockDepotLen, tBlockSize;
	int	iPos, iRecLen, iOffsetAltName;
	int	iBold, iItalic, iSpecial, iEmphasis;
	USHORT	usDocStatus;
	UCHAR	ucFFN;
	char	szWordFont[FONT_LINE_LENGTH], szOurFont[FONT_LINE_LENGTH];

	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(aulBBD == NULL || aulSBD == NULL);

	tFontTableRecords = 0;
	pFontTable = xfree(pFontTable);

	pFontTableFile = pOpenFontTableFile();
	if (pFontTableFile == NULL) {
		/* No translation table file, no translation table */
		return;
	}

	ulBeginFontInfo = ulGetLong(0x112, aucHeader); /* fcSttbfffn */
	DBG_HEX(ulBeginFontInfo);
	tFontInfoLen = (size_t)ulGetLong(0x116, aucHeader); /* lcbSttbfffn */
	DBG_DEC(tFontInfoLen);
	fail(tFontInfoLen < 46);

	/* Use 0Table or 1Table? */
	usDocStatus = usGetWord(0x0a, aucHeader);
	if (usDocStatus & BIT(9)) {
		ulTableStartBlock = pPPS->t1Table.ulSB;
		ulTableSize = pPPS->t1Table.ulSize;
	} else {
		ulTableStartBlock = pPPS->t0Table.ulSB;
		ulTableSize = pPPS->t0Table.ulSize;
	}
	DBG_DEC(ulTableStartBlock);
	if (ulTableStartBlock == 0) {
		DBG_DEC(ulTableStartBlock);
		DBG_MSG("No fontname table");
		(void)fclose(pFontTableFile);
		return;
	}
	DBG_HEX(ulTableSize);
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
	aucBuffer = xmalloc(tFontInfoLen);
	if (!bReadBuffer(pFile, ulTableStartBlock,
			aulBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, ulBeginFontInfo, tFontInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		(void)fclose(pFontTableFile);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tFontInfoLen);

	/* Get the maximum number of entries in the font table */
	tFontTableRecords = (size_t)usGetWord(0, aucBuffer);
	tFontTableRecords *= 4;	/* Plain, Bold, Italic and Bold/italic */
	tFontTableRecords++;	/* One extra for the table-font */
	vCreateFontTable();

	/* Read the font translation file */
	while (bReadFontFile(pFontTableFile, szWordFont,
			&iItalic, &iBold, szOurFont, &iSpecial)) {
		iEmphasis = 0;
		if (iBold != 0) {
			iEmphasis++;
		}
		if (iItalic != 0) {
			iEmphasis += 2;
		}
		pTmp = pFontTable + iEmphasis;
		iPos = 4;
		while (iPos + 40 < (int)tFontInfoLen) {
			iRecLen = (int)ucGetByte(iPos, aucBuffer);
			ucFFN = ucGetByte(iPos + 1, aucBuffer);
			aucFont = aucBuffer + iPos + 40;
			iOffsetAltName = (int)unilen(aucFont);
			if (iPos + 40 + iOffsetAltName + 4 >= iRecLen) {
				aucAltFont = NULL;
			} else {
				aucAltFont = aucFont + iOffsetAltName + 2;
				NO_DBG_UNICODE(aucFont);
				NO_DBG_UNICODE(aucAltFont);
			}
			vFontname2Table(aucFont, aucAltFont, 2, iEmphasis,
					ucFFN, szWordFont, szOurFont, pTmp);
			pTmp += 4;
			iPos += iRecLen + 1;
		}
	}
	(void)fclose(pFontTableFile);
	aucBuffer = xfree(aucBuffer);
	vMinimizeFontTable();
} /* end of vCreate8FontTable */

/*
 * Destroy the internal font table by freeing its memory
 */
void
vDestroyFontTable(void)
{
	DBG_MSG("vDestroyFontTable");

	tFontTableRecords = 0;
	pFontTable = xfree(pFontTable);
} /* end of vDestroyFontTable */

/*
 * pGetNextFontTableRecord
 *
 * returns the next record in the table or NULL if there is no next record
 */
const font_table_type *
pGetNextFontTableRecord(const font_table_type *pRecordCurr)
{
	int	iIndexCurr;

	if (pRecordCurr == NULL) {
		/* No current record, so start with the first */
		return &pFontTable[0];
	}

	iIndexCurr = pRecordCurr - pFontTable;
	if (iIndexCurr + 1 < (int)tFontTableRecords) {
		/* There is a next record, so return it */
		return &pFontTable[iIndexCurr + 1];
	}
	/* There is no next record */
	return NULL;
} /* end of pGetNextFontTableRecord */

/*
 * tGetFontTableLength
 *
 * returns the number of records in the internal font table
 */
size_t
tGetFontTableLength(void)
{
	return tFontTableRecords;
} /* end of tGetFontTableLength */
