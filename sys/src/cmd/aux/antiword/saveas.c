/*
 * saveas.c
 * Copyright (C) 1998-2001 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to save the results as a textfile or a drawfile
 */

#include <stdio.h>
#include <string.h>
#include "saveas.h"
#include "event.h"
#include "antiword.h"

static BOOL
bWrite2File(void *pvBytes, size_t tSize, FILE *pFile, const char *szFilename)
{
	if (fwrite(pvBytes, sizeof(char), tSize, pFile) != tSize) {
		werr(0, "I can't write to '%s'", szFilename);
		return FALSE;
	}
	return TRUE;
} /* end of bWrite2File */

/*
 * bText2File - Save the generated draw file to a Text file
 */
static BOOL
bText2File(char *szFilename, void *pvHandle)
{
	FILE	*pFile;
	diagram_type	*pDiag;
	draw_textstrhdr	tText;
	char	*pcTmp;
	int	iToGo, iSize, iX, iYtopPrev, iHeight, iLines;
	BOOL	bFirst, bIndent, bSuccess;

	fail(szFilename == NULL || szFilename[0] == '\0');
	fail(pvHandle == NULL);

	DBG_MSG("bText2File");
	DBG_MSG(szFilename);

	pDiag = (diagram_type *)pvHandle;
	pFile = fopen(szFilename, "w");
	if (pFile == NULL) {
		werr(0, "I can't open '%s' for writing", szFilename);
		return FALSE;
	}
	bFirst = TRUE;
	iYtopPrev = 0;
	iHeight = (int)lWord2DrawUnits20(DEFAULT_FONT_SIZE);
	bSuccess = TRUE;
	iToGo = pDiag->tInfo.length - sizeof(draw_fileheader);
	DBG_DEC(iToGo);
	pcTmp = pDiag->tInfo.data + sizeof(draw_fileheader);
	while (iToGo > 0 && bSuccess) {
		tText = *(draw_textstrhdr *)pcTmp;
		switch (tText.tag) {
		case draw_OBJFONTLIST:
			/* These are not relevant in a textfile */
			iSize = ((draw_fontliststrhdr *)pcTmp)->size;
			pcTmp += iSize;
			iToGo -= iSize;
			break;
		case draw_OBJTEXT:
			/* Compute the number of lines */
			iLines =
			(iYtopPrev - tText.bbox.y1 + iHeight / 2) / iHeight;
			fail(iLines < 0);
			bIndent = iLines > 0 || bFirst;
			bFirst = FALSE;
			/* Print the newlines */
			while (iLines > 0 && bSuccess) {
				bSuccess = bWrite2File("\n",
					1, pFile, szFilename);
				iLines--;
			}
			/* Print the indentation */
			if (bIndent && bSuccess) {
				for (iX = draw_screenToDraw(8);
				     iX <= tText.bbox.x0 && bSuccess;
				     iX += draw_screenToDraw(16)) {
					bSuccess = bWrite2File(" ",
						1, pFile, szFilename);
				}
			}
			if (!bSuccess) {
				break;
			}
			/* Print the text object */
			pcTmp += sizeof(tText);
			bSuccess = bWrite2File(pcTmp,
				strlen(pcTmp), pFile, szFilename);
			pcTmp += tText.size - sizeof(tText);
			/* Setup for the next text object */
			iToGo -= tText.size;
			iYtopPrev = tText.bbox.y1;
			iHeight = tText.bbox.y1 - tText.bbox.y0;
			break;
		case draw_OBJPATH:
			/* These are not relevant in a textfile */
			iSize = ((draw_pathstrhdr *)pcTmp)->size;
			pcTmp += iSize;
			iToGo -= iSize;
			break;
		case draw_OBJSPRITE:
		case draw_OBJJPEG:
			/* These are not relevant in a textfile */
			iSize = ((draw_spristrhdr *)pcTmp)->size;
			pcTmp += iSize;
			iToGo -= iSize;
			break;
		default:
			DBG_DEC(tText.tag);
			bSuccess = FALSE;
			break;
		}
	}
	DBG_DEC_C(iToGo != 0, iToGo);
	if (bSuccess) {
		bSuccess = bWrite2File("\n", 1, pFile, szFilename);
	}
	(void)fclose(pFile);
	if (bSuccess) {
		vSetFiletype(szFilename, FILETYPE_TEXT);
	} else {
		(void)remove(szFilename);
		werr(0, "Unable to save textfile '%s'", szFilename);
	}
	return bSuccess;
} /* end of bText2File */

/*
 * vSaveTextfile
 */
void
vSaveTextfile(diagram_type *pDiagram)
{
	wimp_emask	tMask;
	int		iRecLen, iNbrRecs, iEstSize;

	fail(pDiagram == NULL);

	DBG_MSG("vSaveTextfile");
	iRecLen = sizeof(draw_textstrhdr) + DEFAULT_SCREEN_WIDTH * 2 / 3;
	iNbrRecs = pDiagram->tInfo.length / iRecLen + 1;
	iEstSize = iNbrRecs * DEFAULT_SCREEN_WIDTH * 2 / 3;
	DBG_DEC(iEstSize);

	tMask = event_getmask();
	event_setmask(0);
	saveas(FILETYPE_TEXT, "WordText",
		iEstSize, bText2File,
		NULL, NULL, pDiagram);
	event_setmask(tMask);
} /* end of vSaveTextfile */

/*
 * bDraw2File - Save the generated draw file to a Draw file
 *
 * Remark: This is not a simple copy action. The origin of the
 * coordinates (0,0) must move from the top-left corner to the
 * bottom-left corner.
 */
static BOOL
bDraw2File(char *szFilename, void *pvHandle)
{
	FILE		*pFile;
	diagram_type	*pDiagram;
	draw_fileheader tHdr;
	draw_textstrhdr	tText;
	draw_pathstrhdr	tPath;
	draw_spristrhdr	tSprite;
	draw_jpegstrhdr tJpeg;
	int	*piPath;
	char	*pcTmp;
	int	iYadd, iToGo, iSize;
	BOOL	bSuccess;

	fail(szFilename == NULL || szFilename[0] == '\0');
	fail(pvHandle == NULL);

	DBG_MSG("bDraw2File");
	NO_DBG_MSG(szFilename);

	pDiagram = (diagram_type *)pvHandle;
	pFile = fopen(szFilename, "wb");
	if (pFile == NULL) {
		werr(0, "I can't open '%s' for writing", szFilename);
		return FALSE;
	}
	iToGo = pDiagram->tInfo.length;
	DBG_DEC(iToGo);
	pcTmp = pDiagram->tInfo.data;
	tHdr = *(draw_fileheader *)pcTmp;
	iYadd = -tHdr.bbox.y0;
	tHdr.bbox.y0 += iYadd;
	tHdr.bbox.y1 += iYadd;
	bSuccess = bWrite2File(&tHdr, sizeof(tHdr), pFile, szFilename);
	iToGo -= sizeof(tHdr);
	DBG_DEC(iToGo);
	pcTmp += sizeof(tHdr);
	while (iToGo > 0 && bSuccess) {
		tText = *(draw_textstrhdr *)pcTmp;
		switch (tText.tag) {
		case draw_OBJFONTLIST:
			iSize = ((draw_fontliststrhdr *)pcTmp)->size;
			bSuccess = bWrite2File(pcTmp,
					iSize, pFile, szFilename);
			pcTmp += iSize;
			iToGo -= iSize;
			break;
		case draw_OBJTEXT:
			/* First correct the coordinates */
			tText.bbox.y0 += iYadd;
			tText.bbox.y1 += iYadd;
			tText.coord.y += iYadd;
			/* Now write the information to file */
			bSuccess = bWrite2File(&tText,
				sizeof(tText), pFile, szFilename);
			pcTmp += sizeof(tText);
			iSize = tText.size - sizeof(tText);
			if (bSuccess) {
				bSuccess = bWrite2File(pcTmp,
					iSize, pFile, szFilename);
				pcTmp += iSize;
			}
			iToGo -= tText.size;
			break;
		case draw_OBJPATH:
			tPath = *(draw_pathstrhdr *)pcTmp;
			/* First correct the coordinates */
			tPath.bbox.y0 += iYadd;
			tPath.bbox.y1 += iYadd;
			/* Now write the information to file */
			bSuccess = bWrite2File(&tPath,
				sizeof(tPath), pFile, szFilename);
			pcTmp += sizeof(tPath);
			iSize = tPath.size - sizeof(tPath);
			fail(iSize < 14 * sizeof(int));
			/* Second correct the path coordinates */
			piPath = xmalloc(iSize);
			memcpy(piPath, pcTmp, iSize);
			piPath[ 2] += iYadd;
			piPath[ 5] += iYadd;
			piPath[ 8] += iYadd;
			piPath[11] += iYadd;
			if (bSuccess) {
				bSuccess = bWrite2File(piPath,
					iSize, pFile, szFilename);
				pcTmp += iSize;
			}
			piPath = xfree(piPath);
			iToGo -= tPath.size;
			break;
		case draw_OBJSPRITE:
			tSprite = *(draw_spristrhdr *)pcTmp;
			/* First correct the coordinates */
			tSprite.bbox.y0 += iYadd;
			tSprite.bbox.y1 += iYadd;
			/* Now write the information to file */
			bSuccess = bWrite2File(&tSprite,
				sizeof(tSprite), pFile, szFilename);
			pcTmp += sizeof(tSprite);
			iSize = tSprite.size - sizeof(tSprite);
			if (bSuccess) {
				bSuccess = bWrite2File(pcTmp,
					iSize, pFile, szFilename);
				pcTmp += iSize;
			}
			iToGo -= tSprite.size;
			break;
		case draw_OBJJPEG:
			tJpeg = *(draw_jpegstrhdr *)pcTmp;
			/* First correct the coordinates */
			tJpeg.bbox.y0 += iYadd;
			tJpeg.bbox.y1 += iYadd;
			tJpeg.trfm[5] += iYadd;
			/* Now write the information to file */
			bSuccess = bWrite2File(&tJpeg,
				sizeof(tJpeg), pFile, szFilename);
			pcTmp += sizeof(tJpeg);
			iSize = tJpeg.size - sizeof(tJpeg);
			if (bSuccess) {
				bSuccess = bWrite2File(pcTmp,
					iSize, pFile, szFilename);
				pcTmp += iSize;
			}
			iToGo -= tJpeg.size;
			break;
		default:
			DBG_DEC(tText.tag);
			bSuccess = FALSE;
			break;
		}
	}
	DBG_DEC_C(iToGo != 0, iToGo);
	(void)fclose(pFile);
	if (bSuccess) {
		vSetFiletype(szFilename, FILETYPE_DRAW);
	} else {
		(void)remove(szFilename);
		werr(0, "Unable to save drawfile '%s'", szFilename);
	}
	return bSuccess;
} /* end of bDraw2File */

/*
 * vSaveDrawfile
 */
void
vSaveDrawfile(diagram_type *pDiagram)
{
	wimp_emask	tMask;
	int		iEstSize;

	fail(pDiagram == NULL);

	DBG_MSG("vSaveDrawfile");
	iEstSize = pDiagram->tInfo.length;
	DBG_DEC(iEstSize);

	tMask = event_getmask();
	event_setmask(0);
	saveas(FILETYPE_DRAW, "WordDraw",
		iEstSize, bDraw2File,
		NULL, NULL, pDiagram);
	event_setmask(tMask);
} /* end of vSaveDrawfile */
