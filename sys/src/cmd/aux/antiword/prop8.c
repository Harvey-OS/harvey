/*
 * prop8.c
 * Copyright (C) 1998-2003 A.J. van Os; Released under GPL
 *
 * Description:
 * Read the property information from a MS Word 8, 9 or 10 file
 *
 * Word  8 is better known as Word 97 or as Word 98 for Mac
 * Word  9 is better known as Word 2000 or as Word 2001 for Mac
 * Word 10 is better known as Word 2002 or as Word XP
 */

#include <stdlib.h>
#include <string.h>
#include "antiword.h"

#define DEFAULT_LISTCHAR	0x002e	/* A full stop */


/*
 * iGet8InfoLength - the length of the information for Word 8/9/10 files
 */
static int
iGet8InfoLength(int iByteNbr, const UCHAR *aucGrpprl)
{
	int	iTmp, iDel, iAdd;
	USHORT	usOpCode;

	usOpCode = usGetWord(iByteNbr, aucGrpprl);

	switch (usOpCode & 0xe000) {
	case 0x0000: case 0x2000:
		return 3;
	case 0x4000: case 0x8000: case 0xa000:
		return 4;
	case 0xe000:
		return 5;
	case 0x6000:
		return 6;
	case 0xc000:
		iTmp = (int)ucGetByte(iByteNbr + 2, aucGrpprl);
		if (usOpCode == 0xc615 && iTmp == 255) {
			iDel = (int)ucGetByte(iByteNbr + 3, aucGrpprl);
			iAdd = (int)ucGetByte(
					iByteNbr + 4 + iDel * 4, aucGrpprl);
			iTmp = 2 + iDel * 4 + iAdd * 3;
		}
		return 3 + iTmp;
	default:
		DBG_HEX(usOpCode);
		DBG_FIXME();
		return 1;
	}
} /* end of iGet8InfoLength */

/*
 * Fill the section information block with information
 * from a Word 8/9/10 file.
 */
static void
vGet8SectionInfo(const UCHAR *aucGrpprl, size_t tBytes,
		section_block_type *pSection)
{
	UINT	uiIndex;
	int	iFodoOff, iInfoLen, iSize, iTmp;
	USHORT	usCcol;
	UCHAR	ucTmp;

	fail(aucGrpprl == NULL || pSection == NULL);

	iFodoOff = 0;
	while (tBytes >= (size_t)iFodoOff + 2) {
		iInfoLen = 0;
		switch (usGetWord(iFodoOff, aucGrpprl)) {
		case 0x3009:	/* bkc */
			ucTmp = ucGetByte(iFodoOff + 2, aucGrpprl);
			DBG_DEC(ucTmp);
			pSection->bNewPage = ucTmp != 0 && ucTmp != 1;
			break;
		case 0x500b:	/* ccolM1 */
			usCcol = 1 + usGetWord(iFodoOff + 2, aucGrpprl);
			DBG_DEC(usCcol);
			break;
		case 0xd202:	/* olstAnm */
			iSize = (int)ucGetByte(iFodoOff + 2, aucGrpprl);
			DBG_DEC_C(iSize != 212, iSize);
			for (uiIndex = 0, iTmp = iFodoOff + 3;
			     uiIndex < 9 && iTmp < iFodoOff + 3 + iSize - 15;
			     uiIndex++, iTmp += 16) {
				pSection->aucNFC[uiIndex] =
						ucGetByte(iTmp, aucGrpprl);
				DBG_DEC(pSection->aucNFC[uiIndex]);
				ucTmp = ucGetByte(iTmp + 3, aucGrpprl);
				DBG_HEX(ucTmp);
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
		default:
			break;
		}
		if (iInfoLen <= 0) {
			iInfoLen = iGet8InfoLength(iFodoOff, aucGrpprl);
			fail(iInfoLen <= 0);
		}
		iFodoOff += iInfoLen;
	}
} /* end of vGet8SectionInfo */

/*
 * Build the lists with Section Property Information for Word 8/9/10 files
 */
void
vGet8SepInfo(FILE *pFile, const pps_info_type *pPPS,
	const ULONG *aulBBD, size_t tBBDLen,
	const ULONG *aulSBD, size_t tSBDLen,
	const UCHAR *aucHeader)
{
	section_block_type	tSection;
	ULONG		*aulSectPage, *aulTextOffset;
	const ULONG	*aulBlockDepot;
	UCHAR	*aucBuffer, *aucFpage;
	ULONG	ulBeginSectInfo;
	ULONG	ulTableSize, ulTableStartBlock;
	size_t	tSectInfoLen, tBlockDepotLen;
	size_t	tBlockSize, tOffset, tLen, tBytes;
	int	iIndex;
	USHORT	usDocStatus;
	UCHAR	aucTmp[2];

	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(aulBBD == NULL || aulSBD == NULL);

	ulBeginSectInfo = ulGetLong(0xca, aucHeader); /* fcPlcfsed */
	NO_DBG_HEX(ulBeginSectInfo);
	tSectInfoLen = (size_t)ulGetLong(0xce, aucHeader); /* lcbPlcfsed */
	NO_DBG_DEC(tSectInfoLen);
	if (tSectInfoLen < 4) {
		DBG_DEC(tSectInfoLen);
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
	DBG_DEC(ulTableStartBlock);
	if (ulTableStartBlock == 0) {
		DBG_MSG("No section information");
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
	aucBuffer = xmalloc(tSectInfoLen);
	if (!bReadBuffer(pFile, ulTableStartBlock,
			aulBlockDepot, tBlockDepotLen, tBlockSize,
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
		if (!bReadBuffer(pFile, pPPS->tWordDocument.ulSB,
				aulBBD, tBBDLen, BIG_BLOCK_SIZE,
				aucTmp, aulSectPage[iIndex], 2)) {
			continue;
		}
		tBytes = 2 + (size_t)usGetWord(0, aucTmp);
		NO_DBG_DEC(tBytes);
		/* Read the bytes */
		aucFpage = xmalloc(tBytes);
		if (!bReadBuffer(pFile, pPPS->tWordDocument.ulSB,
				aulBBD, tBBDLen, BIG_BLOCK_SIZE,
				aucFpage, aulSectPage[iIndex], tBytes)) {
			aucFpage = xfree(aucFpage);
			continue;
		}
		NO_DBG_PRINT_BLOCK(aucFpage, tBytes);
		/* Process the bytes */
		vGetDefaultSection(&tSection);
		vGet8SectionInfo(aucFpage + 2, tBytes - 2, &tSection);
		vAdd2SectionInfoList(&tSection, aulTextOffset[iIndex]);
		aucFpage = xfree(aucFpage);
	}
	aulTextOffset = xfree(aulTextOffset);
	aulSectPage = xfree(aulSectPage);
} /* end of vGet8SepInfo */

/*
 * Translate the rowinfo to a member of the row_info enumeration
 */
row_info_enum
eGet8RowInfo(int iFodo,
	const UCHAR *aucGrpprl, int iBytes, row_block_type *pRow)
{
	int	iFodoOff, iInfoLen;
	int	iIndex, iSize, iCol;
	int	iPosCurr, iPosPrev;
	USHORT	usTmp;
	BOOL	bFound2416_0, bFound2416_1, bFound2417_0, bFound2417_1;
	BOOL	bFoundd608;

	fail(iFodo < 0 || aucGrpprl == NULL || pRow == NULL);

	iFodoOff = 0;
	bFound2416_0 = FALSE;
	bFound2416_1 = FALSE;
	bFound2417_0 = FALSE;
	bFound2417_1 = FALSE;
	bFoundd608 = FALSE;
	while (iBytes >= iFodoOff + 2) {
		iInfoLen = 0;
		switch (usGetWord(iFodo + iFodoOff, aucGrpprl)) {
		case 0x2416:	/* fIntable */
			if (odd(ucGetByte(iFodo + iFodoOff + 2, aucGrpprl))) {
				bFound2416_1 = TRUE;
			} else {
				bFound2416_0 = TRUE;
			}
			break;
		case 0x2417:	/* fTtp */
			if (odd(ucGetByte(iFodo + iFodoOff + 2, aucGrpprl))) {
				bFound2417_1 = TRUE;
			} else {
				bFound2417_0 = TRUE;
			}
			break;
		case 0x6424:	/* brcTop */
			usTmp = usGetWord(iFodo + iFodoOff + 2, aucGrpprl);
			usTmp &= 0xff00;
			NO_DBG_DEC(usTmp >> 8);
			if (usTmp == 0) {
				pRow->ucBorderInfo &= ~TABLE_BORDER_TOP;
			} else {
				pRow->ucBorderInfo |= TABLE_BORDER_TOP;
			}
			break;
		case 0x6425:	/* brcLeft */
			usTmp = usGetWord(iFodo + iFodoOff + 2, aucGrpprl);
			usTmp &= 0xff00;
			NO_DBG_DEC(usTmp >> 8);
			if (usTmp == 0) {
				pRow->ucBorderInfo &= ~TABLE_BORDER_LEFT;
			} else {
				pRow->ucBorderInfo |= TABLE_BORDER_LEFT;
			}
			break;
		case 0x6426:	/* brcBottom */
			usTmp = usGetWord(iFodo + iFodoOff + 2, aucGrpprl);
			usTmp &= 0xff00;
			NO_DBG_DEC(usTmp >> 8);
			if (usTmp == 0) {
				pRow->ucBorderInfo &= ~TABLE_BORDER_BOTTOM;
			} else {
				pRow->ucBorderInfo |= TABLE_BORDER_BOTTOM;
			}
			break;
		case 0x6427:	/* brcRight */
			usTmp = usGetWord(iFodo + iFodoOff + 2, aucGrpprl);
			usTmp &= 0xff00;
			NO_DBG_DEC(usTmp >> 8);
			if (usTmp == 0) {
				pRow->ucBorderInfo &= ~TABLE_BORDER_RIGHT;
			} else {
				pRow->ucBorderInfo |= TABLE_BORDER_RIGHT;
			}
			break;
		case 0xd608:	/* cDefTable */
			iSize = (int)usGetWord(iFodo + iFodoOff + 2, aucGrpprl);
			if (iSize < 6 || iBytes < iFodoOff + 8) {
				DBG_DEC(iSize);
				DBG_DEC(iFodoOff);
				iInfoLen = 2;
				break;
			}
			iCol = (int)ucGetByte(iFodo + iFodoOff + 4, aucGrpprl);
			if (iCol < 1 ||
			    iBytes < iFodoOff + 4 + (iCol + 1) * 2) {
				DBG_DEC(iCol);
				DBG_DEC(iFodoOff);
				iInfoLen = 2;
				break;
			}
			if (iCol >= (int)elementsof(pRow->asColumnWidth)) {
				DBG_DEC(iCol);
				werr(1, "The number of columns is corrupt");
			}
			pRow->ucNumberOfColumns = (UCHAR)iCol;
			pRow->iColumnWidthSum = 0;
			iPosPrev = (int)(short)usGetWord(
					iFodo + iFodoOff + 5,
					aucGrpprl);
			for (iIndex = 0; iIndex < iCol; iIndex++) {
				iPosCurr = (int)(short)usGetWord(
					iFodo + iFodoOff + 7 + iIndex * 2,
					aucGrpprl);
				pRow->asColumnWidth[iIndex] =
						(short)(iPosCurr - iPosPrev);
				pRow->iColumnWidthSum +=
					pRow->asColumnWidth[iIndex];
				iPosPrev = iPosCurr;
			}
			bFoundd608 = TRUE;
			break;
		default:
			break;
		}
		if (iInfoLen <= 0) {
			iInfoLen =
				iGet8InfoLength(iFodo + iFodoOff, aucGrpprl);
			fail(iInfoLen <= 0);
		}
		iFodoOff += iInfoLen;
	}
	if (bFound2416_1 && bFound2417_1 && bFoundd608) {
		return found_end_of_row;
	}
	if (bFound2416_0 && bFound2417_0 && !bFoundd608) {
		return found_not_end_of_row;
	}
	if (bFound2416_1) {
		return found_a_cell;
	}
	if (bFound2416_0) {
		return found_not_a_cell;
	}
	return found_nothing;
} /* end of eGet8RowInfo */

/*
 * Fill the style information block with information
 * from a Word 8/9/10 file.
 */
void
vGet8StyleInfo(int iFodo,
	const UCHAR *aucGrpprl, int iBytes, style_block_type *pStyle)
{
	list_block_type	tList6;
	const list_block_type	*pList;
	int	iFodoOff, iInfoLen;
	int	iTmp, iDel, iAdd, iBefore;
	USHORT	usOpCode, usTmp;
	short	sTmp;

	fail(iFodo < 0 || aucGrpprl == NULL || pStyle == NULL);

	NO_DBG_DEC_C(pStyle->usListIndex != 0, pStyle->usIstd);
	NO_DBG_DEC_C(pStyle->usListIndex != 0, pStyle->usListIndex);

	(void)memset(&tList6, 0, sizeof(tList6));

	iFodoOff = 0;
	while (iBytes >= iFodoOff + 2) {
		iInfoLen = 0;
		usOpCode = usGetWord(iFodo + iFodoOff, aucGrpprl);
		switch (usOpCode) {
		case 0x2403:	/* jc */
			pStyle->ucAlignment = ucGetByte(
					iFodo + iFodoOff + 2, aucGrpprl);
			break;
		case 0x260a:	/* ilvl */
			pStyle->ucListLevel =
				ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
			NO_DBG_DEC(pStyle->ucListLevel);
			pStyle->ucNumLevel = pStyle->ucListLevel;
			break;
		case 0x4600:	/* istd */
			usTmp = usGetWord(iFodo + iFodoOff + 2, aucGrpprl);
			NO_DBG_DEC(usTmp);
			break;
		case 0x460b:	/* ilfo */
			pStyle->usListIndex =
				usGetWord(iFodo + iFodoOff + 2, aucGrpprl);
			NO_DBG_DEC(pStyle->usListIndex);
			break;
		case 0x4610: /* Nest dxaLeft */
			sTmp = (short)usGetWord(
					iFodo + iFodoOff + 2, aucGrpprl);
			pStyle->sLeftIndent += sTmp;
			if (pStyle->sLeftIndent < 0) {
				pStyle->sLeftIndent = 0;
			}
			DBG_DEC(sTmp);
			DBG_DEC(pStyle->sLeftIndent);
			break;
		case 0x6c0d:	/* ChgTabsPapx */
			iTmp = (int)ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
			if (iTmp < 2) {
				iInfoLen = 1;
				break;
			}
			DBG_DEC(iTmp);
			iDel = (int)ucGetByte(iFodo + iFodoOff + 3, aucGrpprl);
			if (iTmp < 2 + 2 * iDel) {
				iInfoLen = 1;
				break;
			}
			DBG_DEC(iDel);
			iAdd = (int)ucGetByte(
				iFodo + iFodoOff + 4 + 2 * iDel, aucGrpprl);
			if (iTmp < 2 + 2 * iDel + 2 * iAdd) {
				iInfoLen = 1;
				break;
			}
			DBG_DEC(iAdd);
			break;
		case 0x840e:	/* dxaRight */
			pStyle->sRightIndent = (short)usGetWord(
					iFodo + iFodoOff + 2, aucGrpprl);
			NO_DBG_DEC(pStyle->sRightIndent);
			break;
		case 0x840f:	/* dxaLeft */
			pStyle->sLeftIndent = (short)usGetWord(
					iFodo + iFodoOff + 2, aucGrpprl);
			NO_DBG_DEC(pStyle->sLeftIndent);
			break;
		case 0x8411:	/* dxaLeft1 */
			pStyle->sLeftIndent1 = (short)usGetWord(
					iFodo + iFodoOff + 2, aucGrpprl);
			NO_DBG_DEC(pStyle->sLeftIndent1);
			break;
		case 0xa413:	/* dyaBefore */
			pStyle->usBeforeIndent = usGetWord(
					iFodo + iFodoOff + 2, aucGrpprl);
			NO_DBG_DEC(pStyle->usBeforeIndent);
			break;
		case 0xa414:	/* dyaAfter */
			pStyle->usAfterIndent = usGetWord(
					iFodo + iFodoOff + 2, aucGrpprl);
			NO_DBG_DEC(pStyle->usAfterIndent);
			break;
		case 0xc63e:	/* anld */
			iTmp = (int)ucGetByte(
					iFodo + iFodoOff + 2, aucGrpprl);
			DBG_DEC_C(iTmp < 84, iTmp);
			if (iTmp >= 1) {
				tList6.ucNFC = ucGetByte(
					iFodo + iFodoOff + 3, aucGrpprl);
			}
			if (tList6.ucNFC != LIST_BULLETS && iTmp >= 2) {
				iBefore = (int)ucGetByte(
					iFodo + iFodoOff + 4, aucGrpprl);
			} else {
				iBefore = 0;
			}
			if (iTmp >= 12) {
				tList6.ulStartAt = (ULONG)usGetWord(
					iFodo + iFodoOff + 13, aucGrpprl);
			}
			if (iTmp >= iBefore + 22) {
				tList6.usListChar = usGetWord(
					iFodo + iFodoOff + iBefore + 23,
					aucGrpprl);
				DBG_HEX(tList6.usListChar);
			}
			break;
		default:
			NO_DBG_HEX(usOpCode);
			break;
		}
		if (iInfoLen <= 0) {
			iInfoLen =
				iGet8InfoLength(iFodo + iFodoOff, aucGrpprl);
			fail(iInfoLen <= 0);
		}
		iFodoOff += iInfoLen;
	}

	if (pStyle->usListIndex == 2047) {
		/* Old style list */
		pStyle->usStartAt = (USHORT)tList6.ulStartAt;
		pStyle->usListChar = tList6.usListChar;
		pStyle->ucNFC = tList6.ucNFC;
	} else {
		/* New style list */
		pList = pGetListInfo(pStyle->usListIndex, pStyle->ucListLevel);
		if (pList != NULL) {
			pStyle->bNoRestart = pList->bNoRestart;
			fail(pList->ulStartAt > (ULONG)USHRT_MAX);
			pStyle->usStartAt = (USHORT)pList->ulStartAt;
			pStyle->usListChar = pList->usListChar;
			pStyle->ucNFC = pList->ucNFC;
			if (pStyle->sLeftIndent <= 0) {
				pStyle->sLeftIndent = pList->sLeftIndent;
			}
		}
	}
} /* end of vGet8StyleInfo */

/*
 * Get the left indentation value from the style information block
 *
 * Returns the value when found, otherwise 0
 */
static short
sGetLeftIndent(const UCHAR *aucGrpprl, size_t tBytes)
{
	int	iOffset, iInfoLen;
	USHORT	usOpCode, usTmp;

	fail(aucGrpprl == NULL);

	iOffset = 0;
	while (tBytes >= (size_t)iOffset + 4) {
		usOpCode = usGetWord(iOffset, aucGrpprl);
		if (usOpCode == 0x840f) {	/* dxaLeft */
			usTmp = usGetWord(iOffset + 2, aucGrpprl);
			if (usTmp <= 0x7fff) {
				NO_DBG_DEC(usTmp);
				return (short)usTmp;
			}
		}
		iInfoLen = iGet8InfoLength(iOffset, aucGrpprl);
		fail(iInfoLen <= 0);
		iOffset += iInfoLen;
	}
	return 0;
} /* end of sGetLeftIndent */

/*
 * Build the list with List Information for Word 8/9/10 files
 */
void
vGet8LstInfo(FILE *pFile, const pps_info_type *pPPS,
	const ULONG *aulBBD, size_t tBBDLen,
	const ULONG *aulSBD, size_t tSBDLen,
	const UCHAR *aucHeader)
{
	list_block_type	tList;
	const ULONG	*aulBlockDepot;
	UCHAR	*aucLfoInfo, *aucLstfInfo, *aucPapx, *aucXString;
	ULONG	ulTableStartBlock, ulTableSize;
	ULONG	ulBeginLfoInfo, ulBeginLstfInfo, ulBeginLvlfInfo;
	ULONG	ulListID, ulStart;
	size_t	tBlockDepotLen, tBlockSize;
	size_t	tLfoInfoLen, tLstfInfoLen, tPapxLen, tXstLen, tOff;
	size_t	tLstfRecords, tStart, tIndex;
	int	iNums;
	USHORT	usDocStatus, usIstd;
	UCHAR	ucTmp, ucListLevel, ucMaxLevel, ucChpxLen;
	UCHAR	aucLvlfInfo[28], aucXst[2];

	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(aulBBD == NULL || aulSBD == NULL);

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
		DBG_MSG("No list information");
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

	/* LFO (List Format Override) */
	ulBeginLfoInfo = ulGetLong(0x2ea, aucHeader); /* fcPlfLfo */
	DBG_HEX(ulBeginLfoInfo);
	tLfoInfoLen = (size_t)ulGetLong(0x2ee, aucHeader); /* lcbPlfLfo */
	DBG_DEC(tLfoInfoLen);
	if (tLfoInfoLen == 0) {
		DBG_MSG("No lists in this document");
		return;
	}

	aucLfoInfo = xmalloc(tLfoInfoLen);
	if (!bReadBuffer(pFile, ulTableStartBlock,
			aulBlockDepot, tBlockDepotLen, tBlockSize,
			aucLfoInfo, ulBeginLfoInfo, tLfoInfoLen)) {
		aucLfoInfo = xfree(aucLfoInfo);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucLfoInfo, tLfoInfoLen);
	vBuildLfoList(aucLfoInfo, tLfoInfoLen);
	aucLfoInfo = xfree(aucLfoInfo);

	/* LSTF (LiST data on File) */
	ulBeginLstfInfo = ulGetLong(0x2e2, aucHeader); /* fcPlcfLst */
	DBG_HEX(ulBeginLstfInfo);
	tLstfInfoLen = (size_t)ulGetLong(0x2e6, aucHeader); /* lcbPlcfLst */
	DBG_DEC(tLstfInfoLen);
	if (tLstfInfoLen == 0) {
		DBG_MSG("No list data on file");
		return;
	}

	aucLstfInfo = xmalloc(tLstfInfoLen);
	if (!bReadBuffer(pFile, ulTableStartBlock,
			aulBlockDepot, tBlockDepotLen, tBlockSize,
			aucLstfInfo, ulBeginLstfInfo, tLstfInfoLen)) {
		aucLstfInfo = xfree(aucLstfInfo);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucLstfInfo, tLstfInfoLen);

	tLstfRecords = (size_t)usGetWord(0, aucLstfInfo);
	if (2 + tLstfRecords * 28 < tLstfInfoLen) {
		DBG_DEC(2 + tLstfRecords * 28);
		DBG_DEC(tLstfInfoLen);
		aucLstfInfo = xfree(aucLstfInfo);
		return;
	}

	/* LVLF (List leVeL on File) */
	ulBeginLvlfInfo = ulBeginLstfInfo + tLstfInfoLen;
	DBG_HEX(ulBeginLvlfInfo);

	aucXString = NULL;
	ulStart = ulBeginLvlfInfo;

	for (tIndex = 0, tStart = 2;
	     tIndex < tLstfRecords;
	     tIndex++, tStart += 28) {
		ulListID = ulGetLong(tStart, aucLstfInfo);
		NO_DBG_HEX(ulListID);
		ucTmp = ucGetByte(tStart + 26, aucLstfInfo);
		ucMaxLevel = odd(ucTmp) ? 1 : 9;
		for (ucListLevel = 0; ucListLevel < ucMaxLevel; ucListLevel++) {
			fail(aucXString != NULL);
			usIstd = usGetWord(
					tStart + 8 + 2 * (size_t)ucListLevel,
					aucLstfInfo);
			DBG_DEC_C(usIstd != STI_NIL, usIstd);
			NO_DBG_HEX(ulStart);
			(void)memset(&tList, 0, sizeof(tList));
			/* Read the lvlf (List leVeL on File) */
			if (!bReadBuffer(pFile, ulTableStartBlock,
					aulBlockDepot, tBlockDepotLen,
					tBlockSize, aucLvlfInfo,
					ulStart, sizeof(aucLvlfInfo))) {
				aucLstfInfo = xfree(aucLstfInfo);
				return;
			}
			NO_DBG_PRINT_BLOCK(aucLvlfInfo, sizeof(aucLvlfInfo));
			if (bAllZero(aucLvlfInfo, sizeof(aucLvlfInfo))) {
				tList.ulStartAt = 1;
				tList.ucNFC = 0x00;
				tList.bNoRestart = FALSE;
			} else {
				tList.ulStartAt = ulGetLong(0, aucLvlfInfo);
				tList.ucNFC = ucGetByte(4, aucLvlfInfo);
				ucTmp = ucGetByte(5, aucLvlfInfo);
				tList.bNoRestart = (ucTmp & BIT(3)) != 0;
			}
			ulStart += sizeof(aucLvlfInfo);
			tPapxLen = (size_t)ucGetByte(25, aucLvlfInfo);
			if (tPapxLen != 0) {
				aucPapx = xmalloc(tPapxLen);
				/* Read the Papx */
				if (!bReadBuffer(pFile, ulTableStartBlock,
						aulBlockDepot, tBlockDepotLen,
						tBlockSize, aucPapx,
						ulStart, tPapxLen)) {
					aucPapx = xfree(aucPapx);
					aucLstfInfo = xfree(aucLstfInfo);
					return;
				}
				NO_DBG_PRINT_BLOCK(aucPapx, tPapxLen);
				tList.sLeftIndent =
					sGetLeftIndent(aucPapx, tPapxLen);
				aucPapx = xfree(aucPapx);
			}
			ulStart += tPapxLen;
			ucChpxLen = ucGetByte(24, aucLvlfInfo);
			ulStart += (ULONG)ucChpxLen;
			/* Read the length of the XString */
			if (!bReadBuffer(pFile, ulTableStartBlock,
					aulBlockDepot, tBlockDepotLen,
					tBlockSize, aucXst,
					ulStart, sizeof(aucXst))) {
				aucLstfInfo = xfree(aucLstfInfo);
				return;
			}
			NO_DBG_PRINT_BLOCK(aucXst, sizeof(aucXst));
			tXstLen = (size_t)usGetWord(0, aucXst);
			ulStart += sizeof(aucXst);
			if (tXstLen == 0) {
				tList.usListChar = DEFAULT_LISTCHAR;
				vAdd2ListInfoList(ulListID,
						usIstd,
						ucListLevel,
						&tList);
				continue;
			}
			tXstLen *= 2;	/* Length in chars to length in bytes */
			aucXString = xmalloc(tXstLen);
			/* Read the XString */
			if (!bReadBuffer(pFile, ulTableStartBlock,
					aulBlockDepot, tBlockDepotLen,
					tBlockSize, aucXString,
					ulStart, tXstLen)) {
				aucXString = xfree(aucXString);
				aucLstfInfo = xfree(aucLstfInfo);
				return;
			}
			NO_DBG_PRINT_BLOCK(aucXString, tXstLen);
			tOff = 0;
			for (iNums = 6; iNums < 15; iNums++) {
				ucTmp = ucGetByte(iNums, aucLvlfInfo);
				if (ucTmp == 0) {
					break;
				}
				tOff = (size_t)ucTmp;
			}
			tOff *= 2;	/* Offset in chars to offset in bytes */
			NO_DBG_DEC(tOff);
			if (tList.ucNFC == LIST_SPECIAL ||
			    tList.ucNFC == LIST_BULLETS) {
				tList.usListChar = usGetWord(0, aucXString);
			} else if (tOff != 0 && tOff < tXstLen) {
				tList.usListChar = usGetWord(tOff, aucXString);
			} else {
				tList.usListChar = DEFAULT_LISTCHAR;
			}
			vAdd2ListInfoList(ulListID,
					usIstd,
					ucListLevel,
					&tList);
			ulStart += tXstLen;
			aucXString = xfree(aucXString);
		}
	}
	aucLstfInfo = xfree(aucLstfInfo);
} /* end of vGet8LstInfo */

/*
 * Build the lists with Paragraph Information for Word 8/9/10 files
 */
void
vGet8PapInfo(FILE *pFile, const pps_info_type *pPPS,
	const ULONG *aulBBD, size_t tBBDLen,
	const ULONG *aulSBD, size_t tSBDLen,
	const UCHAR *aucHeader)
{
	row_block_type		tRow;
	style_block_type	tStyle;
	ULONG		*aulParfPage;
	const ULONG	*aulBlockDepot;
	UCHAR	*aucBuffer;
	ULONG	ulCharPos, ulCharPosFirst, ulCharPosLast;
	ULONG	ulBeginParfInfo;
	ULONG	ulTableSize, ulTableStartBlock;
	size_t	tParfInfoLen, tBlockDepotLen;
	size_t	tBlockSize, tOffset, tLen;
	int	iIndex, iIndex2, iRun, iFodo, iLen;
	row_info_enum	eRowInfo;
	USHORT	usDocStatus, usIstd;
	UCHAR	aucFpage[BIG_BLOCK_SIZE];

	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(aulBBD == NULL || aulSBD == NULL);

	ulBeginParfInfo = ulGetLong(0x102, aucHeader); /* fcPlcfbtePapx */
	NO_DBG_HEX(ulBeginParfInfo);
	tParfInfoLen = (size_t)ulGetLong(0x106, aucHeader); /* lcbPlcfbtePapx */
	NO_DBG_DEC(tParfInfoLen);
	if (tParfInfoLen < 4) {
		DBG_DEC(tParfInfoLen);
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
	DBG_DEC(ulTableStartBlock);
	if (ulTableStartBlock == 0) {
		DBG_MSG("No paragraph information");
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

	aucBuffer = xmalloc(tParfInfoLen);
	if (!bReadBuffer(pFile, ulTableStartBlock,
			aulBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, ulBeginParfInfo, tParfInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tParfInfoLen);

	tLen = (tParfInfoLen / 4 - 1) / 2;
	aulParfPage = xcalloc(tLen, sizeof(ULONG));
	for (iIndex = 0, tOffset = (tLen + 1) * 4;
	     iIndex < (int)tLen;
	     iIndex++, tOffset += 4) {
		 aulParfPage[iIndex] = ulGetLong(tOffset, aucBuffer);
		 NO_DBG_DEC(aulParfPage[iIndex]);
	}
	DBG_HEX(ulGetLong(0, aucBuffer));
	aucBuffer = xfree(aucBuffer);
	NO_DBG_PRINT_BLOCK(aucHeader, HEADER_SIZE);

	(void)memset(&tRow, 0, sizeof(tRow));
	ulCharPosFirst = CP_INVALID;
	for (iIndex = 0; iIndex < (int)tLen; iIndex++) {
		fail(aulParfPage[iIndex] > ULONG_MAX / BIG_BLOCK_SIZE);
		if (!bReadBuffer(pFile, pPPS->tWordDocument.ulSB,
				aulBBD, tBBDLen, BIG_BLOCK_SIZE,
				aucFpage,
				aulParfPage[iIndex] * BIG_BLOCK_SIZE,
				BIG_BLOCK_SIZE)) {
			break;
		}
		NO_DBG_PRINT_BLOCK(aucFpage, BIG_BLOCK_SIZE);
		iRun = (int)ucGetByte(0x1ff, aucFpage);
		NO_DBG_DEC(iRun);
		for (iIndex2 = 0; iIndex2 < iRun; iIndex2++) {
			NO_DBG_HEX(ulGetLong(iIndex2 * 4, aucFpage));
			iFodo = 2 * (int)ucGetByte(
				(iRun + 1) * 4 + iIndex2 * 13, aucFpage);
			if (iFodo <= 0) {
				continue;
			}

			iLen = 2 * (int)ucGetByte(iFodo, aucFpage);
			if (iLen == 0) {
				iFodo++;
				iLen = 2 * (int)ucGetByte(iFodo, aucFpage);
			}

			usIstd = usGetWord(iFodo + 1, aucFpage);
			vFillStyleFromStylesheet(usIstd, &tStyle);
			vGet8StyleInfo(iFodo, aucFpage + 3, iLen - 3, &tStyle);
			ulCharPos = ulGetLong(iIndex2 * 4, aucFpage);
			NO_DBG_HEX(ulCharPos);
			tStyle.ulFileOffset = ulCharPos2FileOffset(ulCharPos);
			vAdd2StyleInfoList(&tStyle);

			eRowInfo = eGet8RowInfo(iFodo,
					aucFpage + 3, iLen - 3, &tRow);
			switch (eRowInfo) {
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
				NO_DBG_HEX_C(
					tRow.ulFileOffsetStart == FC_INVALID,
					ulCharPosFirst);
				break;
			case found_end_of_row:
				ulCharPosLast = ulGetLong(
						iIndex2 * 4, aucFpage);
				NO_DBG_HEX(ulCharPosLast);
				tRow.ulCharPosEnd = ulCharPosLast;
				tRow.ulFileOffsetEnd =
					ulCharPos2FileOffset(ulCharPosLast);
				NO_DBG_HEX_C(tRow.ulFileOffsetEnd == FC_INVALID,
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
	aulParfPage = xfree(aulParfPage);
} /* end of vGet8PapInfo */

/*
 * Fill the font information block with information
 * from a Word 8/9/10 file.
 */
void
vGet8FontInfo(int iFodo, USHORT usIstd,
	const UCHAR *aucGrpprl, int iBytes, font_block_type *pFont)
{
	long	lTmp;
	int	iFodoOff, iInfoLen;
	USHORT	usTmp;
	UCHAR	ucTmp;

	fail(iFodo < 0 || aucGrpprl == NULL || pFont == NULL);

	iFodoOff = 0;
	while (iBytes >= iFodoOff + 2) {
		switch (usGetWord(iFodo + iFodoOff, aucGrpprl)) {
		case 0x0800:	/* fRMarkDel */
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
			if (ucTmp == 0) {
				pFont->usFontStyle &= ~FONT_MARKDEL;
			} else {
				pFont->usFontStyle |= FONT_MARKDEL;
			}
			break;
		case 0x0835:	/* fBold */
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
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
		case 0x0836:	/* fItalic */
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
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
		case 0x0837:	/* fStrike */
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
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
		case 0x083a:	/* fSmallCaps */
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
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
		case 0x083b:	/* fCaps */
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
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
		case 0x083c:	/* fVanish */
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
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
		case 0x2a32:	/* cDefault */
			pFont->usFontStyle &= FONT_HIDDEN;
			pFont->ucFontColor = FONT_COLOR_DEFAULT;
			break;
		case 0x2a33:	/* cPlain */
			DBG_MSG("2a33: cPlain");
			vFillFontFromStylesheet(usIstd, pFont);
			break;
		case 0x2a3e:	/* cKul */
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
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
		case 0x2a42:	/* cIco */
			pFont->ucFontColor =
				ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
			NO_DBG_DEC(pFont->ucFontColor);
			break;
		case 0x2a48:	/* cIss */
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
			ucTmp &= 0x07;
			if (ucTmp == 1) {
				pFont->usFontStyle |= FONT_SUPERSCRIPT;
				NO_DBG_MSG("Superscript");
			} else if (ucTmp == 2) {
				pFont->usFontStyle |= FONT_SUBSCRIPT;
				NO_DBG_MSG("Subscript");
			}
			break;
		case 0x4a30:	/* cIstd */
			usTmp = usGetWord(iFodo + iFodoOff + 2, aucGrpprl);
			NO_DBG_DEC(usTmp);
			break;
		case 0x4a43:	/* cHps */
			pFont->usFontSize =
				usGetWord(iFodo + iFodoOff + 2, aucGrpprl);
			NO_DBG_DEC(pFont->usFontSize);
			break;
		case 0x4a51:	/* cFtc */
			usTmp = usGetWord(iFodo + iFodoOff + 2, aucGrpprl);
			if (usTmp <= (USHORT)UCHAR_MAX) {
				pFont->ucFontNumber = (UCHAR)usTmp;
			} else {
				pFont->ucFontNumber = 0;
			}
			break;
		case 0xca4a:	/* cHps */
			usTmp = usGetWord(iFodo + iFodoOff + 2, aucGrpprl);
			lTmp = (long)pFont->usFontSize + (long)usTmp;
			if (lTmp < 8) {
				pFont->usFontSize = 8;
			} else if (lTmp > 32766) {
				pFont->usFontSize = 32766;
			} else {
				pFont->usFontSize = (USHORT)lTmp;
			}
			break;
		case 0xea3f:	/* cHps, cHpsPos */
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
			if (ucTmp != 0) {
				pFont->usFontSize = (USHORT)ucTmp;
			}
			DBG_DEC(ucTmp);
			break;
		default:
			break;
		}
		iInfoLen = iGet8InfoLength(iFodo + iFodoOff, aucGrpprl);
		fail(iInfoLen <= 0);
		iFodoOff += iInfoLen;
	}
} /* end of vGet8FontInfo */

/*
 * Fill the picture information block with information
 * from a Word 8/9/10 file.
 * Returns TRUE when successful, otherwise FALSE
 */
static BOOL
bGet8PicInfo(int iFodo,
	const UCHAR *aucGrpprl, int iBytes, picture_block_type *pPicture)
{
	int	iFodoOff, iInfoLen;
	BOOL	bFound;
	UCHAR	ucTmp;

	fail(iFodo <= 0 || aucGrpprl == NULL || pPicture == NULL);

	iFodoOff = 0;
	bFound = FALSE;
	while (iBytes >= iFodoOff + 2) {
		switch (usGetWord(iFodo + iFodoOff, aucGrpprl)) {
		case 0x0806:	/* fData */
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
			if (ucTmp == 0x01) {
				/* Not a picture, but a form field */
				return FALSE;
			}
			DBG_DEC_C(ucTmp != 0, ucTmp);
			break;
		case 0x080a:	/* fOle2 */
			ucTmp = ucGetByte(iFodo + iFodoOff + 2, aucGrpprl);
			if (ucTmp == 0x01) {
				/* Not a picture, but an OLE object */
				return FALSE;
			}
			DBG_DEC_C(ucTmp != 0, ucTmp);
			break;
		case 0x6a03:	/* fcPic */
			pPicture->ulPictureOffset = ulGetLong(
					iFodo + iFodoOff + 2, aucGrpprl);
			bFound = TRUE;
			break;
		default:
			break;
		}
		iInfoLen = iGet8InfoLength(iFodo + iFodoOff, aucGrpprl);
		fail(iInfoLen <= 0);
		iFodoOff += iInfoLen;
	}
	return bFound;
} /* end of bGet8PicInfo */

/*
 * Build the lists with Character Information for Word 8/9/10 files
 */
void
vGet8ChrInfo(FILE *pFile, const pps_info_type *pPPS,
	const ULONG *aulBBD, size_t tBBDLen,
	const ULONG *aulSBD, size_t tSBDLen,
	const UCHAR *aucHeader)
{
	font_block_type		tFont;
	picture_block_type	tPicture;
	ULONG		*aulCharPage;
	const ULONG	*aulBlockDepot;
	UCHAR	*aucBuffer;
	ULONG	ulFileOffset, ulCharPos, ulBeginCharInfo;
	ULONG	ulTableSize, ulTableStartBlock;
	size_t	tCharInfoLen, tBlockDepotLen;
	size_t	tOffset, tBlockSize, tLen;
	int	iIndex, iIndex2, iRun, iFodo, iLen;
	USHORT	usDocStatus, usIstd;
	UCHAR	aucFpage[BIG_BLOCK_SIZE];

	fail(pFile == NULL || pPPS == NULL || aucHeader == NULL);
	fail(aulBBD == NULL || aulSBD == NULL);

	ulBeginCharInfo = ulGetLong(0xfa, aucHeader); /* fcPlcfbteChpx */
	NO_DBG_HEX(ulBeginCharInfo);
	tCharInfoLen = (size_t)ulGetLong(0xfe, aucHeader); /* lcbPlcfbteChpx */
	NO_DBG_DEC(tCharInfoLen);
	if (tCharInfoLen < 4) {
		DBG_DEC(tCharInfoLen);
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
	DBG_DEC(ulTableStartBlock);
	if (ulTableStartBlock == 0) {
		DBG_MSG("No character information");
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
	aucBuffer = xmalloc(tCharInfoLen);
	if (!bReadBuffer(pFile, ulTableStartBlock,
			aulBlockDepot, tBlockDepotLen, tBlockSize,
			aucBuffer, ulBeginCharInfo, tCharInfoLen)) {
		aucBuffer = xfree(aucBuffer);
		return;
	}
	NO_DBG_PRINT_BLOCK(aucBuffer, tCharInfoLen);

	tLen = (tCharInfoLen / 4 - 1) / 2;
	aulCharPage = xcalloc(tLen, sizeof(ULONG));
	for (iIndex = 0, tOffset = (tLen + 1) * 4;
	     iIndex < (int)tLen;
	     iIndex++, tOffset += 4) {
		 aulCharPage[iIndex] = ulGetLong(tOffset, aucBuffer);
		 NO_DBG_DEC(aulCharPage[iIndex]);
	}
	DBG_HEX(ulGetLong(0, aucBuffer));
	aucBuffer = xfree(aucBuffer);
	NO_DBG_PRINT_BLOCK(aucHeader, HEADER_SIZE);

	for (iIndex = 0; iIndex < (int)tLen; iIndex++) {
		fail(aulCharPage[iIndex] > ULONG_MAX / BIG_BLOCK_SIZE);
		if (!bReadBuffer(pFile, pPPS->tWordDocument.ulSB,
				aulBBD, tBBDLen, BIG_BLOCK_SIZE,
				aucFpage,
				aulCharPage[iIndex] * BIG_BLOCK_SIZE,
				BIG_BLOCK_SIZE)) {
			break;
		}
		NO_DBG_PRINT_BLOCK(aucFpage, BIG_BLOCK_SIZE);
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
				vGet8FontInfo(iFodo, usIstd,
					aucFpage + 1, iLen - 1, &tFont);
			}
			tFont.ulFileOffset = ulFileOffset;
			vAdd2FontInfoList(&tFont);

			if (iFodo <= 0) {
				continue;
			}

			(void)memset(&tPicture, 0, sizeof(tPicture));
			if (bGet8PicInfo(iFodo, aucFpage + 1,
						iLen - 1, &tPicture)) {
				tPicture.ulFileOffset = ulFileOffset;
				tPicture.ulFileOffsetPicture =
					ulDataPos2FileOffset(
						tPicture.ulPictureOffset);
				vAdd2PictInfoList(&tPicture);
			}
		}
	}
	aulCharPage = xfree(aulCharPage);
} /* end of vGet8ChrInfo */
