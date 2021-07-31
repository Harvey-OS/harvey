/*
 * postscript.c
 * Copyright (C) 1999-2003 A.J. van Os; Released under GNU GPL
 *
 * Description:
 * Functions to deal with the PostScript format
 *
 *================================================================
 * The function vImagePrologue is based on:
 * jpeg2ps - convert JPEG compressed images to PostScript Level 2
 * Copyright (C) 1994-99 Thomas Merz (tm@muc.de)
 *================================================================
 * The credit should go to him, but all the bugs are mine.
 */

#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include "version.h"
#include "antiword.h"

/* The character set */
static encoding_type	eEncoding = encoding_neutral;
/* The image level */
static image_level_enum	eImageLevel = level_default;
/* The output must use landscape orientation */
static BOOL		bUseLandscape = FALSE;
/* The height and width of a PostScript page (in DrawUnits) */
static long		lPageHeight = LONG_MAX;
static long		lPageWidth = LONG_MAX;
/* Current time for a PS header */
static const char	*szCreationDate = NULL;
/* Current creator for a PS header */
static const char	*szCreator = NULL;
/* Current font information */
static draw_fontref	tFontRefCurr = (draw_fontref)-1;
static USHORT		usFontSizeCurr = 0;
static int		iFontColorCurr = -1;
/* Current vertical position information */
static long		lYtopCurr = -1;
/* PostScript page counter */
static int		iPageCount = 0;
/* Image counter */
static int		iImageCount = 0;

static const char *iso_8859_1_data[] = {
"/newcodes	% ISO-8859-1 character encodings",
"[",
"140/ellipsis 141/trademark 142/perthousand 143/bullet",
"144/quoteleft 145/quoteright 146/guilsinglleft 147/guilsinglright",
"148/quotedblleft 149/quotedblright 150/quotedblbase 151/endash 152/emdash",
"153/minus 154/OE 155/oe 156/dagger 157/daggerdbl 158/fi 159/fl",
"160/space 161/exclamdown 162/cent 163/sterling 164/currency",
"165/yen 166/brokenbar 167/section 168/dieresis 169/copyright",
"170/ordfeminine 171/guillemotleft 172/logicalnot 173/hyphen 174/registered",
"175/macron 176/degree 177/plusminus 178/twosuperior 179/threesuperior",
"180/acute 181/mu 182/paragraph 183/periodcentered 184/cedilla",
"185/onesuperior 186/ordmasculine 187/guillemotright 188/onequarter",
"189/onehalf 190/threequarters 191/questiondown 192/Agrave 193/Aacute",
"194/Acircumflex 195/Atilde 196/Adieresis 197/Aring 198/AE 199/Ccedilla",
"200/Egrave 201/Eacute 202/Ecircumflex 203/Edieresis 204/Igrave 205/Iacute",
"206/Icircumflex 207/Idieresis 208/Eth 209/Ntilde 210/Ograve 211/Oacute",
"212/Ocircumflex 213/Otilde 214/Odieresis 215/multiply 216/Oslash",
"217/Ugrave 218/Uacute 219/Ucircumflex 220/Udieresis 221/Yacute 222/Thorn",
"223/germandbls 224/agrave 225/aacute 226/acircumflex 227/atilde",
"228/adieresis 229/aring 230/ae 231/ccedilla 232/egrave 233/eacute",
"234/ecircumflex 235/edieresis 236/igrave 237/iacute 238/icircumflex",
"239/idieresis 240/eth 241/ntilde 242/ograve 243/oacute 244/ocircumflex",
"245/otilde 246/odieresis 247/divide 248/oslash 249/ugrave 250/uacute",
"251/ucircumflex 252/udieresis 253/yacute 254/thorn 255/ydieresis",
"] bind def",
"",
"/reencdict 12 dict def",
"",
};

static const char *iso_8859_2_data[] = {
"/newcodes	% ISO-8859-2 character encodings",
"[",
"160/space 161/Aogonek 162/breve 163/Lslash 164/currency 165/Lcaron",
"166/Sacute 167/section 168/dieresis 169/Scaron 170/Scommaaccent",
"171/Tcaron 172/Zacute 173/hyphen 174/Zcaron 175/Zdotaccent 176/degree",
"177/aogonek 178/ogonek 179/lslash 180/acute 181/lcaron 182/sacute",
"183/caron 184/cedilla 185/scaron 186/scommaaccent 187/tcaron",
"188/zacute 189/hungarumlaut 190/zcaron 191/zdotaccent 192/Racute",
"193/Aacute 194/Acircumflex 195/Abreve 196/Adieresis 197/Lacute",
"198/Cacute 199/Ccedilla 200/Ccaron 201/Eacute 202/Eogonek",
"203/Edieresis 204/Ecaron 205/Iacute 206/Icircumflex 207/Dcaron",
"208/Dcroat 209/Nacute 210/Ncaron 211/Oacute 212/Ocircumflex",
"213/Ohungarumlaut 214/Odieresis 215/multiply 216/Rcaron 217/Uring",
"218/Uacute 219/Uhungarumlaut 220/Udieresis 221/Yacute 222/Tcommaaccent",
"223/germandbls 224/racute 225/aacute 226/acircumflex 227/abreve",
"228/adieresis 229/lacute 230/cacute 231/ccedilla 232/ccaron 233/eacute",
"234/eogonek 235/edieresis 236/ecaron 237/iacute 238/icircumflex",
"239/dcaron 240/dcroat 241/nacute 242/ncaron 243/oacute 244/ocircumflex",
"245/ohungarumlaut 246/odieresis 247/divide 248/rcaron 249/uring",
"250/uacute 251/uhungarumlaut 252/udieresis 253/yacute 254/tcommaaccent",
"255/dotaccent",
"] bind def",
"",
"/reencdict 12 dict def",
"",
};

static const char *iso_8859_x_func[] = {
"% change fonts using ISO-8859-x characters",
"/ChgFnt		% size psname natname => font",
"{",
"	dup FontDirectory exch known		% is re-encoded name known?",
"	{ exch pop }				% yes, get rid of long name",
"	{ dup 3 1 roll ReEncode } ifelse	% no, re-encode it",
"	findfont exch scalefont setfont",
"} bind def",
"",
"/ReEncode",
"{",
"reencdict begin",
"	/newname exch def",
"	/basename exch def",
"	/basedict basename findfont def",
"	/newfont basedict maxlength dict def",
"	basedict",
"	{ exch dup /FID ne",
"		{ dup /Encoding eq",
"			{ exch dup length array copy newfont 3 1 roll put }",
"			{ exch newfont 3 1 roll put } ifelse",
"		}",
"		{ pop pop } ifelse",
"	} forall",
"	newfont /FontName newname put",
"	newcodes aload pop newcodes length 2 idiv",
"	{ newfont /Encoding get 3 1 roll put } repeat",
"	newname newfont definefont pop",
"end",
"} bind def",
"",
};

static const char *misc_func[] = {
"% draw a line and show the string",
"/LineShow	% string linewidth movement",
"{",
"	gsave",
"		0 exch rmoveto",
"		setlinewidth",
"		dup",
"		stringwidth pop",
"		0 rlineto stroke",
"	grestore",
"	show",
"} bind def",
"",
"% begin an EPS file (level 2 and up)",
"/BeginEPSF",
"{",
"	/b4_Inc_state save def",
"	/dict_count countdictstack def",
"	/op_count count 1 sub def",
"	userdict begin",
"		/showpage { } def",
"		0 setgray 0 setlinecap",
"		1 setlinewidth 0 setlinejoin",
"		10 setmiterlimit [ ] 0 setdash newpath",
"		false setstrokeadjust false setoverprint",
"} bind def",
"",
"% end an EPS file",
"/EndEPSF {",
"	count op_count sub { pop } repeat",
"	countdictstack dict_count sub { end } repeat",
"	b4_Inc_state restore",
"} bind def",
"",
};


/*
 * vAddPageSetup - add the page setup
 */
static void
vAddPageSetup(FILE *pOutFile)
{
	if (bUseLandscape) {
		fprintf(pOutFile, "%%%%BeginPageSetup\n");
		fprintf(pOutFile, "90 rotate\n");
		fprintf(pOutFile, "0.00 %.2f translate\n",
					-dDrawUnits2Points(lPageHeight));
		fprintf(pOutFile, "%%%%EndPageSetup\n");
	}
} /* end of vAddPageSetup */

/*
 * vMove2NextPage - move to the start of the next page
 */
static void
vMove2NextPage(diagram_type *pDiag)
{
	fail(pDiag == NULL);

	fprintf(pDiag->pOutFile, "showpage\n");
	iPageCount++;
	fprintf(pDiag->pOutFile, "%%%%Page: %d %d\n", iPageCount, iPageCount);
	vAddPageSetup(pDiag->pOutFile);
	pDiag->lYtop = lPageHeight - PS_TOP_MARGIN;
	lYtopCurr = -1;
} /* end of vMove2NextPage */

/*
 * vMoveToPS - move to the specified X,Y coordinates
 *
 * Move the current position of the specified diagram to its X,Y coordinates,
 * start on a new page if needed
 */
static void
vMoveToPS(diagram_type *pDiag, long lLastVerticalMovement)
{
	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);

	if (pDiag->lYtop < PS_BOTTOM_MARGIN) {
		vMove2NextPage(pDiag);
		/* Repeat the last vertical movement on the new page */
		pDiag->lYtop -= lLastVerticalMovement;
	}
	fail(pDiag->lYtop < PS_BOTTOM_MARGIN);

	if (pDiag->lYtop != lYtopCurr) {
		fprintf(pDiag->pOutFile, "%.2f %.2f moveto\n",
			dDrawUnits2Points(pDiag->lXleft + PS_LEFT_MARGIN),
			dDrawUnits2Points(pDiag->lYtop));
		lYtopCurr = pDiag->lYtop;
	}
} /* end of vMoveToPS */

/*
 * vProloguePS - set options and perform the PostScript initialization
 */
void
vProloguePS(diagram_type *pDiag,
	const char *szTask, const char *szFilename,
	const options_type *pOptions)
{
	FILE	*pOutFile;
	const char	*szTmp;
	time_t	tTime;

	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);
	fail(szTask == NULL || szTask[0] == '\0');
	fail(pOptions == NULL);

	pOutFile = pDiag->pOutFile;

	bUseLandscape = pOptions->bUseLandscape;
	eEncoding = pOptions->eEncoding;
	eImageLevel = pOptions->eImageLevel;

	if (pOptions->iPageHeight == INT_MAX) {
		lPageHeight = LONG_MAX;
	} else {
		lPageHeight = lPoints2DrawUnits(pOptions->iPageHeight);
	}
	DBG_DEC(lPageHeight);
	if (pOptions->iPageWidth == INT_MAX) {
		lPageWidth = LONG_MAX;
	} else {
		lPageWidth = lPoints2DrawUnits(pOptions->iPageWidth);
	}
	DBG_DEC(lPageWidth);
	tFontRefCurr = (draw_fontref)-1;
	usFontSizeCurr = 0;
	iFontColorCurr = -1;
	lYtopCurr = -1;
	iImageCount = 0;
	iPageCount = 0;
	pDiag->lXleft = 0;
	pDiag->lYtop = lPageHeight - PS_TOP_MARGIN;

	szCreator = szTask;

	fprintf(pOutFile, "%%!PS-Adobe-2.0\n");
	fprintf(pOutFile, "%%%%Title: %s\n", szBasename(szFilename));
	fprintf(pOutFile, "%%%%Creator: %s %s\n", szCreator, VERSIONSTRING);
	szTmp = getenv("LOGNAME");
	if (szTmp == NULL || szTmp[0] == '\0') {
		szTmp = getenv("USER");
		if (szTmp == NULL || szTmp[0] == '\0') {
			szTmp = "unknown";
		}
	}
	fprintf(pOutFile, "%%%%For: %.50s\n", szTmp);
	errno = 0;
	tTime = time(NULL);
	if (tTime == (time_t)-1 && errno != 0) {
		szCreationDate = NULL;
	} else {
		szCreationDate = ctime(&tTime);
	}
	if (szCreationDate == NULL || szCreationDate[0] == '\0') {
		szCreationDate = "unknown\n";
	}
	fprintf(pOutFile, "%%%%CreationDate: %s", szCreationDate);
	if (bUseLandscape) {
		fprintf(pOutFile, "%%%%Orientation: Landscape\n");
		fprintf(pOutFile, "%%%%BoundingBox: 0 0 %.0f %.0f\n",
				dDrawUnits2Points(lPageHeight),
				dDrawUnits2Points(lPageWidth));
	} else {
		fprintf(pOutFile, "%%%%Orientation: Portrait\n");
		fprintf(pOutFile, "%%%%BoundingBox: 0 0 %.0f %.0f\n",
				dDrawUnits2Points(lPageWidth),
				dDrawUnits2Points(lPageHeight));
	}
} /* end of vProloguePS */

/*
 * vEpiloguePS - clean up after everything is done
 */
void
vEpiloguePS(diagram_type *pDiag)
{
	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);

	if (pDiag->lYtop < lPageHeight - PS_TOP_MARGIN) {
		fprintf(pDiag->pOutFile, "showpage\n");
	}
	fprintf(pDiag->pOutFile, "%%%%Trailer\n");
	fprintf(pDiag->pOutFile, "%%%%Pages: %d\n", iPageCount);
	fprintf(pDiag->pOutFile, "%%%%EOF\n");
	szCreationDate = NULL;
	szCreator = NULL;
} /* end of vEpiloguePS */

/*
 * vPrintPalette - print a postscript palette
 */
static void
vPrintPalette(FILE *pOutFile, const imagedata_type *pImg)
{
	int	iIndex;

	fail(pOutFile == NULL);
	fail(pImg == NULL);
	fail(pImg->iColorsUsed < 2);
	fail(pImg->iColorsUsed > 256);

	fprintf(pOutFile, "[ /Indexed\n");
	fprintf(pOutFile, "\t/Device%s %d\n",
		pImg->bColorImage ? "RGB" : "Gray", pImg->iColorsUsed - 1);
	fprintf(pOutFile, "<");
	for (iIndex = 0; iIndex < pImg->iColorsUsed; iIndex++) {
		fprintf(pOutFile, "%02x",
				(unsigned int)pImg->aucPalette[iIndex][0]);
		if (pImg->bColorImage) {
			fprintf(pOutFile, "%02x%02x",
				(unsigned int)pImg->aucPalette[iIndex][1],
				(unsigned int)pImg->aucPalette[iIndex][2]);
		}
		if (iIndex % 8 == 7) {
			fprintf(pOutFile, "\n");
		} else {
			fprintf(pOutFile, " ");
		}
	}
	fprintf(pOutFile, ">\n");
	fprintf(pOutFile, "] setcolorspace\n");
} /* end of vPrintPalette */

/*
 * vImageProloguePS - perform the Encapsulated PostScript initialization
 */
void
vImageProloguePS(diagram_type *pDiag, const imagedata_type *pImg)
{
	FILE	*pOutFile;

	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);
	fail(pImg == NULL);

	if (pImg->iVerSizeScaled <= 0 || pImg->iHorSizeScaled <= 0) {
		return;
	}

	fail(szCreationDate == NULL);
	fail(szCreator == NULL);
	fail(eImageLevel == level_no_images);

	iImageCount++;

	DBG_DEC_C(pDiag->lXleft != 0, pDiag->lXleft);

	pDiag->lYtop -= lPoints2DrawUnits(pImg->iVerSizeScaled);
	vMoveToPS(pDiag, lPoints2DrawUnits(pImg->iVerSizeScaled));

	pOutFile = pDiag->pOutFile;

	fprintf(pOutFile, "BeginEPSF\n");
	fprintf(pOutFile, "%%%%BeginDocument: image%03d.eps\n", iImageCount);
	fprintf(pOutFile, "%%!PS-Adobe-2.0 EPSF-2.0\n");
	fprintf(pOutFile, "%%%%Creator: %s %s\n", szCreator, VERSIONSTRING);
	fprintf(pOutFile, "%%%%Title: Image %03d\n", iImageCount);
	fprintf(pOutFile, "%%%%CreationDate: %s", szCreationDate);
	fprintf(pOutFile, "%%%%BoundingBox: 0 0 %d %d\n",
				pImg->iHorSizeScaled, pImg->iVerSizeScaled);
	fprintf(pOutFile, "%%%%DocumentData: Clean7Bit\n");
	fprintf(pOutFile, "%%%%LanguageLevel: 2\n");
	fprintf(pOutFile, "%%%%EndComments\n");
	fprintf(pOutFile, "%%%%BeginProlog\n");
	fprintf(pOutFile, "%%%%EndProlog\n");
	fprintf(pOutFile, "%%%%Page: 1 1\n");

	fprintf(pOutFile, "save\n");

	switch (pImg->eImageType) {
	case imagetype_is_jpeg:
		fprintf(pOutFile, "/Data1 currentfile ");
		fprintf(pOutFile, "/ASCII85Decode filter def\n");
		fprintf(pOutFile, "/Data Data1 << ");
		fprintf(pOutFile, ">> /DCTDecode filter def\n");
		switch (pImg->iComponents) {
		case 1:
			fprintf(pOutFile, "/DeviceGray setcolorspace\n");
			break;
		case 3:
			fprintf(pOutFile, "/DeviceRGB setcolorspace\n");
			break;
		case 4:
			fprintf(pOutFile, "/DeviceCMYK setcolorspace\n");
			break;
		default:
			DBG_DEC(pImg->iComponents);
			break;
		}
		break;
	case imagetype_is_png:
		if (eImageLevel == level_gs_special) {
			fprintf(pOutFile,
			"/Data2 currentfile /ASCII85Decode filter def\n");
			fprintf(pOutFile,
			"/Data1 Data2 << >> /FlateDecode filter def\n");
			fprintf(pOutFile, "/Data Data1 <<\n");
			fprintf(pOutFile, "\t/Colors %d\n", pImg->iComponents);
			fprintf(pOutFile, "\t/BitsPerComponent %u\n",
						pImg->uiBitsPerComponent);
			fprintf(pOutFile, "\t/Columns %d\n", pImg->iWidth);
			fprintf(pOutFile,
				">> /PNGPredictorDecode filter def\n");
		} else {
			fprintf(pOutFile,
			"/Data1 currentfile /ASCII85Decode filter def\n");
			fprintf(pOutFile,
			"/Data Data1 << >> /FlateDecode filter def\n");
		}
		if (pImg->iComponents == 3) {
			fprintf(pOutFile, "/DeviceRGB setcolorspace\n");
		} else if (pImg->iColorsUsed > 0) {
			vPrintPalette(pOutFile, pImg);
		} else {
			fprintf(pOutFile, "/DeviceGray setcolorspace\n");
		}
		break;
	case imagetype_is_dib:
		fprintf(pOutFile, "/Data currentfile ");
		fprintf(pOutFile, "/ASCII85Decode filter def\n");
		if (pImg->uiBitsPerComponent <= 8) {
			vPrintPalette(pOutFile, pImg);
		} else {
			fprintf(pOutFile, "/DeviceRGB setcolorspace\n");
		}
		break;
	default:
		fprintf(pOutFile, "/Data currentfile ");
		fprintf(pOutFile, "/ASCIIHexDecode filter def\n");
		fprintf(pOutFile, "/Device%s setcolorspace\n",
			pImg->bColorImage ? "RGB" : "Gray");
		break;
	}

	/* Translate to lower left corner of image */
	fprintf(pOutFile, "%.2f %.2f translate\n",
			dDrawUnits2Points(pDiag->lXleft + PS_LEFT_MARGIN),
			dDrawUnits2Points(pDiag->lYtop));

	fprintf(pOutFile, "%d %d scale\n",
				pImg->iHorSizeScaled, pImg->iVerSizeScaled);

	fprintf(pOutFile, "{ <<\n");
	fprintf(pOutFile, "\t/ImageType 1\n");
	fprintf(pOutFile, "\t/Width %d\n", pImg->iWidth);
	fprintf(pOutFile, "\t/Height %d\n", pImg->iHeight);
	if (pImg->eImageType == imagetype_is_dib) {
		/* Scanning from left to right and bottom to top */
		fprintf(pOutFile, "\t/ImageMatrix [ %d 0 0 %d 0 0 ]\n",
			pImg->iWidth, pImg->iHeight);
	} else {
		/* Scanning from left to right and top to bottom */
		fprintf(pOutFile, "\t/ImageMatrix [ %d 0 0 %d 0 %d ]\n",
			pImg->iWidth, -pImg->iHeight, pImg->iHeight);
	}
	fprintf(pOutFile, "\t/DataSource Data\n");

	switch (pImg->eImageType) {
	case imagetype_is_jpeg:
		fprintf(pOutFile, "\t/BitsPerComponent 8\n");
		switch (pImg->iComponents) {
		case 1:
			fprintf(pOutFile, "\t/Decode [0 1]\n");
			break;
		case 3:
			fprintf(pOutFile, "\t/Decode [0 1 0 1 0 1]\n");
			break;
		case 4:
			if (pImg->bAdobe) {
				/*
				 * Adobe-conforming CMYK file
				 * applying workaround for color inversion
				 */
				fprintf(pOutFile,
					"\t/Decode [1 0 1 0 1 0 1 0]\n");
			} else {
				fprintf(pOutFile,
					"\t/Decode [0 1 0 1 0 1 0 1]\n");
			}
			break;
		default:
			DBG_DEC(pImg->iComponents);
			break;
		}
		break;
	case imagetype_is_png:
		if (pImg->iComponents == 3) {
			fprintf(pOutFile, "\t/BitsPerComponent 8\n");
			fprintf(pOutFile, "\t/Decode [0 1 0 1 0 1]\n");
		} else if (pImg->iColorsUsed > 0) {
			fail(pImg->uiBitsPerComponent > 8);
			fprintf(pOutFile, "\t/BitsPerComponent %u\n",
					pImg->uiBitsPerComponent);
			fprintf(pOutFile, "\t/Decode [0 %d]\n",
					(1 << pImg->uiBitsPerComponent) - 1);
		} else {
			fprintf(pOutFile, "\t/BitsPerComponent 8\n");
			fprintf(pOutFile, "\t/Decode [0 1]\n");
		}
		break;
	case imagetype_is_dib:
		fprintf(pOutFile, "\t/BitsPerComponent 8\n");
		if (pImg->uiBitsPerComponent <= 8) {
			fprintf(pOutFile, "\t/Decode [0 255]\n");
		} else {
			fprintf(pOutFile, "\t/Decode [0 1 0 1 0 1]\n");
		}
		break;
	default:
		fprintf(pOutFile, "\t/BitsPerComponent 8\n");
		if (pImg->bColorImage) {
			fprintf(pOutFile, "\t/Decode [0 1 0 1 0 1]\n");
		} else {
			fprintf(pOutFile, "\t/Decode [0 1]\n");
		}
		break;
	}

	fprintf(pOutFile, "  >> image\n");
	fprintf(pOutFile, "  Data closefile\n");
	fprintf(pOutFile, "  showpage\n");
	fprintf(pOutFile, "  restore\n");
	fprintf(pOutFile, "} exec\n");
} /* end of vImageProloguePS */

/*
 * vImageEpiloguePS - clean up after Encapsulated PostScript
 */
void
vImageEpiloguePS(diagram_type *pDiag)
{
	FILE	*pOutFile;

	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);

	pOutFile = pDiag->pOutFile;

	fprintf(pOutFile, "%%%%EOF\n");
	fprintf(pOutFile, "%%%%EndDocument\n");
	fprintf(pOutFile, "EndEPSF\n");

	pDiag->lXleft = 0;
} /* end of vImageEpiloguePS */

/*
 * bAddDummyImagePS - add a dummy image
 *
 * return TRUE when successful, otherwise FALSE
 */
BOOL
bAddDummyImagePS(diagram_type *pDiag, const imagedata_type *pImg)
{
	FILE	*pOutFile;

	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);
	fail(pImg == NULL);

	if (pImg->iVerSizeScaled <= 0 || pImg->iHorSizeScaled <= 0) {
		return FALSE;
	}

	iImageCount++;

	DBG_DEC_C(pDiag->lXleft != 0, pDiag->lXleft);

	pDiag->lYtop -= lPoints2DrawUnits(pImg->iVerSizeScaled);
	vMoveToPS(pDiag, lPoints2DrawUnits(pImg->iVerSizeScaled));

	pOutFile = pDiag->pOutFile;

	fprintf(pOutFile, "gsave %% Image %03d\n", iImageCount);
	fprintf(pOutFile, "\tnewpath\n");
	fprintf(pOutFile, "\t%.2f %.2f moveto\n",
			dDrawUnits2Points(pDiag->lXleft + PS_LEFT_MARGIN),
			dDrawUnits2Points(pDiag->lYtop));
	fprintf(pOutFile, "\t1.0 setlinewidth\n");
	fprintf(pOutFile, "\t0.3 setgray\n");
	fprintf(pOutFile, "\t0 %d rlineto\n", pImg->iVerSizeScaled);
	fprintf(pOutFile, "\t%d 0 rlineto\n", pImg->iHorSizeScaled);
	fprintf(pOutFile, "\t0 %d rlineto\n", -pImg->iVerSizeScaled);
	fprintf(pOutFile, "\tclosepath\n");
	fprintf(pOutFile, "\tstroke\n");
	fprintf(pOutFile, "grestore\n");

	pDiag->lXleft = 0;

	return TRUE;
} /* end of bAddDummyImagePS */

/*
 * vAddFontsPS - add the list of fonts and complete the prologue
 */
void
vAddFontsPS(diagram_type *pDiag)
{
	FILE	*pOutFile;
	const font_table_type *pTmp, *pTmp2;
	size_t	tIndex;
	int	iLineLen;
	BOOL	bFound;

	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);

	pOutFile = pDiag->pOutFile;
	iLineLen = fprintf(pOutFile, "%%%%DocumentFonts:");

	if (tGetFontTableLength() == 0) {
		iLineLen += fprintf(pOutFile, " Courier");
	} else {
		pTmp = NULL;
		while ((pTmp = pGetNextFontTableRecord(pTmp)) != NULL) {
			/* Print the document fonts */
			bFound = FALSE;
			pTmp2 = NULL;
			while ((pTmp2 = pGetNextFontTableRecord(pTmp2))
					!= NULL && pTmp2 < pTmp) {
				bFound = STREQ(pTmp2->szOurFontname,
						pTmp->szOurFontname);
				if (bFound) {
					break;
				}
			}
			if (bFound) {
				continue;
			}
			if (iLineLen + (int)strlen(pTmp->szOurFontname) > 78) {
				fprintf(pOutFile, "\n%%%%+");
				iLineLen = 3;
			}
			iLineLen += fprintf(pOutFile,
					" %s", pTmp->szOurFontname);
		}
	}
	fprintf(pOutFile, "\n");
	fprintf(pOutFile, "%%%%Pages: (atend)\n");
	fprintf(pOutFile, "%%%%EndComments\n");
	fprintf(pOutFile, "%%%%BeginProlog\n");

	switch (eEncoding) {
	case encoding_iso_8859_1:
		for (tIndex = 0;
		     tIndex < elementsof(iso_8859_1_data);
		     tIndex++) {
			fprintf(pOutFile, "%s\n", iso_8859_1_data[tIndex]);
		}
		fprintf(pOutFile, "\n");
		for (tIndex = 0;
		     tIndex < elementsof(iso_8859_x_func);
		     tIndex++) {
			fprintf(pOutFile, "%s\n", iso_8859_x_func[tIndex]);
		}
		break;
	case encoding_iso_8859_2:
		for (tIndex = 0;
		     tIndex < elementsof(iso_8859_2_data);
		     tIndex++) {
			fprintf(pOutFile, "%s\n", iso_8859_2_data[tIndex]);
		}
		fprintf(pOutFile, "\n");
		for (tIndex = 0;
		     tIndex < elementsof(iso_8859_x_func);
		     tIndex++) {
			fprintf(pOutFile, "%s\n", iso_8859_x_func[tIndex]);
		}
		break;
	case encoding_utf8:
		werr(1,
		"The combination PostScript and UTF-8 is not supported");
		break;
	default:
		DBG_DEC(eEncoding);
		break;
	}

	/* The rest of the functions */
	for (tIndex = 0; tIndex < elementsof(misc_func); tIndex++) {
		fprintf(pOutFile, "%s\n", misc_func[tIndex]);
	}
	fprintf(pOutFile, "%%%%EndProlog\n");
	iPageCount = 1;
	fprintf(pDiag->pOutFile, "%%%%Page: %d %d\n", iPageCount, iPageCount);
	vAddPageSetup(pDiag->pOutFile);
} /* end of vAddFontsPS */

/*
 * vPrintPS - print a PostScript string
 */
static void
vPrintPS(FILE *pFile, const char *szString, size_t tStringLength,
		USHORT usFontstyle)
{
	double		dSuperscriptMove, dSubscriptMove;
	const UCHAR	*ucBytes;
	size_t		tCount;

	fail(szString == NULL);

	if (szString == NULL || szString[0] == '\0' || tStringLength == 0) {
		return;
	}
	DBG_DEC_C(usFontSizeCurr < MIN_FONT_SIZE, usFontSizeCurr);

	dSuperscriptMove = 0.0;
	dSubscriptMove = 0.0;

	/* Up for superscript */
	if (bIsSuperscript(usFontstyle) && usFontSizeCurr != 0) {
		dSuperscriptMove = (double)((usFontSizeCurr + 1) / 2) * 0.375;
		fprintf(pFile, "0 %.2f rmoveto\n", dSuperscriptMove);
	}

	/* Down for subscript */
	if (bIsSubscript(usFontstyle) && usFontSizeCurr != 0) {
		dSubscriptMove = (double)usFontSizeCurr * 0.125;
		fprintf(pFile, "0 %.2f rmoveto\n", -dSubscriptMove);
	}

	/* Generate and print the PostScript output */
	ucBytes = (UCHAR *)szString;
	(void)putc('(', pFile);
	for (tCount = 0; tCount < tStringLength ; tCount++) {
		switch (ucBytes[tCount]) {
		case '(':
		case ')':
		case '\\':
			(void)putc('\\', pFile);
			(void)putc(szString[tCount], pFile);
			break;
		default:
			if (ucBytes[tCount] < 0x20 ||
			    (ucBytes[tCount] >= 0x7f &&
			     ucBytes[tCount] < 0x8c)) {
				DBG_HEX(ucBytes[tCount]);
				(void)putc(' ', pFile);
			} else if (ucBytes[tCount] >= 0x80) {
				fprintf(pFile, "\\%03o", (UINT)ucBytes[tCount]);
			} else {
				(void)putc(szString[tCount], pFile);
			}
			break;
		}
	}
	fprintf(pFile, ") ");
	if ((bIsStrike(usFontstyle) || bIsMarkDel(usFontstyle)) &&
			usFontSizeCurr != 0) {
		fprintf(pFile, "%.2f %.2f LineShow\n",
			(double)usFontSizeCurr * 0.02,
			(double)usFontSizeCurr * 0.12);
	} else if (bIsUnderline(usFontstyle) && usFontSizeCurr != 0) {
		fprintf(pFile, "%.2f %.2f LineShow\n",
			(double)usFontSizeCurr * 0.02,
			(double)usFontSizeCurr * -0.06);
	} else {
		fprintf(pFile, "show\n");
	}

	/* Undo the superscript move */
	if (bIsSuperscript(usFontstyle) && usFontSizeCurr != 0) {
		fprintf(pFile, "0 %.2f rmoveto\n", -dSuperscriptMove);
	}

	/* Undo the subscript move */
	if (bIsSubscript(usFontstyle) && usFontSizeCurr != 0) {
		fprintf(pFile, "0 %.2f rmoveto\n", dSubscriptMove);
	}
} /* end of vPrintPS */

/*
 * vSetColor - move to the specified color
 */
static void
vSetColor(FILE *pFile, UCHAR ucFontColor)
{
	ULONG	ulTmp, ulRed, ulGreen, ulBlue;

	ulTmp = ulColor2Color(ucFontColor);
	ulRed   = (ulTmp & 0x0000ff00) >> 8;
	ulGreen = (ulTmp & 0x00ff0000) >> 16;
	ulBlue  = (ulTmp & 0xff000000) >> 24;
	fprintf(pFile, "%.3f %.3f %.3f setrgbcolor\n",
				ulRed / 255.0, ulGreen / 255.0, ulBlue / 255.0);
} /* end of vSetColor */

/*
 * vMove2NextLinePS - move to the next line
 */
void
vMove2NextLinePS(diagram_type *pDiag, USHORT usFontSize)
{
	fail(pDiag == NULL);
	fail(usFontSize < MIN_FONT_SIZE || usFontSize > MAX_FONT_SIZE);

	pDiag->lYtop -= lComputeLeading(usFontSize);
} /* end of vMove2NextLinePS */

/*
 * vSubstringPS - print a sub string
 */
void
vSubstringPS(diagram_type *pDiag,
	char *szString, size_t tStringLength, long lStringWidth,
	UCHAR ucFontColor, USHORT usFontstyle, draw_fontref tFontRef,
	USHORT usFontSize, USHORT usMaxFontSize)
{
	const char	*szOurFontname;

	fail(pDiag == NULL || szString == NULL);
	fail(pDiag->pOutFile == NULL);
	fail(pDiag->lXleft < 0);
	fail(tStringLength != strlen(szString));
	fail(usFontSize < MIN_FONT_SIZE || usFontSize > MAX_FONT_SIZE);
	fail(usMaxFontSize < MIN_FONT_SIZE || usMaxFontSize > MAX_FONT_SIZE);
	fail(usFontSize > usMaxFontSize);

	if (szString[0] == '\0' || tStringLength == 0) {
		return;
	}

	if (tFontRef != tFontRefCurr || usFontSize != usFontSizeCurr) {
		szOurFontname = szGetFontname(tFontRef);
		fail(szOurFontname == NULL);
		fprintf(pDiag->pOutFile,
			"%.1f /%s /%s-ISO-8859-x ChgFnt\n",
			(double)usFontSize / 2.0,
			szOurFontname, szOurFontname);
		tFontRefCurr = tFontRef;
		usFontSizeCurr = usFontSize;
	}
	if ((int)ucFontColor != iFontColorCurr) {
		vSetColor(pDiag->pOutFile, ucFontColor);
		iFontColorCurr = (int)ucFontColor;
	}
	vMoveToPS(pDiag, lComputeLeading(usMaxFontSize));
	vPrintPS(pDiag->pOutFile, szString, tStringLength, usFontstyle);
	pDiag->lXleft += lStringWidth;
} /* end of vSubstringPS */

/*
 * Create an start of paragraph by moving the y-top mark
 */
void
vStartOfParagraphPS(diagram_type *pDiag, long lBeforeIndentation)
{
	fail(pDiag == NULL);
	fail(lBeforeIndentation < 0);

	pDiag->lXleft = 0;
	pDiag->lYtop -= lMilliPoints2DrawUnits(lBeforeIndentation);
} /* end of vStartOfParagraphPS */

/*
 * Create an end of paragraph by moving the y-top mark
 */
void
vEndOfParagraphPS(diagram_type *pDiag,
	draw_fontref tFontRef, USHORT usFontSize, long lAfterIndentation)
{
	fail(pDiag == NULL);
	fail(pDiag->pOutFile == NULL);
	fail(usFontSize < MIN_FONT_SIZE || usFontSize > MAX_FONT_SIZE);
	fail(lAfterIndentation < 0);

	if (pDiag->lXleft > 0) {
		/* To the start of the line */
		vMove2NextLine(pDiag, tFontRef, usFontSize);
	}

	pDiag->lXleft = 0;
	pDiag->lYtop -= lMilliPoints2DrawUnits(lAfterIndentation);
} /* end of vEndOfParagraphPS */

/*
 * Create an end of page
 */
void
vEndOfPagePS(diagram_type *pDiag)
{
	vMove2NextPage(pDiag);
} /* end of vEndOfPagePS */
