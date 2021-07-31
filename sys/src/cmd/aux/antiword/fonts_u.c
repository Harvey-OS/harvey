/*
 * fonts_u.c
 * Copyright (C) 1999-2002 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to deal with fonts (Unix version)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "antiword.h"
#include "fontinfo.h"

/* Don't use fonts, just plain text */
static BOOL		bUsePlainText = TRUE;
/* Which character set should be used */
static encoding_type	eEncoding = encoding_neutral;


/*
 * pOpenFontTableFile - open the Font translation file
 *
 * Returns the file pointer or NULL
 */
FILE *
pOpenFontTableFile(void)
{
	FILE		*pFile;
	const char	*szHome, *szAntiword;
	const char	*szGlobalFile;
	char		szEnvironmentFile[PATH_MAX+1];
	char		szLocalFile[PATH_MAX+1];

	/* Try the environment version of the fontnames file */
	szAntiword = szGetAntiwordDirectory();
	if (szAntiword != NULL && szAntiword[0] != '\0') {
		if (strlen(szAntiword) +
		    sizeof(FILE_SEPARATOR FONTNAMES_FILE) >=
		    sizeof(szEnvironmentFile)) {
			werr(0,
			"The name of your ANTIWORDHOME directory is too long");
			return NULL;
		}
		sprintf(szEnvironmentFile, "%s%s",
			szAntiword,
			FILE_SEPARATOR FONTNAMES_FILE);
		DBG_MSG(szEnvironmentFile);

		pFile = fopen(szLocalFile, "r");
		if (pFile != NULL) {
			return pFile;
		}
	}

	/* Try the local version of the fontnames file */
	szHome = szGetHomeDirectory();
	if (strlen(szHome) +
	    sizeof(FILE_SEPARATOR ANTIWORD_DIR FILE_SEPARATOR FONTNAMES_FILE) >=
	    sizeof(szLocalFile)) {
		werr(0, "The name of your HOME directory is too long");
		return NULL;
	}

	sprintf(szLocalFile, "%s%s",
		szHome,
		FILE_SEPARATOR ANTIWORD_DIR FILE_SEPARATOR FONTNAMES_FILE);
	DBG_MSG(szLocalFile);

	pFile = fopen(szLocalFile, "r");
	if (pFile != NULL) {
		return pFile;
	}

	/* Try the global version of the fontnames file */
	szGlobalFile = GLOBAL_ANTIWORD_DIR FILE_SEPARATOR FONTNAMES_FILE;
	DBG_MSG(szGlobalFile);

	pFile = fopen(szGlobalFile, "r");
	if (pFile != NULL) {
		return pFile;
	}

	werr(0, "I can not open your fontnames file.\n"
		"Neither '%s' nor\n"
		"'%s' can be opened for reading.",
		szLocalFile, szGlobalFile);
	return NULL;
} /* end of pOpenFontTableFile */

/*
 * vCloseFont - close the current font, if any
 */
void
vCloseFont(void)
{
	NO_DBG_MSG("vCloseFont");
	/* For safety: to be overwritten at the next call of tOpenfont() */
	bUsePlainText = TRUE;
	eEncoding = encoding_neutral;
} /* end of vCloseFont */

/*
 * tOpenFont - make the specified font the current font
 *
 * Returns the font reference number
 */
draw_fontref
tOpenFont(UCHAR ucWordFontNumber, USHORT usFontStyle, USHORT usWordFontSize)
{
	options_type	tOptions;
	const char	*szOurFontname;
	int	iIndex, iFontnumber;

	NO_DBG_MSG("tOpenFont");
	NO_DBG_DEC(ucWordFontNumber);
	NO_DBG_HEX(usFontStyle);
	NO_DBG_DEC(usWordFontSize);

	/* Keep the relevant bits */
	usFontStyle &= FONT_BOLD|FONT_ITALIC;
	NO_DBG_HEX(usFontStyle);

	vGetOptions(&tOptions);
	bUsePlainText = tOptions.eConversionType != conversion_draw &&
			tOptions.eConversionType != conversion_ps;
	eEncoding = tOptions.eEncoding;

	if (bUsePlainText) {
		/* Plain text, no fonts */
		return (draw_fontref)0;
	}

	iFontnumber = iGetFontByNumber(ucWordFontNumber, usFontStyle);
	szOurFontname = szGetOurFontname(iFontnumber);
	if (szOurFontname == NULL || szOurFontname[0] == '\0') {
		DBG_DEC(iFontnumber);
		return (draw_fontref)0;
	}
	NO_DBG_MSG(szOurFontname);

	for (iIndex = 0; iIndex < (int)elementsof(szFontnames); iIndex++) {
		if (STREQ(szFontnames[iIndex], szOurFontname)) {
			NO_DBG_DEC(iIndex);
			return (draw_fontref)iIndex;
		}
	}
	return (draw_fontref)0;
} /* end of tOpenFont */

/*
 * tOpenTableFont - make the table font the current font
 *
 * Returns the font reference number
 */
draw_fontref
tOpenTableFont(USHORT usWordFontSize)
{
	options_type	tOptions;
	int	iWordFontnumber;

	NO_DBG_MSG("tOpenTableFont");

	vGetOptions(&tOptions);
	bUsePlainText = tOptions.eConversionType != conversion_draw &&
			tOptions.eConversionType != conversion_ps;
	eEncoding = tOptions.eEncoding;

	if (bUsePlainText) {
		/* Plain text, no fonts */
		return (draw_fontref)0;
	}

	iWordFontnumber = iFontname2Fontnumber(TABLE_FONT, FONT_REGULAR);
	if (iWordFontnumber < 0 || iWordFontnumber > (int)UCHAR_MAX) {
		DBG_DEC(iWordFontnumber);
		return (draw_fontref)0;
	}

	return tOpenFont((UCHAR)iWordFontnumber, FONT_REGULAR, usWordFontSize);
} /* end of tOpenTableFont */

/*
 * szGetFontname - get the fontname
 */
const char *
szGetFontname(draw_fontref tFontRef)
{
	fail((size_t)(UCHAR)tFontRef >= elementsof(szFontnames));
	return szFontnames[(int)(UCHAR)tFontRef];
} /* end of szGetFontname */

/*
 * lComputeStringWidth - compute the string width
 *
 * Note: the fontsize is specified in half-points!
 *       the stringlength is specified in bytes, not characters!
 *
 * Returns the string width in millipoints
 */
long
lComputeStringWidth(const char *szString, size_t tStringLength,
		draw_fontref tFontRef, USHORT usFontSize)
{
	USHORT	*ausCharWidths;
	UCHAR	*pucChar;
	long	lRelWidth;
	size_t	tIndex;
	int	iFontRef;

	fail(szString == NULL);
	fail(usFontSize < MIN_FONT_SIZE || usFontSize > MAX_FONT_SIZE);

	if (szString[0] == '\0' || tStringLength == 0) {
		/* Empty string */
		return 0;
	}

	if (eEncoding == encoding_utf8) {
		fail(!bUsePlainText);
		return lChar2MilliPoints(
			utf8_strwidth(szString, tStringLength));
	}

	if (bUsePlainText) {
		/* No current font, use "systemfont" */
		return lChar2MilliPoints(tStringLength);
	}

	DBG_DEC_C(eEncoding != encoding_iso_8859_1 &&
		eEncoding != encoding_iso_8859_2, eEncoding);
	fail(eEncoding != encoding_iso_8859_1 &&
		eEncoding != encoding_iso_8859_2);

	/* Compute the relative string width */
	iFontRef = (int)(UCHAR)tFontRef;
	if (eEncoding == encoding_iso_8859_2) {
		ausCharWidths = ausCharacterWidths2[iFontRef];
	} else {
		ausCharWidths = ausCharacterWidths1[iFontRef];
	}
	lRelWidth = 0;
	for (tIndex = 0, pucChar = (UCHAR *)szString;
	     tIndex < tStringLength;
	     tIndex++, pucChar++) {
		lRelWidth += (long)ausCharWidths[(int)*pucChar];
	}

	/* Compute the absolute string width */
	return (lRelWidth * usFontSize + 1) / 2;
} /* end of lComputeStringWidth */

/*
 * tCountColumns - count the number of columns in a string
 *
 * Note: the length is specified in bytes!
 *       A UTF-8 a character can be 0, 1 or 2 columns wide.
 *
 * Returns the number of columns
 */
size_t
tCountColumns(const char *szString, size_t tLength)
{
	fail(szString == NULL);

	if (eEncoding != encoding_utf8) {
		/* One byte, one character, one column */
		return tLength;
	}
	return (size_t)utf8_strwidth(szString, tLength);
} /* end of tCountColumns */

/*
 * tGetCharacterLength - the length of the specified character in bytes
 *
 * Returns the length in bytes
 */
size_t
tGetCharacterLength(const char *szString)
{
	fail(szString == NULL);

	if (eEncoding != encoding_utf8) {
		return 1;
	}
	return (size_t)utf8_chrlength(szString);
} /* end of tGetCharacterLength */
