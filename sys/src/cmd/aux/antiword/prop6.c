/*
 * prop6.c
 * Copyright (C) 1998-2003 A.J. van Os; Released under GPL
 *
 * Description:
 * Read the property information from a MS Word 6 or 7 file
 */

#include <stdlib.h>
#include <string.h>
#include "antiword.h"


/*
 * iGet6InfoLength - the length of the information for Word 6/7 files
 */
static int
iGet6InfoLength(int iByteNbr, const UCHAR *aucGrpprl)
{
	int	iTmp, iDel, iAdd;

	switch (ucGetByte(iByteNbr, aucGrpprl)) {
	case   2: case  16: case  17: case  18: case  19: case  21: case  22:
	case  26: case  27: case  28: case  30: case  31: case  32: case  33:
	case  34: case  35: case  36: case  38: case  39: case  40: case  41:
	case  42: case  43: case  45: case  46: case  47: case  48: case  49:
	case  69: case  72: case  80: case  93: case  96: case  97: case  99:
	case 101: case 105: case 106: case 107: case 109: case 110: case 121:
	case 122: case 123: case 124: case 140: case 141: case 144: case 145:
	case 148: case 149: case 154: case 155: case 156: case 157: case 160:
	case 161: case 164: case 165: case 166: case 167: case 168: case 169:
	case 170: case 171: case 182: case 183: case 184: case 189: case 195:
	case 197: case 198:
		return 1 + 2;
	case   3: case  12: case  15: case  81: case 103: case 108: case 188:
	case 190: case 191:
		return 2 + (int)ucGetByte(iByteNbr + 1, aucGrpprl);
	case  20: case  70: case  74: case 192: case 194: case 196: case 200:
		return 1 + 4;
	case  23:
		iTmp = (int)ucGetByte(iByteNbr + 1, aucGrpprl);
		if (iTmp == 255) {
			iDel = (int)ucGetByte(iByteNbr + 2, aucGrpprl);
			iAdd = (int)ucGetByte(
					iByteNbr + 3 + iDel * 4, aucGrpprl);
			iTmp = 2 + iDel * 4 + iAdd * 3;
		}
		return 2 + iTmp;
	case  68: case 193: case 199:
		return 1 + 5;
	case  73: case  95: case 136: case 137:
		return 1 + 3;
	case 120:
		return 1 + 13;
	case 187:
		return 1 + 12;
	default:
		return 1 + 1;
	}
} /* end of iGet6InfoLength */

/*
 * Fill the section information block with information
 * from a Word 6/7 file.
 */
static void
vGet6SectionInfo(const UCHAR *aucGrpprl, size_t tBytes,
		section_block_type *pSection)
{
	UINT	uiIndex;
	int	iFodoOff, iInfoLen, iSize, iTmp;
	USHORT	usCcol;
	UCHAR	ucTmp;

	fail(aucGrpprl == NULL || pSection == NULL);

	iFodoOff = 0;
	while (tBytes >= (size_t)iFodoOff + 1) {
		iInfoLen = 0;
		switch (ucGetByte(iFodoOff, aucGrpprl)) {
		case 133:	/* olstAnm */
			iSize = (int)ucGetByte(iFodoOff + 1, aucGrpprl);
			DBG_DEC_C(iSize != 212, iSize);
			for (uiIndex = 0, iTmp = iFodoOff + 2;
			     uiIndex < 9 && iTmp < iFodoOff + 2 + iSize - 15;
			     uiIndex++, iTmp += 16) {
				pSection->aucNFC[uiIndex] =
						ucGetByte(iTmp, aucGrpprl);
				NO_DBG_DEC(pSection->aucNFC[uiIndex]);
				ucTmp = ucGetByte(iTmp + 3, aucGrpprl);
				NO_DBG_HEX(ucTmp);
				if ((ucTmp & BIT(2)) != 0) {
					pSection->usNeedPrevLvl |=
							(USHORT)BIT(uiIndex);
				}
				if ((ucTmp & BIT(3)) != 0) {
					pSection->usHangingIndent |=
							(USHORT)BIT(uiIndex);
				}
			}
			DBG_HEX(pSection->usNeedPrevLvl);
			DBG_HEX(pSection->usHangingIndent);
			break;
		case 142:	/* bkc */
			ucTmp = ucGetByte(iFodoOff + 1, aucGrpprl);
			DBG_DEC(ucTmp);
			pSection->bNewPage = ucTmp != 0 && ucTmp != 1;
			break;
		case 144:	/* ccolM1 */
			usCcol = 1 + usGetWord(iFodoOff + 1, aucGrpprl);
			DBG_DEC(usCcol);
			break;
		default:
			break;
		}
		if (iInfoLen <= 0) {
			iInfoLen = iGet6InfoLength(iFodoOff, aucGrpprl);
			fail(iInfoLen <= 0);
		}
		iFodoOff += iInfoLen;
	}
} /* end of vGet6SectionInfo */

/*
 * Build the lists with Section Property Information for Word 6/7 files
 */
void
vGet6SepInfo(FILE *pFile, ULONG ulStartBlock,
	const ULONG *aulBBD, size_t tBBDLen,
	const UCHAR *aucHeader)
{
	section_block_type	tSection;
	ULONG		*aulSectPage, *aulTextOffset;
	UCHAR	*aucBuffer, *aucFpage;
	ULONG	ulBeginSectInfo;
	size_t	tSectInfoLen, tOffset, tLen, tBytes;
	int	iIndex;
	UCHAR	aucTmp[2];

	fail(pFile == NULL || aucHeader == NULL);
	fail(ulStartBlock > MAX_BLOCKNUMBER && ulStartBlock != END_OF_CHAIN);
	fail(aulBBD == NULL);

	ulBeginSectInfo = ulGetLong(0x88, aucHeader); /* fcPlcfsed */
	DBG_HEX(ulBeginSectInfo);
	tSectInfoLen = (size_t)ulGetLong(0x8c, aucHeader); /* lcbPlcfsed */
	DBG_DEC(tSectInfoLen);
	if (tSectInfoLen < 4) {
		DBG_DEC(tSectInfoLen);
		return;
	}

	aucBuffer = xmalloc(tSectInfoLen);
	if (!bReadBuffer(pFile, ulStartBlock,
			aulBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, ulBeginSectInfo, tSectInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tSectInfoLen);

	/* Read the Section Descriptors */
	tLen = (tSectInfoLen - 4) / 16;
	/* Save the section offsets */
	aulTextOffset = xcalloc(tLen, sizeof(ULONG));
	for (iIndex = 0, tOffset = 0;
	     iIndex < (int)tLen;
	     iIndex++, tOffset += 4) {
		aulTextOffset[iIndex] = ulGetLong(tOffset, aucBuffer);
	}
	/* Save the Sepx offsets */
	aulSectPage = xcalloc(tLen, sizeof(ULONG));
	for (iIndex = 0, tOffset = (tLen + 1) * 4;
	     iIndex < (int)tLen;
	     iIndex++, tOffset += 12) {
		aulSectPage[iIndex] = ulGetLong(tOffset + 2, aucBuffer);
		NO_DBG_HEX(aulSectPage[iIndex]); /* fcSepx */
	}
	aucBuffer = xfree(aucBuffer);

	/* Read the Section Properties */
	for (iIndex = 0; iIndex < (int)tLen; iIndex++) {
		if (aulSectPage[iIndex] == FC_INVALID) {
			vDefault2SectionInfoList(aulTextOffset[iIndex]);
			continue;
		}
		/* Get the number of bytes to read */
		if (!bReadBuffer(pFile, ulStartBlock,
				aulBBD, tBBDLen, BIG_BLOCK_SIZE,
				aucTmp, aulSectPage[iIndex], 2)) {
			continue;
		}
		tBytes = 2 + (size_t)usGetWord(0, aucTmp);
		NO_DBG_DEC(tBytes);
		/* Read the bytes */
		aucFpage = xmalloc(tBytes);
		if (!bReadBuffer(pFile, ulStartBlock,
				aulBBD, tBBDLen, BIG_BLOCK_SIZE,
				aucFpage, aulSectPage[iIndex], tBytes)) {
			aucFpage = xfree(aucFpage);
			continue;
		}
		NO_DBG_PRINT_BLOCK(aucFpage, tBytes);
		/* Process the bytes */
		vGetDefaultSection(&tSection);
		vGet6SectionInfo(aucFpage + 2, tBytes - 2, &tSection);
		vAdd2SectionInfoList(&tSection, aulTextOffset[iIndex]);
		aucFpage = xfree(aucFpage);
	}
	aulTextOffset = xfree(aulTextOffset);
	aulSectPage = xfree(aulSectPage);
} /* end of vGet6SepInfo */

/*
 * Translate the rowinfo to a member of the row_info enumeration
 */
row_info_enum
eGet6RowInfo(int iFodo,
	const UCHAR *aucGrpprl, int iBytes, row_block_type *pRow)
{
	int	iFodoOff, iInfoLen;
	int	iIndex, iSize, iCol;
	int	iPosCurr, iPosPrev;
	USHORT	usTmp;
	BOOL	bFound24_0, bFound24_1, bFound25_0, bFound25_1, bFound190;

	fail(iFodo < 0 || aucGrpprl == NULL || pRow == NULL);

	iFodoOff = 0;
	bFound24_0 = FALSE;
	bFound24_1 = FALSE;
	bFound25_0 = FALSE;
	bFound25_1 = FALSE;
	bFound190 = FALSE;
	while (iBytes >= iFodoOff + 1) {
		iInfoLen = 0;
		switch (ucGetByte(iFodo + iFodoOff, aucGrpprl)) {
		case  24:	/* fIntable */
			if (odd(ucGetByte(iFodo + iFodoOff + 1, aucGrpprl))) {
				bFound24_1 = TRUE;
			} else {
				bFound24_0 = TRUE;
			}
			break;
		case  25:	/* fTtp */
			if (odd(ucGetByte(iFodo + iFodoOff + 1, aucGrpprl))) {
				bFound25_1 = TRUE;
			} else {
				bFound25_0 = TRUE;
			}
			break;
		case 38:	/* brcTop */
			usTmp = usGetWord(iFodo + iFodoOff + 1, aucGrpprl);
			usTmp &= 0x0018;
			NO_DBG_DEC(usTmp >> 3);
			if (usTmp == 0) {
				pRow->ucBorderInfo &= ~TABLE_BORDER_TOP;
			} else {
				pRow->ucBorderInfo |= TABLE_BORDER_TOP;
			}
			break;
		case 39:	/* brcLeft */
			usTmp = usGetWord(iFodo + iFodoOff + 1, aucGrpprl);
			usTmp &= 0x0018;
			NO_DBG_DEC(usTmp >> 3);
			if (usTmp == 0) {
				pRow->ucBorderInfo &= ~TABLE_BORDER_LEFT;
			} else {
				pRow->ucBorderInfo |= TABLE_BORDER_LEFT;
			}
			break;
		case 40:	/* brcBottom */
			usTmp = usGetWord(iFodo + iFodoOff + 1, aucGrpprl);
			usTmp &= 0x0018;
			NO_DBG_DEC(usTmp >> 3);
			if (usTmp == 0) {
				pRow->ucBorderInfo &= ~TABLE_BORDER_BOTTOM;
			} else {
				pRow->ucBorderInfo |= TABLE_BORDER_BOTTOM;
			}
			break;
		case 41:	/* brcRight */
			usTmp = usGetWord(iFodo + iFodoOff + 1, aucGrpprl);
			usTmp &= 0x0018;
			NO_DBG_DEC(usTmp >> 3);
			if (usTmp == 0) {
				pRow->ucBorderInfo &= ~TABLE_BORDER_RIGHT;
			} else {
				pRow->ucBorderInfo |= TABLE_BORDER_RIGHT;
			}
			break;
		case 190:	/* cDefTable */
			iSize = (int)usGetWord(iFodo + iFodoOff + 1, aucGrpprl);
			if (iSize < 6 || iBytes < iFodoOff + 7) {
				DBG_DEC(iSize);
				DBG_DEC(iFodoOff);
				iInfoLen = 1;
				break;
			}
			iCol = (int)ucGetByte(iFodo + iFodoOff + 3, aucGrpprl);
			if (iCol < 1 ||
			    iBytes < iFodoOff + 3 + (iCol + 1) * 2) {
				DBG_DEC(iCol);
				DBG_DEC(iFodoOff);
				iInfoLen = 1;
				break;
			}
			if (iCol >= (int)elementsof(pRow->asColumnWidth)) {
				DBG_DEC(iCol);
				werr(1, "The number of columns is corrupt");
			}
			pRow->ucNumberOfColumns = (UCHAR)iCol;
			pRow->iColumnWidthSum = 0;
			iPosPrev = (int)(short)usGetWord(
					iFodo + iFodoOff + 4,
					aucGrpprl);
			for (iIndex = 0; iIndex < iCol; iIndex++) {
				iPosCurr = (int)(short)usGetWord(
					iFodo + iFodoOff + 6 + iIndex * 2,
					aucGrpprl);
				pRow->asColumnWidth[iIndex] =
						(short)(iPosCurr - iPosPrev);
				pRow->iColumnWidthSum +=
					pRow->asColumnWidth[iIndex];
				iPosPrev = iPosCurr;
			}
			bFound190 = TRUE;
			break;
		default:
			break;
		}
		if (iInfoLen <= 0) {
			iInfoLen =
				iGet6InfoLength(iFodo + iFodoOff, aucGrpprl);
			fail(iInfoLen <= 0);
		}
		iFodoOff += iInfoLen;
	}
	if (bFound24_1 && bFound25_1 && bFound190) {
		return found_end_of_row;
	}
	if (bFound24_0 && bFound25_0 && !bFound190) {
		return found_not_end_of_row;
	}
	if (bFound24_1) {
		return found_a_cell;
	}
	if (bFound24_0) {
		return found_not_a_cell;
	}
	return found_nothing;
} /* end of eGet6RowInfo */

/*
 * Fill the style information block with information
 * from a Word 6/7 file.
 */
void
vGet6StyleInfo(int iFodo,
	const UCHAR *aucGrpprl, int iBytes, style_block_type *pStyle)
{
	int	iFodoOff, iInfoLen;
	int	iTmp, iDel, iAdd, iBefore;
	short	sTmp;
	UCHAR	ucTmp;

	fail(iFodo < 0 || aucGrpprl == NULL || pStyle == NULL);

	NO_DBG_DEC(pStyle->usIstd);

	iFodoOff = 0;
	while (iBytes >= iFodoOff + 1) {
		iInfoLen = 0;
		switch (ucGetByte(iFodo + iFodoOff, aucGrpprl)) {
		case   2:	/* istd */
			sTmp = (short)ucGetByte(
					iFodo + iFodoOff + 1, aucGrpprl);
			NO_DBG_DEC(sTmp);
			break;
		case   5:	/* jc */
			pStyle->ucAlignment = ucGetByte(
					iFodo + iFodoOff + 1, aucGrpprl);
			break;
		case  12:	/* anld */
			iTmp = (int)ucGetByte(
					iFodo + iFodoOff + 1, aucGrpprl);
			DBG_DEC_C(iTmp < 52, iTmp);
			if (iTmp >= 1) {
				pStyle->ucNFC = ucGetByte(
					iFodo + iFodoOff + 2, aucGrpprl);
			}
			if (pStyle->ucNFC != LIST_BULLETS && iTmp >= 2) {
				iBefore = (int)ucGetByte(
					iFodo + iFodoOff + 3, aucGrpprl);
			} else {
				iBefore = 0;
			}
			if (iTmp >= 12) {
				pStyle->usStartAt = usGetWord(
					iFodo + iFodoOff + 12, aucGrpprl);
			}
			if (iTmp >= iBefore + 21) {
				pStyle->usListChar = (USHORT)ucGetByte(
					iFodo + iFodoOff + iBefore + 22,
					aucGrpprl);
				NO_DBG_HEX(pStyle->usListChar);
			}
			break;
		case  13:	/* nLvlAnm */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			pStyle->ucNumLevel = ucTmp;
			pStyle->bNumPause =
				eGetNumType(ucTmp) == level_type_pause;
			break;
		case  15:	/* ChgTabsPapx */
			iTmp = (int)ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			if (iTmp < 2) {
				iInfoLen = 1;
				break;
			}
			NO_DBG_DEC(iTmp);
			iDel = (int)ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
			if (iTmp < 2 + 2 * iDel) {
				iInfoLen = 1;
				break;
			}
			NO_DBG_DEC(iDel);
			iAdd = (int)ucGetByte(
				iFodo + iFodoOff + 3 + 2 * iDel, aucGrpprl);
			if (iTmp < 2 + 2 * iDel + 2 * iAdd) {
				iInfoLen = 1;
				break;
			}
			NO_DBG_DEC(iAdd);
			break;
		case  16:	/* dxaRight */
			pStyle->sRightIndent = (short)usGetWord(
					iFodo + iFodoOff + 1, aucGrpprl);
			NO_DBG_DEC(pStyle->sRightIndent);
			break;
		case  17:	/* dxaLeft */
			pStyle->sLeftIndent = (short)usGetWord(
					iFodo + iFodoOff + 1, aucGrpprl);
			NO_DBG_DEC(pStyle->sLeftIndent);
			break;
		case  18:	/* Nest dxaLeft */
			sTmp = (short)usGetWord(
					iFodo + iFodoOff + 1, aucGrpprl);
			pStyle->sLeftIndent += sTmp;
			if (pStyle->sLeftIndent < 0) {
				pStyle->sLeftIndent = 0;
			}
			NO_DBG_DEC(sTmp);
			NO_DBG_DEC(pStyle->sLeftIndent);
			break;
		case  19:	/* dxaLeft1 */
			pStyle->sLeftIndent1 = (short)usGetWord(
					iFodo + iFodoOff + 1, aucGrpprl);
			NO_DBG_DEC(pStyle->sLeftIndent1);
			break;
		case  21:	/* dyaBefore */
			pStyle->usBeforeIndent = usGetWord(
					iFodo + iFodoOff + 1, aucGrpprl);
			NO_DBG_DEC(pStyle->usBeforeIndent);
			break;
		case  22:	/* dyaAfter */
			pStyle->usAfterIndent = usGetWord(
					iFodo + iFodoOff + 1, aucGrpprl);
			NO_DBG_DEC(pStyle->usAfterIndent);
			break;
		default:
			break;
		}
		if (iInfoLen <= 0) {
			iInfoLen =
				iGet6InfoLength(iFodo + iFodoOff, aucGrpprl);
			fail(iInfoLen <= 0);
		}
		iFodoOff += iInfoLen;
	}
} /* end of vGet6StyleInfo */

/*
 * Build the lists with Paragraph Information for Word 6/7 files
 */
void
vGet6PapInfo(FILE *pFile, ULONG ulStartBlock,
	const ULONG *aulBBD, size_t tBBDLen,
	const UCHAR *aucHeader)
{
	row_block_type		tRow;
	style_block_type	tStyle;
	USHORT	*ausParfPage;
	UCHAR	*aucBuffer;
	ULONG	ulCharPos, ulCharPosFirst, ulCharPosLast;
	ULONG	ulBeginParfInfo;
	size_t	tParfInfoLen, tParfPageNum, tOffset, tSize, tLenOld, tLen;
	int	iIndex, iIndex2, iRun, iFodo, iLen;
	row_info_enum	eRowInfo;
	USHORT	usParfFirstPage, usCount, usIstd;
	UCHAR	aucFpage[BIG_BLOCK_SIZE];

	fail(pFile == NULL || aucHeader == NULL);
	fail(ulStartBlock > MAX_BLOCKNUMBER && ulStartBlock != END_OF_CHAIN);
	fail(aulBBD == NULL);

	ulBeginParfInfo = ulGetLong(0xc0, aucHeader); /* fcPlcfbtePapx */
	NO_DBG_HEX(ulBeginParfInfo);
	tParfInfoLen = (size_t)ulGetLong(0xc4, aucHeader); /* lcbPlcfbtePapx */
	NO_DBG_DEC(tParfInfoLen);
	if (tParfInfoLen < 4) {
		DBG_DEC(tParfInfoLen);
		return;
	}

	aucBuffer = xmalloc(tParfInfoLen);
	if (!bReadBuffer(pFile, ulStartBlock,
			aulBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, ulBeginParfInfo, tParfInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tParfInfoLen);

	tLen = (tParfInfoLen - 4) / 6;
	ausParfPage = xcalloc(tLen, sizeof(USHORT));
	for (iIndex = 0, tOffset = (tLen + 1) * 4;
	     iIndex < (int)tLen;
	     iIndex++, tOffset += 2) {
		 ausParfPage[iIndex] = usGetWord(tOffset, aucBuffer);
		 NO_DBG_DEC(ausParfPage[iIndex]);
	}
	DBG_HEX(ulGetLong(0, aucBuffer));
	aucBuffer = xfree(aucBuffer);
	tParfPageNum = (size_t)usGetWord(0x190, aucHeader); /* cpnBtePap */
	DBG_DEC(tParfPageNum);
	if (tLen < tParfPageNum) {
		/* Replace ParfPage by a longer version */
		tLenOld = tLen;
		usParfFirstPage = usGetWord(0x18c, aucHeader); /* pnPapFirst */
		DBG_DEC(usParfFirstPage);
		tLen += tParfPageNum - 1;
		tSize = tLen * sizeof(USHORT);
		ausParfPage = xrealloc(ausParfPage, tSize);
		/* Add new values */
		usCount = usParfFirstPage + 1;
		for (iIndex = (int)tLenOld; iIndex < (int)tLen; iIndex++) {
			ausParfPage[iIndex] = usCount;
			NO_DBG_DEC(ausParfPage[iIndex]);
			usCount++;
		}
	}

	(void)memset(&tRow, 0, sizeof(tRow));
	ulCharPosFirst = CP_INVALID;
	for (iIndex = 0; iIndex < (int)tLen; iIndex++) {
		if (!bReadBuffer(pFile, ulStartBlock,
				aulBBD, tBBDLen, BIG_BLOCK_SIZE,
				aucFpage,
				(ULONG)ausParfPage[iIndex] * BIG_BLOCK_SIZE,
				BIG_BLOCK_SIZE)) {
			break;
		}
		iRun = (int)ucGetByte(0x1ff, aucFpage);
		NO_DBG_DEC(iRun);
		for (iIndex2 = 0; iIndex2 < iRun; iIndex2++) {
			NO_DBG_HEX(ulGetLong(iIndex2 * 4, aucFpage));
			iFodo = 2 * (int)ucGetByte(
				(iRun + 1) * 4 + iIndex2 * 7, aucFpage);
			if (iFodo <= 0) {
				continue;
			}

			iLen = 2 * (int)ucGetByte(iFodo, aucFpage);

			usIstd = (USHORT)ucGetByte(iFodo + 1, aucFpage);
			vFillStyleFromStylesheet(usIstd, &tStyle);
			vGet6StyleInfo(iFodo, aucFpage + 3, iLen - 3, &tStyle);
			ulCharPos = ulGetLong(iIndex2 * 4, aucFpage);
			NO_DBG_HEX(ulCharPos);
			tStyle.ulFileOffset = ulCharPos2FileOffset(ulCharPos);
			vAdd2StyleInfoList(&tStyle);

			eRowInfo = eGet6RowInfo(iFodo,
					aucFpage + 3, iLen - 3, &tRow);
			switch(eRowInfo) {
			case found_a_cell:
				if (ulCharPosFirst != CP_INVALID) {
					break;
				}
				ulCharPosFirst = ulGetLong(
						iIndex2 * 4, aucFpage);
				NO_DBG_HEX(ulCharPosFirst);
				tRow.ulCharPosStart = ulCharPosFirst;
				tRow.ulFileOffsetStart =
					ulCharPos2FileOffset(ulCharPosFirst);
				DBG_HEX_C(tRow.ulFileOffsetStart == FC_INVALID,
							ulCharPosFirst);
				break;
			case found_end_of_row:
				ulCharPosLast = ulGetLong(
						iIndex2 * 4, aucFpage);
				NO_DBG_HEX(ulCharPosLast);
				tRow.ulCharPosEnd = ulCharPosLast;
				tRow.ulFileOffsetEnd = ulCharPos2FileOffset(
							ulCharPosLast);
				DBG_HEX_C(tRow.ulFileOffsetEnd == FC_INVALID,
							ulCharPosLast);
				vAdd2RowInfoList(&tRow);
				(void)memset(&tRow, 0, sizeof(tRow));
				ulCharPosFirst = CP_INVALID;
				break;
			case found_nothing:
				break;
			default:
				DBG_DEC(eRowInfo);
				break;
			}
		}
	}
	ausParfPage = xfree(ausParfPage);
} /* end of vGet6PapInfo */

/*
 * Fill the font information block with information
 * from a Word 6/7 file.
 * Returns TRUE when successful, otherwise FALSE
 */
void
vGet6FontInfo(int iFodo, USHORT usIstd,
	const UCHAR *aucGrpprl, int iBytes, font_block_type *pFont)
{
	long	lTmp;
	int	iFodoOff, iInfoLen;
	USHORT	usTmp;
	UCHAR	ucTmp;

	fail(iFodo < 0 || aucGrpprl == NULL || pFont == NULL);

	iFodoOff = 0;
	while (iBytes >= iFodoOff + 1) {
		switch (ucGetByte(iFodo + iFodoOff, aucGrpprl)) {
		case  65:	/* fRMarkDel */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			if (ucTmp == 0) {
				pFont->usFontStyle &= ~FONT_MARKDEL;
			} else {
				pFont->usFontStyle |= FONT_MARKDEL;
			}
			break;
		case  80:	/* cIstd */
			usTmp = usGetWord(iFodo + iFodoOff + 1, aucGrpprl);
			NO_DBG_DEC(usTmp);
                        break;
		case  82:	/* cDefault */
			pFont->usFontStyle &= FONT_HIDDEN;
			pFont->ucFontColor = FONT_COLOR_DEFAULT;
			break;
		case  83:	/* cPlain */
			DBG_MSG("83: cPlain");
			vFillFontFromStylesheet(usIstd, pFont);
			break;
		case  85:	/* fBold */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			switch (ucTmp) {
			case   0:	/* Unset */
				pFont->usFontStyle &= ~FONT_BOLD;
				break;
			case   1:	/* Set */
				pFont->usFontStyle |= FONT_BOLD;
				break;
			case 128:	/* Unchanged */
				break;
			case 129:	/* Negation */
				pFont->usFontStyle ^= FONT_BOLD;
				break;
			default:
				DBG_DEC(ucTmp);
				DBG_FIXME();
				break;
			}
			break;
		case  86:	/* fItalic */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			switch (ucTmp) {
			case   0:	/* Unset */
				pFont->usFontStyle &= ~FONT_ITALIC;
				break;
			case   1:	/* Set */
				pFont->usFontStyle |= FONT_ITALIC;
				break;
			case 128:	/* Unchanged */
				break;
			case 129:	/* Negation */
				pFont->usFontStyle ^= FONT_ITALIC;
				break;
			default:
				DBG_DEC(ucTmp);
				DBG_FIXME();
				break;
			}
			break;
		case  87:	/* fStrike */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			switch (ucTmp) {
			case   0:	/* Unset */
				pFont->usFontStyle &= ~FONT_STRIKE;
				break;
			case   1:	/* Set */
				pFont->usFontStyle |= FONT_STRIKE;
				break;
			case 128:	/* Unchanged */
				break;
			case 129:	/* Negation */
				pFont->usFontStyle ^= FONT_STRIKE;
				break;
			default:
				DBG_DEC(ucTmp);
				DBG_FIXME();
				break;
			}
			break;
		case  90:	/* fSmallCaps */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			switch (ucTmp) {
			case   0:	/* Unset */
				pFont->usFontStyle &= ~FONT_SMALL_CAPITALS;
				break;
			case   1:	/* Set */
				pFont->usFontStyle |= FONT_SMALL_CAPITALS;
				break;
			case 128:	/* Unchanged */
				break;
			case 129:	/* Negation */
				pFont->usFontStyle ^= FONT_SMALL_CAPITALS;
				break;
			default:
				DBG_DEC(ucTmp);
				DBG_FIXME();
				break;
			}
			break;
		case  91:	/* fCaps */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			switch (ucTmp) {
			case   0:	/* Unset */
				pFont->usFontStyle &= ~FONT_CAPITALS;
				break;
			case   1:	/* Set */
				pFont->usFontStyle |= FONT_CAPITALS;
				break;
			case 128:	/* Unchanged */
				break;
			case 129:	/* Negation */
				pFont->usFontStyle ^= FONT_CAPITALS;
				break;
			default:
				DBG_DEC(ucTmp);
				DBG_FIXME();
				break;
			}
			break;
		case  92:	/* fVanish */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			switch (ucTmp) {
			case   0:	/* Unset */
				pFont->usFontStyle &= ~FONT_HIDDEN;
				break;
			case   1:	/* Set */
				pFont->usFontStyle |= FONT_HIDDEN;
				break;
			case 128:	/* Unchanged */
				break;
			case 129:	/* Negation */
				pFont->usFontStyle ^= FONT_HIDDEN;
				break;
			default:
				DBG_DEC(ucTmp);
				DBG_FIXME();
				break;
			}
			break;
		case  93:	/* cFtc */
			usTmp = usGetWord(iFodo + iFodoOff + 1, aucGrpprl);
			if (usTmp <= (USHORT)UCHAR_MAX) {
				pFont->ucFontNumber = (UCHAR)usTmp;
			} else {
				pFont->ucFontNumber = 0;
			}
			break;
		case  94:	/* cKul */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			if (ucTmp == 0 || ucTmp == 5) {
				pFont->usFontStyle &= ~FONT_UNDERLINE;
			} else {
				NO_DBG_MSG("Underline text");
				pFont->usFontStyle |= FONT_UNDERLINE;
				if (ucTmp == 6) {
					DBG_MSG("Bold text");
					pFont->usFontStyle |= FONT_BOLD;
				}
			}
			break;
		case  95:	/* cHps, cHpsPos */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			if (ucTmp != 0) {
				pFont->usFontSize = (USHORT)ucTmp;
			}
			DBG_DEC(ucTmp);
			break;
		case  98:	/* cIco */
			pFont->ucFontColor =
				ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			break;
		case  99:	/* cHps */
			pFont->usFontSize =
				usGetWord(iFodo + iFodoOff + 1, aucGrpprl);
			break;
		case 104:	/* cIss */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			ucTmp &= 0x07;
			if (ucTmp == 1) {
				pFont->usFontStyle |= FONT_SUPERSCRIPT;
				NO_DBG_MSG("Superscript");
			} else if (ucTmp == 2) {
				pFont->usFontStyle |= FONT_SUBSCRIPT;
				NO_DBG_MSG("Subscript");
			}
			break;
		case 106:	/* cHps */
			usTmp = usGetWord(iFodo + iFodoOff + 1, aucGrpprl);
			lTmp = (long)pFont->usFontSize + (long)usTmp;
			if (lTmp < 8) {
				pFont->usFontSize = 8;
			} else if (lTmp > 32766) {
				pFont->usFontSize = 32766;
			} else {
				pFont->usFontSize = (USHORT)lTmp;
			}
			break;
		default:
			break;
		}
		iInfoLen = iGet6InfoLength(iFodo + iFodoOff, aucGrpprl);
		fail(iInfoLen <= 0);
		iFodoOff += iInfoLen;
	}
} /* end of vGet6FontInfo */

/*
 * Fill the picture information block with information
 * from a Word 6/7 file.
 * Returns TRUE when successful, otherwise FALSE
 */
static BOOL
bGet6PicInfo(int iFodo,
	const UCHAR *aucGrpprl, int iBytes, picture_block_type *pPicture)
{
	int	iFodoOff, iInfoLen;
	BOOL	bFound;
	UCHAR	ucTmp;

	fail(iFodo < 0 || aucGrpprl == NULL || pPicture == NULL);

	iFodoOff = 0;
	bFound = FALSE;
	while (iBytes >= iFodoOff + 1) {
		switch (ucGetByte(iFodo + iFodoOff, aucGrpprl)) {
		case  68:	/* fcPic */
			pPicture->ulPictureOffset = ulGetLong(
					iFodo + iFodoOff + 2, aucGrpprl);
			bFound = TRUE;
			break;
		case  71:	/* dttm */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			if (ucTmp == 0x01) {
				/* Not a picture, but a form field */
				return FALSE;
			}
			DBG_DEC_C(ucTmp != 0, ucTmp);
			break;
		case  75:	/* fOle2 */
			ucTmp = ucGetByte(iFodo + iFodoOff + 1, aucGrpprl);
			if (ucTmp == 0x01) {
				/* Not a picture, but an OLE object */
				return FALSE;
			}
			DBG_DEC_C(ucTmp != 0, ucTmp);
			break;
		default:
			break;
		}
		iInfoLen = iGet6InfoLength(iFodo + iFodoOff, aucGrpprl);
		fail(iInfoLen <= 0);
		iFodoOff += iInfoLen;
	}
	return bFound;
} /* end of bGet6PicInfo */

/*
 * Build the lists with Character Information for Word 6/7 files
 */
void
vGet6ChrInfo(FILE *pFile, ULONG ulStartBlock,
	const ULONG *aulBBD, size_t tBBDLen, const UCHAR *aucHeader)
{
	font_block_type		tFont;
	picture_block_type	tPicture;
	USHORT	*ausCharPage;
	UCHAR	*aucBuffer;
	ULONG	ulFileOffset, ulCharPos, ulBeginCharInfo;
	size_t	tCharInfoLen, tOffset, tSize, tLenOld, tLen, tCharPageNum;
	int	iIndex, iIndex2, iRun, iFodo, iLen;
	USHORT	usCharFirstPage, usCount, usIstd;
	UCHAR	aucFpage[BIG_BLOCK_SIZE];

	fail(pFile == NULL || aucHeader == NULL);
	fail(ulStartBlock > MAX_BLOCKNUMBER && ulStartBlock != END_OF_CHAIN);
	fail(aulBBD == NULL);

	ulBeginCharInfo = ulGetLong(0xb8, aucHeader); /* fcPlcfbteChpx */
	NO_DBG_HEX(lBeginCharInfo);
	tCharInfoLen = (size_t)ulGetLong(0xbc, aucHeader); /* lcbPlcfbteChpx */
	NO_DBG_DEC(tCharInfoLen);
	if (tCharInfoLen < 4) {
		DBG_DEC(tCharInfoLen);
		return;
	}

	aucBuffer = xmalloc(tCharInfoLen);
	if (!bReadBuffer(pFile, ulStartBlock,
			aulBBD, tBBDLen, BIG_BLOCK_SIZE,
			aucBuffer, ulBeginCharInfo, tCharInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}

	tLen = (tCharInfoLen - 4) / 6;
	ausCharPage = xcalloc(tLen, sizeof(USHORT));
	for (iIndex = 0, tOffset = (tLen + 1) * 4;
	     iIndex < (int)tLen;
	     iIndex++, tOffset += 2) {
		 ausCharPage[iIndex] = usGetWord(tOffset, aucBuffer);
		 NO_DBG_DEC(ausCharPage[iIndex]);
	}
	DBG_HEX(ulGetLong(0, aucBuffer));
	aucBuffer = xfree(aucBuffer);
	tCharPageNum = (size_t)usGetWord(0x18e, aucHeader); /* cpnBteChp */
	DBG_DEC(tCharPageNum);
	if (tLen < tCharPageNum) {
		/* Replace CharPage by a longer version */
		tLenOld = tLen;
		usCharFirstPage = usGetWord(0x18a, aucHeader); /* pnChrFirst */
		DBG_DEC(usCharFirstPage);
		tLen += tCharPageNum - 1;
		tSize = tLen * sizeof(USHORT);
		ausCharPage = xrealloc(ausCharPage, tSize);
		/* Add new values */
		usCount = usCharFirstPage + 1;
		for (iIndex = (int)tLenOld; iIndex < (int)tLen; iIndex++) {
			ausCharPage[iIndex] = usCount;
			NO_DBG_DEC(ausCharPage[iIndex]);
			usCount++;
		}
	}

	for (iIndex = 0; iIndex < (int)tLen; iIndex++) {
		if (!bReadBuffer(pFile, ulStartBlock,
				aulBBD, tBBDLen, BIG_BLOCK_SIZE,
				aucFpage,
				(ULONG)ausCharPage[iIndex] * BIG_BLOCK_SIZE,
				BIG_BLOCK_SIZE)) {
			break;
		}
		iRun = (int)ucGetByte(0x1ff, aucFpage);
		NO_DBG_DEC(iRun);
		for (iIndex2 = 0; iIndex2 < iRun; iIndex2++) {
		  	ulCharPos = ulGetLong(iIndex2 * 4, aucFpage);
			ulFileOffset = ulCharPos2FileOffset(ulCharPos);
			iFodo = 2 * (int)ucGetByte(
				(iRun + 1) * 4 + iIndex2, aucFpage);

			iLen = (int)ucGetByte(iFodo, aucFpage);

			usIstd = usGetIstd(ulFileOffset);
			vFillFontFromStylesheet(usIstd, &tFont);
			if (iFodo != 0) {
				vGet6FontInfo(iFodo, usIstd,
					aucFpage + 1, iLen - 1, &tFont);
			}
			tFont.ulFileOffset = ulFileOffset;
			vAdd2FontInfoList(&tFont);

			if (iFodo <= 0) {
				continue;
			}

			(void)memset(&tPicture, 0, sizeof(tPicture));
			if (bGet6PicInfo(iFodo, aucFpage + 1,
						iLen - 1, &tPicture)) {
				tPicture.ulFileOffset = ulFileOffset;
				tPicture.ulFileOffsetPicture =
					ulDataPos2FileOffset(
						tPicture.ulPictureOffset);
				vAdd2PictInfoList(&tPicture);
			}
		}
	}
	ausCharPage = xfree(ausCharPage);
} /* end of vGet6ChrInfo */
