/*
 * draw.c
 * Copyright (C) 1998-2003 A.J. van Os; Released under GPL
 *
 * Description:
 * Functions to deal with the Draw format
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "akbd.h"
#include "flex.h"
#include "wimp.h"
#include "template.h"
#include "wimpt.h"
#include "win.h"
#include "antiword.h"


/* The work area must be a little bit larger than the diagram */
#define WORKAREA_EXTENSION	    5
/* Diagram memory */
#define INITIAL_SIZE		32768	/* 32k */
#define EXTENSION_SIZE		 4096	/*  4k */
/* Main window title */
#define WINDOW_TITLE_LEN	   28
#define FILENAME_TITLE_LEN	(WINDOW_TITLE_LEN - 10)

static BOOL	(*bDrawRenderDiag)(draw_diag *,
			draw_redrawstr *, double, draw_error *) = NULL;

/*
 * vCreateMainWindow - create the Main Window
 *
 * remark: does not return if the Main Window can't be created
 */
static wimp_w
tCreateMainWindow(void)
{
	static int	iY = 0;
	template	*pTemplate;
	wimp_w		tMainWindow;

	/* Find and check the template */
	pTemplate = template_find("MainWindow");
	if (pTemplate == NULL) {
		werr(1, "The 'MainWindow' template can't be found");
	}
	pTemplate = template_copy(pTemplate);
	if (pTemplate == NULL) {
		werr(1, "I can't copy the 'MainWindow' template");
	}
	if ((pTemplate->window.titleflags & wimp_INDIRECT) !=
							wimp_INDIRECT) {
		werr(1,
	"The title of the 'MainWindow' template must be indirected text");
	}
	if (pTemplate->window.title.indirecttext.bufflen < WINDOW_TITLE_LEN) {
		werr(1, "The 'MainWindow' title needs %d characters",
			WINDOW_TITLE_LEN);
	}

	/*
	 * Leave 48 OS units between two windows, as recommended by the
	 * Style guide. And try to stay away from the iconbar.
	 */
	if (pTemplate->window.box.y0 < iY + 130) {
		iY = 48;
	} else {
		pTemplate->window.box.y0 -= iY;
		pTemplate->window.box.y1 -= iY;
		iY += 48;
	}

	/* Create the window */
	wimpt_noerr(wimp_create_wind(&pTemplate->window, &tMainWindow));
	return tMainWindow;
} /* end of tCreateMainWindow */

/*
 * vCreateScaleWindow - create the Scale view Window
 *
 * remark: does not return if the Scale view Window can't be created
 */
static wimp_w
tCreateScaleWindow(void)
{
	wimp_wind	*pw;
	wimp_w		tScaleWindow;

	pw = template_syshandle("ScaleView");
	if (pw == NULL) {
		werr(1, "Template 'ScaleView' can't be found");
	}
	wimpt_noerr(wimp_create_wind(pw, &tScaleWindow));
	return tScaleWindow;
} /* end of tCreateScaleWindow */

/*
 * pCreateDiagram - create and initialize a diagram
 *
 * remark: does not return if the diagram can't be created
 */
diagram_type *
pCreateDiagram(const char *szTask, const char *szFilename)
{
	diagram_type	*pDiag;
	options_type	tOptions;
	wimp_w		tMainWindow, tScaleWindow;
	draw_box	tBox;

	DBG_MSG("pCreateDiagram");

	fail(szTask == NULL || szTask[0] == '\0');

	/* Create the main window */
	tMainWindow = tCreateMainWindow();
	/* Create the scale view window */
	tScaleWindow = tCreateScaleWindow();

	/* Get the necessary memory */
	pDiag = xmalloc(sizeof(diagram_type));
	if (flex_alloc((flex_ptr)&pDiag->tInfo.data, INITIAL_SIZE) != 1) {
		werr(1, "Memory allocation failed, unable to continue");
	}

	/* Determine which function to use for rendering the diagram */
	if (iGetRiscOsVersion() >= 360) {
		/* Home brew for RISC OS 3.6 functionality */
	  	bDrawRenderDiag = bDrawRenderDiag360;
	  } else {
		/* The function from RISC_OSLib */
		bDrawRenderDiag = draw_render_diag;
	}

	/* Initialize the diagram */
	vGetOptions(&tOptions);
	pDiag->tMainWindow = tMainWindow;
	pDiag->tScaleWindow = tScaleWindow;
	pDiag->iScaleFactorCurr = tOptions.iScaleFactor;
	pDiag->iScaleFactorTemp = tOptions.iScaleFactor;
	pDiag->tMemorySize = INITIAL_SIZE;
	tBox.x0 = 0;
	tBox.y0 = -(draw_screenToDraw(32 + 3) * 8 + 1);
	tBox.x1 = draw_screenToDraw(16) * MIN_SCREEN_WIDTH + 1;
	tBox.y1 = 0;
	draw_create_diag(&pDiag->tInfo, (char *)szTask, tBox);
	DBG_DEC(pDiag->tInfo.length);
	pDiag->lXleft = 0;
	pDiag->lYtop = 0;
	strncpy(pDiag->szFilename,
			szBasename(szFilename), sizeof(pDiag->szFilename) - 1);
	pDiag->szFilename[sizeof(pDiag->szFilename) - 1] = '\0';
	/* Return success */
	return pDiag;
} /* end of pCreateDiagram */

/*
 * vDestroyDiagram - remove a diagram by freeing the memory it uses
 */
static void
vDestroyDiagram(wimp_w tWindow, diagram_type *pDiag)
{
	DBG_MSG("vDestroyDiagram");

	fail(pDiag != NULL && pDiag->tMainWindow != tWindow);

	wimpt_noerr(wimp_close_wind(tWindow));
	if (pDiag == NULL) {
		return;
	}
	if (pDiag->tInfo.data != NULL && pDiag->tMemorySize != 0) {
		flex_free((flex_ptr)&pDiag->tInfo.data);
	}
	pDiag = xfree(pDiag);
} /* end of vDestroyDiagram */

/*
 * vPrintDrawError - print an error reported by a draw function
 */
static void
vPrintDrawError(draw_error *pError)
{
	DBG_MSG("vPrintDrawError");

	fail(pError == NULL);

	switch (pError->type) {
	case DrawOSError:
		DBG_DEC(pError->err.os.errnum);
		DBG_MSG(pError->err.os.errmess);
		werr(1, "DrawOSError: %d: %s",
			pError->err.os.errnum, pError->err.os.errmess);
		break;
	case DrawOwnError:
		DBG_DEC(pError->err.draw.code);
		DBG_HEX(pError->err.draw.location);
		werr(1, "DrawOwnError: Code %d - Location &%x",
			pError->err.draw.code, pError->err.draw.location);
		break;
	case None:
	default:
		break;
	}
} /* end of vPrintDrawError */

/*
 * vExtendDiagramSize - make sure the diagram is big enough
 */
static void
vExtendDiagramSize(diagram_type *pDiag, size_t tSize)
{
	fail(pDiag == NULL || tSize % 4 != 0);

	while (pDiag->tInfo.length + tSize > pDiag->tMemorySize) {
		if (flex_extend((flex_ptr)&pDiag->tInfo.data,
				pDiag->tMemorySize + EXTENSION_SIZE) != 1) {
			werr(1, "Memory extend failed, unable to continue");
		}
		pDiag->tMemorySize += EXTENSION_SIZE;
		NO_DBG_DEC(pDiag->tMemorySize);
	}
} /* end of vExtendDiagramSize */

/*
 * vPrologue2 - prologue part 2; add a font list to a diagram
 */
void
vPrologue2(diagram_type *pDiag, int iWordVersion)
{
	draw_objectType	tNew;
	draw_error	tError;
	draw_object	tHandle;
	const font_table_type	*pTmp;
	char	*pcTmp;
	size_t	tRealSize, tSize;
	int	iCount;

	fail(pDiag == NULL);

	if (tGetFontTableLength() == 0) {
		return;
	}
	tRealSize = sizeof(draw_fontliststrhdr);
	pTmp = NULL;
	while ((pTmp = pGetNextFontTableRecord(pTmp)) != NULL) {
		tRealSize += 2 + strlen(pTmp->szOurFontname);
	}
	tSize = ROUND4(tRealSize);
	vExtendDiagramSize(pDiag, tSize);
	tNew.fontList = xmalloc(tSize);
	tNew.fontList->tag = draw_OBJFONTLIST;
	tNew.fontList->size = tSize;
	pcTmp = (char *)&tNew.fontList->fontref;
	iCount = 0;
	pTmp = NULL;
	while ((pTmp = pGetNextFontTableRecord(pTmp)) != NULL) {
		*pcTmp = ++iCount;
		pcTmp++;
		strcpy(pcTmp, pTmp->szOurFontname);
		pcTmp += 1 + strlen(pTmp->szOurFontname);
	}
	memset((char *)tNew.fontList + tRealSize, 0, tSize - tRealSize);
	if (draw_createObject(&pDiag->tInfo, tNew, draw_LastObject,
						TRUE, &tHandle, &tError)) {
		draw_translateText(&pDiag->tInfo);
	} else {
		DBG_MSG("draw_createObject() failed");
		vPrintDrawError(&tError);
	}
	tNew.fontList = xfree(tNew.fontList);
} /* end of vPrologue2 */

/*
 * vSubstring2Diagram - put a sub string into a diagram
 */
void
vSubstring2Diagram(diagram_type *pDiag,
	char *szString, size_t tStringLength, long lStringWidth,
	UCHAR ucFontColor, USHORT usFontstyle, draw_fontref tFontRef,
	USHORT usFontSize, USHORT usMaxFontSize)
{
	draw_objectType	tNew;
	draw_error	tError;
	draw_object	tHandle;
	long	lSizeX, lSizeY, lOffset, l20, lYMove;
	size_t	tRealSize, tSize;

	fail(pDiag == NULL || szString == NULL);
	fail(pDiag->lXleft < 0);
	fail(tStringLength != strlen(szString));
	fail(usFontSize < MIN_FONT_SIZE || usFontSize > MAX_FONT_SIZE);
	fail(usMaxFontSize < MIN_FONT_SIZE || usMaxFontSize > MAX_FONT_SIZE);
	fail(usFontSize > usMaxFontSize);

	if (szString[0] == '\0' || tStringLength == 0) {
		return;
	}

	if (tFontRef == 0) {
		lOffset = draw_screenToDraw(2);
		l20 = draw_screenToDraw(32 + 3);
		lSizeX = draw_screenToDraw(16);
		lSizeY = draw_screenToDraw(32);
	} else {
		lOffset = lToBaseLine(usMaxFontSize);
		l20 = lWord2DrawUnits20(usMaxFontSize);
		lSizeX = lWord2DrawUnits00(usFontSize);
		lSizeY = lWord2DrawUnits00(usFontSize);
	}

	lYMove = 0;

	/* Up for superscript */
	if (bIsSuperscript(usFontstyle)) {
		lYMove = lMilliPoints2DrawUnits((((long)usFontSize + 1) / 2) * 375);
	}
	/* Down for subscript */
	if (bIsSubscript(usFontstyle)) {
		lYMove = -lMilliPoints2DrawUnits((long)usFontSize * 125);
	}

	tRealSize = sizeof(draw_textstr) + tStringLength;
	tSize = ROUND4(tRealSize);
	vExtendDiagramSize(pDiag, tSize);
	tNew.text = xmalloc(tSize);
	tNew.text->tag = draw_OBJTEXT;
	tNew.text->size = tSize;
	tNew.text->bbox.x0 = (int)pDiag->lXleft;
	tNew.text->bbox.y0 = (int)(pDiag->lYtop + lYMove);
	tNew.text->bbox.x1 = (int)(pDiag->lXleft + lStringWidth);
	tNew.text->bbox.y1 = (int)(pDiag->lYtop + l20 + lYMove);
	tNew.text->textcolour = (draw_coltyp)ulColor2Color(ucFontColor);
	tNew.text->background = 0xffffff00;	/* White */
	tNew.text->textstyle.fontref = tFontRef;
	tNew.text->textstyle.reserved8 = 0;
	tNew.text->textstyle.reserved16 = 0;
	tNew.text->fsizex = (int)lSizeX;
	tNew.text->fsizey = (int)lSizeY;
	tNew.text->coord.x = (int)pDiag->lXleft;
	tNew.text->coord.y = (int)(pDiag->lYtop + lOffset + lYMove);
	strncpy(tNew.text->text, szString, tStringLength);
	tNew.text->text[tStringLength] = '\0';
	memset((char *)tNew.text + tRealSize, 0, tSize - tRealSize);
	if (!draw_createObject(&pDiag->tInfo, tNew, draw_LastObject,
						TRUE, &tHandle, &tError)) {
		DBG_MSG("draw_createObject() failed");
		vPrintDrawError(&tError);
	}
	tNew.text = xfree(tNew.text);
	draw_translateText(&pDiag->tInfo);
	pDiag->lXleft += lStringWidth;
} /* end of vSubstring2Diagram */

/*
 * vImage2Diagram - put an image into a diagram
 */
void
vImage2Diagram(diagram_type *pDiag, const imagedata_type *pImg,
	UCHAR *pucImage, size_t tImageSize)
{
  	draw_objectType	tTmp;
  	draw_imageType	tNew;
	draw_error	tError;
	draw_object	tHandle;
	long	lWidth, lHeight;
	size_t	tRealSize, tSize;

	DBG_MSG("vImage2Diagram");

	fail(pDiag == NULL);
	fail(pImg == NULL);
	fail(pDiag->lXleft < 0);
	fail(pImg->eImageType != imagetype_is_dib &&
	     pImg->eImageType != imagetype_is_jpeg);

	DBG_DEC_C(pDiag->lXleft != 0, pDiag->lXleft);

	lWidth = lPoints2DrawUnits(pImg->iHorSizeScaled);
	lHeight = lPoints2DrawUnits(pImg->iVerSizeScaled);
	DBG_DEC(lWidth);
	DBG_DEC(lHeight);

	pDiag->lYtop -= lHeight;

	switch (pImg->eImageType) {
	case imagetype_is_dib:
		tRealSize = sizeof(draw_spristrhdr) + tImageSize;
		tSize = ROUND4(tRealSize);
		vExtendDiagramSize(pDiag, tSize);
		tNew.sprite = xmalloc(tSize);
		tNew.sprite->tag = draw_OBJSPRITE;
		tNew.sprite->size = tSize;
		tNew.sprite->bbox.x0 = (int)pDiag->lXleft;
		tNew.sprite->bbox.y0 = (int)pDiag->lYtop;
		tNew.sprite->bbox.x1 = (int)(pDiag->lXleft + lWidth);
		tNew.sprite->bbox.y1 = (int)(pDiag->lYtop + lHeight);
		memcpy(&tNew.sprite->sprite, pucImage, tImageSize);
		memset((char *)tNew.sprite + tRealSize, 0, tSize - tRealSize);
		break;
	case imagetype_is_jpeg:
#if defined(DEBUG)
		(void)bGetJpegInfo(pucImage, tImageSize);
#endif /* DEBUG */
		tRealSize = sizeof(draw_jpegstrhdr) + tImageSize;
		tSize = ROUND4(tRealSize);
		vExtendDiagramSize(pDiag, tSize);
		tNew.jpeg = xmalloc(tSize);
		tNew.jpeg->tag = draw_OBJJPEG;
		tNew.jpeg->size = tSize;
		tNew.jpeg->bbox.x0 = (int)pDiag->lXleft;
		tNew.jpeg->bbox.y0 = (int)pDiag->lYtop;
		tNew.jpeg->bbox.x1 = (int)(pDiag->lXleft + lWidth);
		tNew.jpeg->bbox.y1 = (int)(pDiag->lYtop + lHeight);
		tNew.jpeg->width = (int)lWidth;
		tNew.jpeg->height = (int)lHeight;
		tNew.jpeg->xdpi = 90;
		tNew.jpeg->ydpi = 90;
		tNew.jpeg->trfm[0] = 0x10000;
		tNew.jpeg->trfm[1] = 0;
		tNew.jpeg->trfm[2] = 0;
		tNew.jpeg->trfm[3] = 0x10000;
		tNew.jpeg->trfm[4] = (int)pDiag->lXleft;
		tNew.jpeg->trfm[5] = (int)pDiag->lYtop;
		tNew.jpeg->len = tImageSize;
		memcpy(&tNew.jpeg->jpeg, pucImage, tImageSize);
		memset((char *)tNew.jpeg + tRealSize, 0, tSize - tRealSize);
		break;
	default:
		DBG_DEC(pImg->eImageType);
		break;
	}

	tTmp = *(draw_objectType *)&tNew;
	if (!draw_createObject(&pDiag->tInfo, tTmp, draw_LastObject,
						TRUE, &tHandle, &tError)) {
		DBG_MSG("draw_createObject() failed");
		vPrintDrawError(&tError);
	}

	switch (pImg->eImageType) {
	case imagetype_is_dib:
		tNew.sprite = xfree(tNew.sprite);
		break;
	case imagetype_is_jpeg:
		tNew.jpeg = xfree(tNew.jpeg);
		break;
	default:
		DBG_DEC(pImg->eImageType);
		break;
	}
	pDiag->lXleft = 0;
} /* end of vImage2Diagram */

/*
 * bAddDummyImage - add a dummy image
 *
 * return TRUE when successful, otherwise FALSE
 */
BOOL
bAddDummyImage(diagram_type *pDiag, const imagedata_type *pImg)
{
  	draw_objectType	tNew;
	draw_error	tError;
	draw_object	tHandle;
	int	*piTmp;
	long	lWidth, lHeight;
	size_t	tRealSize, tSize;

	DBG_MSG("bAddDummyImage");

	fail(pDiag == NULL);
	fail(pImg == NULL);
	fail(pDiag->lXleft < 0);

	if (pImg->iVerSizeScaled <= 0 || pImg->iHorSizeScaled <= 0) {
		return FALSE;
	}

	DBG_DEC_C(pDiag->lXleft != 0, pDiag->lXleft);

	lWidth = lPoints2DrawUnits(pImg->iHorSizeScaled);
	lHeight = lPoints2DrawUnits(pImg->iVerSizeScaled);

	pDiag->lYtop -= lHeight;

	tRealSize = sizeof(draw_pathstrhdr) + 14 * sizeof(int);
	tSize = ROUND4(tRealSize);
	vExtendDiagramSize(pDiag, tSize);
	tNew.path = xmalloc(tSize);
	tNew.path->tag = draw_OBJPATH;
	tNew.path->size = tSize;
	tNew.path->bbox.x0 = (int)pDiag->lXleft;
	tNew.path->bbox.y0 = (int)pDiag->lYtop;
	tNew.path->bbox.x1 = (int)(pDiag->lXleft + lWidth);
	tNew.path->bbox.y1 = (int)(pDiag->lYtop + lHeight);
	tNew.path->fillcolour = -1;
	tNew.path->pathcolour = 0x4d4d4d00;	/* Gray 70 percent */
	tNew.path->pathwidth = (int)lMilliPoints2DrawUnits(500);
	tNew.path->pathstyle.joincapwind = 0;
	tNew.path->pathstyle.reserved8 = 0;
	tNew.path->pathstyle.tricapwid = 0;
	tNew.path->pathstyle.tricaphei = 0;
	piTmp = (int *)((char *)tNew.path + sizeof(draw_pathstrhdr));
	*piTmp++ = draw_PathMOVE;
	*piTmp++ = tNew.path->bbox.x0;
	*piTmp++ = tNew.path->bbox.y0;
	*piTmp++ = draw_PathLINE;
	*piTmp++ = tNew.path->bbox.x0;
	*piTmp++ = tNew.path->bbox.y1;
	*piTmp++ = draw_PathLINE;
	*piTmp++ = tNew.path->bbox.x1;
	*piTmp++ = tNew.path->bbox.y1;
	*piTmp++ = draw_PathLINE;
	*piTmp++ = tNew.path->bbox.x1;
	*piTmp++ = tNew.path->bbox.y0;
	*piTmp++ = draw_PathCLOSE;
	*piTmp++ = draw_PathTERM;
	memset((char *)tNew.path + tRealSize, 0, tSize - tRealSize);
	if (!draw_createObject(&pDiag->tInfo, tNew, draw_LastObject,
						TRUE, &tHandle, &tError)) {
		DBG_MSG("draw_createObject() failed");
		vPrintDrawError(&tError);
	}
	tNew.path = xfree(tNew.path);
	pDiag->lXleft = 0;
	return TRUE;
} /* end of bAddDummyImage */

/*
 * vMove2NextLine - move to the next line
 */
void
vMove2NextLine(diagram_type *pDiag, draw_fontref tFontRef, USHORT usFontSize)
{
	long	l20;

	fail(pDiag == NULL);
	fail(usFontSize < MIN_FONT_SIZE || usFontSize > MAX_FONT_SIZE);

	if (tFontRef == 0) {
		l20 = draw_screenToDraw(32 + 3);
	} else {
		l20 = lWord2DrawUnits20(usFontSize);
	}
	pDiag->lYtop -= l20;
} /* end of vMove2NextLine */

/*
 * Create an start of paragraph (Phase 1)
 */
void
vStartOfParagraph1(diagram_type *pDiag, long lBeforeIndentation)
{
	fail(pDiag == NULL);
	fail(lBeforeIndentation < 0);

	pDiag->lXleft = 0;
	pDiag->lYtop -= lMilliPoints2DrawUnits(lBeforeIndentation);
} /* end of vStartOfParagraph1 */

/*
 * Create an start of paragraph (Phase 2)
 */
void
vStartOfParagraph2(diagram_type *pDiag)
{
	/* DUMMY */
} /* end of vStartOfParagraph2 */

/*
 * Create an end of paragraph
 */
void
vEndOfParagraph(diagram_type *pDiag,
	draw_fontref tFontRef, USHORT usFontSize, long lAfterIndentation)
{
	fail(pDiag == NULL);
	fail(usFontSize < MIN_FONT_SIZE || usFontSize > MAX_FONT_SIZE);
	fail(lAfterIndentation < 0);

	pDiag->lXleft = 0;
	pDiag->lYtop -= lMilliPoints2DrawUnits(lAfterIndentation);
} /* end of vEndOfParagraph */

/*
 * Create an end of page
 */
void
vEndOfPage(diagram_type *pDiag, long lAfterIndentation)
{
	fail(pDiag == NULL);
	fail(lAfterIndentation < 0);

	pDiag->lXleft = 0;
	pDiag->lYtop -= lMilliPoints2DrawUnits(lAfterIndentation);
} /* end of vEndOfPage */

/*
 * vSetHeaders - set the headers
 */
void
vSetHeaders(diagram_type *pDiag, USHORT usIstd)
{
	/* DUMMY */
} /* end of vSetHeaders */

/*
 * Create a start of list
 */
void
vStartOfList(diagram_type *pDiag, UCHAR ucNFC, BOOL bIsEndOfTable)
{
	/* DUMMY */
} /* end of vStartOfList */

/*
 * Create an end of list
 */
void
vEndOfList(diagram_type *pDiag)
{
	/* DUMMY */
} /* end of vEndOfList */

/*
 * Create a start of a list item
 */
void
vStartOfListItem(diagram_type *pDiag, BOOL bNoMarks)
{
	/* DUMMY */
} /* end of vStartOfListItem */

/*
 * Create an end of a table
 */
void
vEndOfTable(diagram_type *pDiag)
{
	/* DUMMY */
} /* end of vEndTable */

/*
 * Add a table row
 *
 * Returns TRUE when conversion type is XML
 */
BOOL
bAddTableRow(diagram_type *pDiag, char **aszColTxt,
	int iNbrOfColumns, const short *asColumnWidth, UCHAR ucBorderInfo)
{
	/* DUMMY */
	return FALSE;
} /* end of bAddTableRow */

/*
 * vForceRedraw - force a redraw of the main window
 */
static void
vForceRedraw(diagram_type *pDiag)
{
	wimp_wstate	tWindowState;
	wimp_redrawstr	tRedraw;

	DBG_MSG("vForceRedraw");

	fail(pDiag == NULL);

	DBG_DEC(pDiag->iScaleFactorCurr);

	/* Read the size of the current diagram */
	draw_queryBox(&pDiag->tInfo, (draw_box *)&tRedraw.box, TRUE);
	tRedraw.w = pDiag->tMainWindow;
	/* Adjust the size of the work area */
	tRedraw.box.x0 = tRedraw.box.x0 * pDiag->iScaleFactorCurr / 100 - 1;
	tRedraw.box.y0 = tRedraw.box.y0 * pDiag->iScaleFactorCurr / 100 - 1;
	tRedraw.box.x1 = tRedraw.box.x1 * pDiag->iScaleFactorCurr / 100 + 1;
	tRedraw.box.y1 = tRedraw.box.y1 * pDiag->iScaleFactorCurr / 100 + 1;
	/* Work area extension */
	tRedraw.box.x0 -= WORKAREA_EXTENSION;
	tRedraw.box.y0 -= WORKAREA_EXTENSION;
	tRedraw.box.x1 += WORKAREA_EXTENSION;
	tRedraw.box.y1 += WORKAREA_EXTENSION;
	wimpt_noerr(wimp_set_extent(&tRedraw));
	/* Widen the box slightly to be sure all the edges are drawn */
	tRedraw.box.x0 -= 5;
	tRedraw.box.y0 -= 5;
	tRedraw.box.x1 += 5;
	tRedraw.box.y1 += 5;
	/* Force the redraw */
	wimpt_noerr(wimp_force_redraw(&tRedraw));
	/* Reopen the window to show the correct size */
	wimpt_noerr(wimp_get_wind_state(pDiag->tMainWindow, &tWindowState));
	tWindowState.o.behind = -1;
	wimpt_noerr(wimp_open_wind(&tWindowState.o));
} /* end of vForceRedraw */

/*
 * bVerifyDiagram - Verify the diagram generated from the Word file
 *
 * returns TRUE if the diagram is correct
 */
BOOL
bVerifyDiagram(diagram_type *pDiag)
{
	draw_error	tError;

	fail(pDiag == NULL);
	DBG_MSG("bVerifyDiagram");

	if (draw_verify_diag(&pDiag->tInfo, &tError)) {
		return TRUE;
	}
	DBG_MSG("draw_verify_diag() failed");
	vPrintDrawError(&tError);
	return FALSE;
} /* end of bVerifyDiagram */

/*
 * vShowDiagram - put the diagram on the screen
 */
void
vShowDiagram(diagram_type *pDiag)
{
	wimp_wstate	tWindowState;
	wimp_redrawstr	tRedraw;

	fail(pDiag == NULL);

	DBG_MSG("vShowDiagram");

	wimpt_noerr(wimp_get_wind_state(pDiag->tMainWindow, &tWindowState));
	tWindowState.o.behind = -1;
	wimpt_noerr(wimp_open_wind(&tWindowState.o));

	draw_queryBox(&pDiag->tInfo, (draw_box *)&tRedraw.box, TRUE);
	tRedraw.w = pDiag->tMainWindow;
	/* Work area extension */
	tRedraw.box.x0 -= WORKAREA_EXTENSION;
	tRedraw.box.y0 -= WORKAREA_EXTENSION;
	tRedraw.box.x1 += WORKAREA_EXTENSION;
	tRedraw.box.y1 += WORKAREA_EXTENSION;
	wimpt_noerr(wimp_set_extent(&tRedraw));
	vForceRedraw(pDiag);
} /* end of vShowDiagram */

/*
 * vMainButtonClick - handle mouse buttons clicks for the main screen
 */
static void
vMainButtonClick(wimp_mousestr *m)
{
	wimp_caretstr	c;
	wimp_wstate	ws;

	fail(m == NULL);

	NO_DBG_HEX(m->bbits);
	NO_DBG_DEC(m->i);

	if (m->w >= 0 &&
	    m->i == -1 &&
	    ((m->bbits & wimp_BRIGHT) == wimp_BRIGHT ||
	     (m->bbits & wimp_BLEFT) == wimp_BLEFT)) {
		/* Get the input focus */
		wimpt_noerr(wimp_get_wind_state(m->w, &ws));
		c.w = m->w;
		c.i = -1;
		c.x = m->x - ws.o.box.x0;
		c.y = m->y - ws.o.box.y1;
		c.height = (int)BIT(25);
		c.index = 0;
		wimpt_noerr(wimp_set_caret_pos(&c));
	}
} /* end of vMainButtonClick */

/*
 * vMainKeyPressed - handle pressed keys for the main screen
 */
static void
vMainKeyPressed(int chcode, wimp_caretstr *c, diagram_type *pDiag)
{
	fail(c == NULL || pDiag == NULL);
	fail(c->w != pDiag->tMainWindow);

	switch (chcode) {
	case akbd_Ctl+akbd_Fn+2:	/* Ctrl F2 */
		vDestroyDiagram(c->w, pDiag);
		break;
	case akbd_Fn+3:			/* F3 */
		vSaveDrawfile(pDiag);
		break;
	case akbd_Sh+akbd_Fn+3:		/* Shift F3 */
		vSaveTextfile(pDiag);
		break;
	default:
		DBG_DEC(chcode);
		wimpt_noerr(wimp_processkey(chcode));
	}
} /* end of vMainKeyPressed */

/*
 * vRedrawMainWindow - redraw the main window
 */
static void
vRedrawMainWindow(wimp_w tWindow, diagram_type *pDiag)
{
	wimp_redrawstr	r;
	draw_error	tError;
	double		dScaleFactor;
	draw_diag	*pInfo;
	BOOL		bMore;

	fail(pDiag == NULL);
	fail(pDiag->tMainWindow != tWindow);
	fail(pDiag->iScaleFactorCurr < MIN_SCALE_FACTOR);
	fail(pDiag->iScaleFactorCurr > MAX_SCALE_FACTOR);
	fail(bDrawRenderDiag == NULL);

	dScaleFactor = (double)pDiag->iScaleFactorCurr / 100.0;
	pInfo = &pDiag->tInfo;

	r.w = tWindow;
	wimpt_noerr(wimp_redraw_wind(&r, &bMore));

	while (bMore) {
		if (pInfo->data != NULL) {
			if (!bDrawRenderDiag(pInfo,
					(draw_redrawstr *)&r,
					dScaleFactor,
					&tError)) {
				DBG_MSG("bDrawRenderDiag() failed");
				vPrintDrawError(&tError);
			}
		}
		wimp_get_rectangle(&r, &bMore);
	}
} /* end of vRedrawMainWindow */

/*
 * vMainEventHandler - event handler for the main screen
 */
void
vMainEventHandler(wimp_eventstr *pEvent, void *pvHandle)
{
	diagram_type	*pDiag;

	fail(pEvent == NULL);

	pDiag = (diagram_type *)pvHandle;

	switch (pEvent->e) {
	case wimp_ENULL:
		break;
	case wimp_EREDRAW:
		vRedrawMainWindow(pEvent->data.o.w, pDiag);
		break;
	case wimp_EOPEN:
		wimpt_noerr(wimp_open_wind(&pEvent->data.o));
		break;
	case wimp_ECLOSE:
		vDestroyDiagram(pEvent->data.o.w, pDiag);
		break;
	case wimp_EBUT:
		vMainButtonClick(&pEvent->data.but.m);
		break;
	case wimp_EKEY:
		vMainKeyPressed(pEvent->data.key.chcode,
				&pEvent->data.key.c, pDiag);
		break;
	default:
		break;
	}
} /* end of vMainEventHandler */

/*
 * vScaleOpenAction - action to be taken when the Scale view window opens
 */
void
vScaleOpenAction(diagram_type *pDiag)
{
	wimp_wstate	tWindowState;
	wimp_mousestr	tMouseInfo;
	int		iMoveX, iMoveY;

	fail(pDiag == NULL);

	wimpt_noerr(wimp_get_wind_state(pDiag->tScaleWindow, &tWindowState));
	if ((tWindowState.flags & wimp_WOPEN) == wimp_WOPEN) {
		/* The window is already open */
		return;
	}

	DBG_MSG("vScaleOpenAction");

	/* Allow the window to move in relation to the mouse position */
	wimpt_noerr(wimp_get_point_info(&tMouseInfo));
	iMoveX = tMouseInfo.x - tWindowState.o.box.x0 + 24;
	iMoveY = tMouseInfo.y - tWindowState.o.box.y1 + 20;

	pDiag->iScaleFactorTemp = pDiag->iScaleFactorCurr;
	vUpdateWriteableNumber(pDiag->tScaleWindow,
			SCALE_SCALE_WRITEABLE, pDiag->iScaleFactorTemp);

	tWindowState.o.box.x0 += iMoveX;
	tWindowState.o.box.x1 += iMoveX;
	tWindowState.o.box.y0 += iMoveY;
	tWindowState.o.box.y1 += iMoveY;
	tWindowState.o.behind = -1;
	wimpt_noerr(wimp_open_wind(&tWindowState.o));
} /* end of vScaleOpenAction */

/*
 * vSetTitle - set the title of a window
 */
void
vSetTitle(diagram_type *pDiag)
{
	char	szTitle[WINDOW_TITLE_LEN];

	fail(pDiag == NULL);
	fail(pDiag->szFilename[0] == '\0');

	(void)sprintf(szTitle, "%.*s at %d%%",
				FILENAME_TITLE_LEN,
				pDiag->szFilename,
				pDiag->iScaleFactorCurr % 1000);
	if (strlen(pDiag->szFilename) > FILENAME_TITLE_LEN) {
		szTitle[FILENAME_TITLE_LEN - 1] = OUR_ELLIPSIS;
	}

	win_settitle(pDiag->tMainWindow, szTitle);
} /* end of vSetTitle */


/*
 * vScaleButtonClick - handle a mouse button click in the Scale view window
 */
static void
vScaleButtonClick(wimp_mousestr *m, diagram_type *pDiag)
{
	BOOL	bCloseWindow, bRedraw;

	fail(m == NULL || pDiag == NULL);
	fail(m->w != pDiag->tScaleWindow);

	bCloseWindow = FALSE;
	bRedraw = FALSE;
	switch (m->i) {
	case SCALE_CANCEL_BUTTON:
		bCloseWindow = TRUE;
		pDiag->iScaleFactorTemp = pDiag->iScaleFactorCurr;
		break;
	case SCALE_SCALE_BUTTON:
		bCloseWindow = TRUE;
		bRedraw = pDiag->iScaleFactorCurr != pDiag->iScaleFactorTemp;
		pDiag->iScaleFactorCurr = pDiag->iScaleFactorTemp;
		break;
	case SCALE_50_PCT:
		pDiag->iScaleFactorTemp = 50;
		break;
	case SCALE_75_PCT:
		pDiag->iScaleFactorTemp = 75;
		break;
	case SCALE_100_PCT:
		pDiag->iScaleFactorTemp = 100;
		break;
	case SCALE_150_PCT:
		pDiag->iScaleFactorTemp = 150;
		break;
	default:
		DBG_DEC(m->i);
		break;
	}
	if (bCloseWindow) {
		/* Close the scale window */
		wimpt_noerr(wimp_close_wind(m->w));
		if (bRedraw) {
			/* Redraw the main window */
			vSetTitle(pDiag);
			vForceRedraw(pDiag);
		}
	} else {
		vUpdateWriteableNumber(m->w,
				SCALE_SCALE_WRITEABLE,
				pDiag->iScaleFactorTemp);
	}
} /* end of vScaleButtonClick */

static void
vScaleKeyPressed(int chcode, wimp_caretstr *c, diagram_type *pDiag)
{
	wimp_icon	tIcon;
	char		*pcChar;
	int		iTmp;

	DBG_MSG("vScaleKeyPressed");

	fail(c == NULL || pDiag == NULL);
	fail(c->w != pDiag->tScaleWindow);

	DBG_DEC_C(c->i != SCALE_SCALE_WRITEABLE, c->i);
	DBG_DEC_C(c->i == SCALE_SCALE_WRITEABLE, chcode);

	if (chcode != '\r' ||
	    c->w != pDiag->tScaleWindow ||
	    c->i != SCALE_SCALE_WRITEABLE) {
		wimpt_noerr(wimp_processkey(chcode));
		return;
	}

	wimpt_noerr(wimp_get_icon_info(c->w, c->i, &tIcon));
	if ((tIcon.flags & (wimp_ITEXT|wimp_INDIRECT)) !=
	    (wimp_ITEXT|wimp_INDIRECT)) {
		werr(1, "Icon %d must be indirected text", (int)c->i);
		return;
	}
	iTmp = (int)strtol(tIcon.data.indirecttext.buffer, &pcChar, 10);
	if (*pcChar != '\0' && *pcChar != '\r') {
		DBG_DEC(*pcChar);
	} else if (iTmp < MIN_SCALE_FACTOR) {
		pDiag->iScaleFactorTemp = MIN_SCALE_FACTOR;
	} else if (iTmp > MAX_SCALE_FACTOR) {
		pDiag->iScaleFactorTemp = MAX_SCALE_FACTOR;
	} else {
		pDiag->iScaleFactorTemp = iTmp;
	}
	pDiag->iScaleFactorCurr = pDiag->iScaleFactorTemp;
	/* Close the scale window */
	wimpt_noerr(wimp_close_wind(c->w));
	/* Redraw the main window */
	vSetTitle(pDiag);
	vForceRedraw(pDiag);
} /* end of vScaleKeyPressed */

/*
 * vScaleEventHandler - event handler for the scale view screen
 */
void
vScaleEventHandler(wimp_eventstr *pEvent, void *pvHandle)
{
	diagram_type	*pDiag;

	DBG_MSG("vScaleEventHandler");

	fail(pEvent == NULL);

	DBG_DEC(pEvent->e);

	pDiag = (diagram_type *)pvHandle;

	switch (pEvent->e) {
	case wimp_ENULL:
		break;
	case wimp_EREDRAW:
		/* handled by the WIMP */
		break;
	case wimp_EOPEN:
		wimpt_noerr(wimp_open_wind(&pEvent->data.o));
		break;
	case wimp_ECLOSE:
		wimpt_noerr(wimp_close_wind(pEvent->data.o.w));
		break;
	case wimp_EBUT:
		vScaleButtonClick(&pEvent->data.but.m, pDiag);
		break;
	case wimp_EKEY:
		vScaleKeyPressed(pEvent->data.key.chcode,
				&pEvent->data.key.c, pDiag);
		break;
	default:
		break;
	}
} /* end of vScaleEventHandler */
