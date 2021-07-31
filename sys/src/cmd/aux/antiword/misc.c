/*
 * misc.c
 * Copyright (C) 1998-2003 A.J. van Os; Released under GNU GPL
 *
 * Description:
 * Miscellaneous functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#if defined(__riscos)
#include "kernel.h"
#include "swis.h"
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif /* __riscos */
#if defined(__dos)
#define S_ISDIR(x)	(((x) & S_IFMT) == S_IFDIR)
#define S_ISREG(x)	(((x) & S_IFMT) == S_IFREG)
#endif /* __dos */
#include "antiword.h"


#if !defined(__riscos)
/*
 * szGetHomeDirectory - get the name of the home directory
 */
const char *
szGetHomeDirectory(void)
{
	const char	*szHome;

#if defined(__vms)
#include <unixlib.h>
	szHome = decc$translate_vms(getenv("HOME"));
#elif defined(__Plan9__)
	szHome = getenv("home");
#else 
	szHome = getenv("HOME");
#endif

	if (szHome == NULL || szHome[0] == '\0') {
#if defined(__dos)
		szHome = "C:";
#else
		werr(0, "I can't find the name of your HOME directory");
		szHome = "";
#endif /* __dos */
	}
	return szHome;
} /* end of szGetHomeDirectory */

/*
 * szGetAntiwordDirectory - get the name of the Antiword directory
 */
const char *
szGetAntiwordDirectory(void)
{
#if defined(__vms)
#include <unixlib.h>
	return decc$translate_vms(getenv("ANTIWORDHOME"));
#else
	return getenv("ANTIWORDHOME");
#endif /* __vms */
} /* end of szGetHomeDirectory */
#endif /* !__riscos */

/*
 * Get the size of the specified file.
 * Returns -1 if the file does not exist or is not a proper file.
 */
long
lGetFilesize(const char *szFilename)
{
#if defined(__riscos)
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;

	(void)memset(&regs, 0, sizeof(regs));
	regs.r[0] = 17;
	regs.r[1] = (int)szFilename;
	e = _kernel_swi(OS_File, &regs, &regs);
	if (e != NULL) {
		werr(0, "Get Filesize error %d: %s",
			e->errnum, e->errmess);
		return -1;
	}
	if (regs.r[0] != 1) {
		/* It's not a proper file or the file does not exist */
		return -1;
	}
	return (long)regs.r[4];
#else
	struct stat	tBuffer;

	if (stat(szFilename, &tBuffer) != 0) {
		werr(0, "Get Filesize error %d", errno);
		return -1;
	}
	if (!S_ISREG(tBuffer.st_mode)) {
		/* It's not a regular file */
		return -1;
	}
	return (long)tBuffer.st_size;
#endif /* __riscos */
} /* end of lGetFilesize */

#if defined(DEBUG)
void
vPrintBlock(const char	*szFile, int iLine,
		const UCHAR *aucBlock, size_t tLength)
{
	int i, j;

	fail(szFile == NULL || iLine < 0 || aucBlock == NULL);

	fprintf(stderr, "%s[%3d]:\n", szFile, iLine);
	for (i = 0; i < 32; i++) {
		if (16 * i >= (int)tLength) {
			return;
		}
		fprintf(stderr, "%03x: ", (unsigned int)(16 * i));
		for (j = 0; j < 16; j++) {
			if (16 * i + j < (int)tLength) {
				fprintf(stderr, "%02x ",
					(unsigned int)aucBlock[16 * i + j]);
			}
		}
		fprintf(stderr, "\n");
	}
} /* end of vPrintBlock */

void
vPrintUnicode(const char  *szFile, int iLine, const UCHAR *aucUni, size_t tLen)
{
	char	*szASCII;

	fail(tLen % 2 != 0);

	tLen /= 2;	/* Length in bytes to length in characters */
	szASCII = xmalloc(tLen + 1);
	(void)unincpy(szASCII, aucUni, tLen);
	szASCII[tLen] = '\0';
	(void)fprintf(stderr, "%s[%3d]: %.*s\n",
				szFile, iLine, (int)tLen, szASCII);
	szASCII = xfree(szASCII);
} /* end of vPrintUnicode */

BOOL
bCheckDoubleLinkedList(output_type *pAnchor)
{
	output_type	*pCurr, *pLast;
	int		iInList;

	pLast = pAnchor;
	iInList = 0;
	for (pCurr = pAnchor; pCurr != NULL; pCurr = pCurr->pNext) {
		pLast = pCurr;
		iInList++;
	}
	NO_DBG_DEC(iInList);
	for (pCurr = pLast; pCurr != NULL; pCurr = pCurr->pPrev) {
		pLast = pCurr;
		iInList--;
	}
	DBG_DEC_C(iInList != 0, iInList);
	return pAnchor == pLast && iInList == 0;
} /* end of bCheckDoubleLinkedList */
#endif /* DEBUG */

/*
 * bReadBytes
 * This function reads the specified number of bytes from the specified file,
 * starting from the specified offset.
 * Returns TRUE when successfull, otherwise FALSE
 */
BOOL
bReadBytes(UCHAR *aucBytes, size_t tMemb, ULONG ulOffset, FILE *pFile)
{
	fail(aucBytes == NULL || pFile == NULL || ulOffset > (ULONG)LONG_MAX);

	if (ulOffset > (ULONG)LONG_MAX) {
		return FALSE;
	}
	if (fseek(pFile, (long)ulOffset, SEEK_SET) != 0) {
		return FALSE;
	}
	if (fread(aucBytes, sizeof(UCHAR), tMemb, pFile) != tMemb) {
		return FALSE;
	}
	return TRUE;
} /* end of bReadBytes */

/*
 * bReadBuffer
 * This function fills the specified buffer with the specified number of bytes,
 * starting at the specified offset within the Big/Small Block Depot.
 *
 * Returns TRUE when successful, otherwise FALSE
 */
BOOL
bReadBuffer(FILE *pFile, ULONG ulStartBlock,
	const ULONG *aulBlockDepot, size_t tBlockDepotLen, size_t tBlockSize,
	UCHAR *aucBuffer, ULONG ulOffset, size_t tToRead)
{
	ULONG	ulBegin, ulIndex;
	size_t	tLen;

	fail(pFile == NULL);
	fail(ulStartBlock > MAX_BLOCKNUMBER && ulStartBlock != END_OF_CHAIN);
	fail(aulBlockDepot == NULL);
	fail(tBlockSize != BIG_BLOCK_SIZE && tBlockSize != SMALL_BLOCK_SIZE);
	fail(aucBuffer == NULL);
	fail(tToRead == 0);

	for (ulIndex = ulStartBlock;
	     ulIndex != END_OF_CHAIN && tToRead != 0;
	     ulIndex = aulBlockDepot[ulIndex]) {
		if (ulIndex >= (ULONG)tBlockDepotLen) {
			DBG_DEC(ulIndex);
			DBG_DEC(tBlockDepotLen);
			if (tBlockSize >= BIG_BLOCK_SIZE) {
				werr(1, "The Big Block Depot is damaged");
			} else {
				werr(1, "The Small Block Depot is damaged");
			}
		}
		if (ulOffset >= (ULONG)tBlockSize) {
			ulOffset -= tBlockSize;
			continue;
		}
		ulBegin = ulDepotOffset(ulIndex, tBlockSize) + ulOffset;
		tLen = min(tBlockSize - (size_t)ulOffset, tToRead);
		ulOffset = 0;
		if (!bReadBytes(aucBuffer, tLen, ulBegin, pFile)) {
			werr(0, "Read big block 0x%lx not possible", ulBegin);
			return FALSE;
		}
		aucBuffer += tLen;
		tToRead -= tLen;
	}
	DBG_DEC_C(tToRead != 0, tToRead);
	return tToRead == 0;
} /* end of bReadBuffer */

/*
 * Translate a Word colornumber into a true color for use in a drawfile
 *
 * Returns the true color
 */
ULONG
ulColor2Color(UCHAR ucFontColor)
{
	static const ULONG	aulColorTable[] = {
		/*  0 */	0x00000000UL,	/* Automatic */
		/*  1 */	0x00000000UL,	/* Black */
		/*  2 */	0xff000000UL,	/* Blue */
		/*  3 */	0xffff0000UL,	/* Turquoise */
		/*  4 */	0x00ff0000UL,	/* Bright Green */
		/*  5 */	0xff00ff00UL,	/* Pink */
		/*  6 */	0x0000ff00UL,	/* Red */
		/*  7 */	0x00ffff00UL,	/* Yellow */
		/*  8 */	0xffffff00UL,	/* White */
		/*  9 */	0x80000000UL,	/* Dark Blue */
		/* 10 */	0x80800000UL,	/* Teal */
		/* 11 */	0x00800000UL,	/* Green */
		/* 12 */	0x80008000UL,	/* Violet */
		/* 13 */	0x00008000UL,	/* Dark Red */
		/* 14 */	0x00808000UL,	/* Dark Yellow */
		/* 15 */	0x80808000UL,	/* Gray 50% */
		/* 16 */	0xc0c0c000UL,	/* Gray 25% */
	};
	if ((size_t)ucFontColor >= elementsof(aulColorTable)) {
		return aulColorTable[0];
	}
	return aulColorTable[(int)ucFontColor];
} /* end of ulColor2Color */

/*
 * iFindSplit - find a place to split the string
 *
 * returns the index of the split character or -1 if no split found.
 */
static int
iFindSplit(const char *szString, size_t tStringLen)
{
	size_t	tSplit;

	if (tStringLen == 0) {
		return -1;
	}
	tSplit = tStringLen - 1;
	while (tSplit >= 1) {
		if (szString[tSplit] == ' ' ||
		    (szString[tSplit] == '-' && szString[tSplit - 1] != ' ')) {
			return (int)tSplit;
		}
		tSplit--;
	}
	return -1;
} /* end of iFindSplit */

/*
 * pSplitList - split the specified list in a printable part and a leftover part
 *
 * returns the pointer to the leftover part
 */
output_type *
pSplitList(output_type *pAnchor)
{
	output_type	*pCurr, *pLeftOver;
	int		iIndex;

 	fail(pAnchor == NULL);

	for (pCurr = pAnchor; pCurr->pNext != NULL; pCurr = pCurr->pNext)
		;	/* EMPTY */
	iIndex = -1;
	for (; pCurr != NULL; pCurr = pCurr->pPrev) {
		iIndex = iFindSplit(pCurr->szStorage, pCurr->tNextFree);
		if (iIndex >= 0) {
			break;
		}
	}

	if (pCurr == NULL || iIndex < 0) {
		/* No split, no leftover */
		return NULL;
	}
	/* Split over the iIndex-th character */
	NO_DBG_MSG("pLeftOver");
	pLeftOver = xmalloc(sizeof(*pLeftOver));
	fail(pCurr->tNextFree < (size_t)iIndex);
	pLeftOver->tStorageSize = pCurr->tNextFree - (size_t)iIndex;
	pLeftOver->szStorage = xmalloc(pLeftOver->tStorageSize);
	pLeftOver->tNextFree = pCurr->tNextFree - (size_t)iIndex - 1;
	(void)strncpy(pLeftOver->szStorage,
		pCurr->szStorage + iIndex + 1, pLeftOver->tNextFree);
	pLeftOver->szStorage[pLeftOver->tNextFree] = '\0';
	NO_DBG_MSG(pLeftOver->szStorage);
	pLeftOver->ucFontColor = pCurr->ucFontColor;
	pLeftOver->usFontStyle = pCurr->usFontStyle;
	pLeftOver->tFontRef = pCurr->tFontRef;
	pLeftOver->usFontSize = pCurr->usFontSize;
	pLeftOver->lStringWidth = lComputeStringWidth(
					pLeftOver->szStorage,
					pLeftOver->tNextFree,
					pLeftOver->tFontRef,
					pLeftOver->usFontSize);
	pLeftOver->pPrev = NULL;
	pLeftOver->pNext = pCurr->pNext;
	if (pLeftOver->pNext != NULL) {
		pLeftOver->pNext->pPrev = pLeftOver;
	}
	fail(!bCheckDoubleLinkedList(pLeftOver));

	NO_DBG_MSG("pAnchor");
	NO_DBG_HEX(pCurr->szStorage[iIndex]);
	while (iIndex >= 0 && isspace((int)(UCHAR)pCurr->szStorage[iIndex])) {
		iIndex--;
	}
	pCurr->tNextFree = (size_t)iIndex + 1;
	pCurr->szStorage[pCurr->tNextFree] = '\0';
	NO_DBG_MSG(pCurr->szStorage);
	pCurr->lStringWidth = lComputeStringWidth(
					pCurr->szStorage,
					pCurr->tNextFree,
					pCurr->tFontRef,
					pCurr->usFontSize);
	pCurr->pNext = NULL;
	fail(!bCheckDoubleLinkedList(pAnchor));

	return pLeftOver;
} /* end of pSplitList */

/*
 * tNumber2Roman - convert a number to Roman Numerals
 *
 * returns the number of characters written
 */
size_t
tNumber2Roman(UINT uiNumber, BOOL bUpperCase, char *szOutput)
{
	char	*outp, *p, *q;
	UINT	uiNextVal, uiValue;

	fail(szOutput == NULL);

	uiNumber %= 4000;	/* Very high numbers can't be represented */
	if (uiNumber == 0) {
		szOutput[0] = '\0';
		return 0;
	}

	outp = szOutput;
	p = bUpperCase ? "M\2D\5C\2L\5X\2V\5I" : "m\2d\5c\2l\5x\2v\5i";
	uiValue = 1000;
	for (;;) {
		while (uiNumber >= uiValue) {
			*outp++ = *p;
			uiNumber -= uiValue;
		}
		if (uiNumber == 0) {
			*outp = '\0';
			fail(outp < szOutput);
			return (size_t)(outp - szOutput);
		}
		q = p + 1;
		uiNextVal = uiValue / (UINT)(UCHAR)*q;
		if ((int)*q == 2) {		/* magic */
			uiNextVal /= (UINT)(UCHAR)*(q += 2);
		}
		if (uiNumber + uiNextVal >= uiValue) {
			*outp++ = *++q;
			uiNumber += uiNextVal;
		} else {
			p++;
			uiValue /= (UINT)(UCHAR)(*p++);
		}
	}
} /* end of tNumber2Roman */

/*
 * iNumber2Alpha - convert a number to alphabetic "numbers"
 *
 * returns the number of characters written
 */
size_t
tNumber2Alpha(UINT uiNumber, BOOL bUpperCase, char *szOutput)
{
	char	*outp;
	UINT	uiTmp;

	fail(szOutput == NULL);

	if (uiNumber == 0) {
		szOutput[0] = '\0';
		return 0;
	}

	outp = szOutput;
	uiTmp = (UINT)(bUpperCase ? 'A': 'a');
	if (uiNumber <= 26) {
		uiNumber -= 1;
		*outp++ = (char)(uiTmp + uiNumber);
	} else if (uiNumber <= 26U + 26U*26U) {
		uiNumber -= 26 + 1;
		*outp++ = (char)(uiTmp + uiNumber / 26);
		*outp++ = (char)(uiTmp + uiNumber % 26);
	} else if (uiNumber <= 26U + 26U*26U + 26U*26U*26U) {
		uiNumber -= 26 + 26*26 + 1;
		*outp++ = (char)(uiTmp + uiNumber / (26*26));
		*outp++ = (char)(uiTmp + uiNumber / 26 % 26);
		*outp++ = (char)(uiTmp + uiNumber % 26);
	}
	*outp = '\0';
	fail(outp < szOutput);
	return (size_t)(outp - szOutput);
} /* end of tNumber2Alpha */

/*
 * unincpy - copy a counted Unicode string to an single-byte string
 */
char *
unincpy(char *s1, const UCHAR *s2, size_t n)
{
	char	*dest;
	ULONG	ulChar;
	size_t	tLen;
	USHORT	usUni;

	for (dest = s1, tLen = 0; tLen < n; dest++, tLen++) {
		usUni = usGetWord(tLen * 2, s2);
		if (usUni == 0) {
			break;
		}
		ulChar = ulTranslateCharacters(usUni, 0, 8,
				conversion_unknown, encoding_neutral, FALSE);
		if (ulChar == IGNORE_CHARACTER) {
			ulChar = (ULONG)'?';
		}
		*dest = (char)ulChar;
	}
	for (; tLen < n; tLen++) {
		*dest++ = '\0';
	}
	return s1;
} /* end of unincpy */

/*
 * unilen - calculate the length of a Unicode string
 *
 * returns the length in bytes
 */
size_t
unilen(const UCHAR *s)
{
	size_t	tLen;
	USHORT	usUni;

	tLen = 0;
	for (;;) {
		usUni = usGetWord(tLen, s);
		if (usUni == 0) {
			return tLen;
		}
		tLen += 2;
	}
} /* end of unilen */

/*
 * szBaseName - get the basename of the specified filename
 */
const char *
szBasename(const char *szFilename)
{
	const char	*szTmp;

	fail(szFilename == NULL);

	if (szFilename == NULL || szFilename[0] == '\0') {
		return "null";
	}

	szTmp = strrchr(szFilename, FILE_SEPARATOR[0]);
	if (szTmp == NULL) {
		return szFilename;
	}
	return ++szTmp;
} /* end of szBasename */

/*
 * lComputeLeading - compute the leading
 *
 * NOTE: the fontsize is specified in half points
 *
 * Returns the leading in drawunits
 */
long
lComputeLeading(USHORT usFontSize)
{
	long	lLeading;

	lLeading = (long)usFontSize * 500L;
	if (usFontSize < 18) {		/* Small text: 112% */
		lLeading *= 112;
	} else if (usFontSize < 28) {	/* Normal text: 124% */
		lLeading *= 124;
	} else if (usFontSize < 48) {	/* Small headlines: 104% */
		lLeading *= 104;
	} else {			/* Large headlines: 100% */
		lLeading *= 100;
	}
	lLeading = lMilliPoints2DrawUnits(lLeading);
	lLeading += 50;
	lLeading /= 100;
	return lLeading;
} /* end of lComputeLeading */

/*
 * Convert a UCS character to an UTF-8 string
 *
 * Returns the string length of the result
 */
size_t
tUcs2Utf8(ULONG ulChar, char *szResult, size_t tMaxResultLen)
{
	if (szResult == NULL || tMaxResultLen == 0) {
		return 0;
	}

	if (ulChar < 0x80 && tMaxResultLen >= 2) {
		szResult[0] = (char)ulChar;
		szResult[1] = '\0';
		return 1;
	}
	if (ulChar < 0x800 && tMaxResultLen >= 3) {
		szResult[0] = (char)(0xc0 | ulChar >> 6);
		szResult[1] = (char)(0x80 | (ulChar & 0x3f));
		szResult[2] = '\0';
		return 2;
	}
	if (ulChar < 0x10000 && tMaxResultLen >= 4) {
		szResult[0] = (char)(0xe0 | ulChar >> 12);
		szResult[1] = (char)(0x80 | (ulChar >> 6 & 0x3f));
		szResult[2] = (char)(0x80 | (ulChar & 0x3f));
		szResult[3] = '\0';
		return 3;
	}
	if (ulChar < 0x200000 && tMaxResultLen >= 5) {
		szResult[0] = (char)(0xf0 | ulChar >> 18);
		szResult[1] = (char)(0x80 | (ulChar >> 12 & 0x3f));
		szResult[2] = (char)(0x80 | (ulChar >> 6 & 0x3f));
		szResult[3] = (char)(0x80 | (ulChar & 0x3f));
		szResult[4] = '\0';
		return 4;
	}
	szResult[0] = '\0';
	return 0;
} /* end of tUcs2Utf8 */

/*
 * vGetBulletValue - get the bullet value for the conversing type and encoding
 */
void
vGetBulletValue(conversion_type eConversionType, encoding_type eEncoding,
	char *szResult, size_t tMaxResultLen)
{
	fail(szResult == NULL);
	fail(tMaxResultLen < 2);

	if (eEncoding == encoding_utf8) {
		(void)tUcs2Utf8(UNICODE_BULLET, szResult, tMaxResultLen);
	} else if (eEncoding == encoding_iso_8859_1 &&
		   eConversionType == conversion_ps) {
		szResult[0] = OUR_BULLET_PS;
		szResult[1] = '\0';
	} else {
		szResult[0] = OUR_BULLET_TEXT;
		szResult[1] = '\0';
	}
} /* end of vGetBulletValue */

/*
 * bAllZero - are all bytes zero?
 */
BOOL
bAllZero(const UCHAR *aucBytes, size_t tLength)
{
	size_t	tIndex;

	if (aucBytes == NULL || tLength == 0) {
		return TRUE;
	}

	for (tIndex = 0; tIndex < tLength; tIndex++) {
		if (aucBytes[tIndex] != 0) {
			return FALSE;
		}
	}
	return TRUE;
} /* end of bAllZero */
