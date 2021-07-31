/*
 * options.c
 * Copyright (C) 1998-2003 A.J. van Os; Released under GPL
 *
 * Description:
 * Read and write the options
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__riscos)
#include "wimpt.h"
#else
#include <stdlib.h>
#if defined(__dos)
extern int getopt(int, char **, const char *);
#else
#include <unistd.h>
#endif /* __dos */
#endif /* __riscos */
#include "antiword.h"

#if defined(__riscos)
#define PARAGRAPH_BREAK		"set paragraph_break=%d"
#define AUTOFILETYPE		"set autofiletype_allowed=%d"
#define USE_OUTLINEFONTS	"set use_outlinefonts=%d"
#define SHOW_IMAGES		"set show_images=%d"
#define HIDE_HIDDEN_TEXT	"set hide_hidden_text=%d"
#define SCALE_FACTOR_START	"set scale_factor_start=%d"
#endif /* __riscos */

/* Current values for options */
static options_type	tOptionsCurr;
#if defined(__riscos)
/* Temporary values for options */
static options_type	tOptionsTemp;
#else
typedef struct papersize_tag {
	char	szName[16];	/* Papersize name */
	USHORT	usWidth;	/* In points */
	USHORT	usHeight;	/* In points */
} papersize_type;

static const papersize_type atPaperSizes[] = {
	{	"10x14",	 720,	1008	},
	{	"a3",		 842,	1191	},
	{	"a4",		 595,	 842	},
	{	"a5",		 420,	 595	},
	{	"b4",		 729,	1032	},
	{	"b5",		 516,	 729	},
	{	"executive",	 540,	 720	},
	{	"folio",	 612,	 936	},
	{	"legal",	 612,	1008	},
	{	"letter",	 612,	 792	},
	{	"note",		 540,	 720	},
	{	"quarto",	 610,	 780	},
	{	"statement",	 396,	 612	},
	{	"tabloid",	 792,	1224	},
	{	"",		   0,	   0	},
};
#endif /* __riscos */
/* Default values for options */
static const options_type	tOptionsDefault = {
	DEFAULT_SCREEN_WIDTH,
#if defined(__riscos)
	conversion_draw,
#else
	conversion_text,
#endif /* __riscos */
	TRUE,
	FALSE,
	encoding_iso_8859_1,
	INT_MAX,
	INT_MAX,
	level_default,
#if defined(__riscos)
	TRUE,
	DEFAULT_SCALE_FACTOR,
#endif /* __riscos */
};


/*
 * iReadOptions - read options
 *
 * returns:	-1: error
 *		 0: help
 *		>0: index first file argument
 */
int
iReadOptions(int argc, char **argv)
{
#if defined(__riscos)
	FILE	*pFile;
	const char	*szAlphabet;
	int	iAlphabet;
	char	szLine[81];
#else
	extern	char	*optarg;
	extern int	optind;
	const papersize_type	*pPaperSize;
	const char	*szHome, *szAntiword;
	char	*pcChar, *szTmp;
	int	iChar;
	BOOL	bFound;
	char	szLeafname[32+1];
	char	szMappingFile[PATH_MAX+1];
#endif /* __riscos */
	int	iTmp;

	DBG_MSG("iReadOptions");

/* Defaults */
	tOptionsCurr = tOptionsDefault;

#if defined(__riscos)
/* Choices file */
	pFile = fopen("<AntiWord$ChoicesFile>", "r");
	DBG_MSG_C(pFile == NULL, "Choices file not found");
	DBG_HEX_C(pFile != NULL, pFile);
	if (pFile != NULL) {
		while (fgets(szLine, (int)sizeof(szLine), pFile) != NULL) {
			DBG_MSG(szLine);
			if (szLine[0] == '#' ||
			    szLine[0] == '\r' ||
			    szLine[0] == '\n') {
				continue;
			}
			if (sscanf(szLine, PARAGRAPH_BREAK, &iTmp) == 1 &&
			    (iTmp == 0 ||
			    (iTmp >= MIN_SCREEN_WIDTH &&
			     iTmp <= MAX_SCREEN_WIDTH))) {
				tOptionsCurr.iParagraphBreak = iTmp;
				DBG_DEC(tOptionsCurr.iParagraphBreak);
			} else if (sscanf(szLine, AUTOFILETYPE, &iTmp)
								== 1) {
				tOptionsCurr.bAutofiletypeAllowed =
								iTmp != 0;
				DBG_DEC(tOptionsCurr.bAutofiletypeAllowed);
			} else if (sscanf(szLine, USE_OUTLINEFONTS, &iTmp)
								== 1) {
				tOptionsCurr.eConversionType =
					iTmp == 0 ?
					conversion_text : conversion_draw;
				DBG_DEC(tOptionsCurr.eConversionType);
			} else if (sscanf(szLine, SHOW_IMAGES, &iTmp)
								== 1) {
				tOptionsCurr.eImageLevel = iTmp != 0 ?
					level_default : level_no_images;
			} else if (sscanf(szLine, HIDE_HIDDEN_TEXT, &iTmp)
								== 1) {
				tOptionsCurr.bHideHiddenText = iTmp != 0;
				DBG_DEC(tOptionsCurr.bHideHiddenText);
			} else if (sscanf(szLine, SCALE_FACTOR_START, &iTmp)
								== 1) {
				if (iTmp >= MIN_SCALE_FACTOR &&
				    iTmp <= MAX_SCALE_FACTOR) {
					tOptionsCurr.iScaleFactor = iTmp;
					DBG_DEC(tOptionsCurr.iScaleFactor);
				}
			}
		}
		(void)fclose(pFile);
	}
	iAlphabet = iReadCurrentAlphabetNumber();
	switch (iAlphabet) {
	case 101:	/* ISO-8859-1 aka Latin1 */
		szAlphabet = "<AntiWord$Latin1>";
		break;
	case 112:	/* ISO-8859-15 aka Latin9 */
		szAlphabet = "<AntiWord$Latin9>";
		break;
	default:
		werr(0, "Alphabet '%d' is not supported", iAlphabet);
		return -1;
	}
	if (bReadCharacterMappingTable(szAlphabet)) {
		return 1;
	}
	return -1;
#else
/* Environment */
	szTmp = getenv("COLUMNS");
	if (szTmp != NULL) {
		DBG_MSG(szTmp);
		iTmp = (int)strtol(szTmp, &pcChar, 10);
		if (*pcChar == '\0') {
			iTmp -= 4;	/* This is for the edge */
			if (iTmp < MIN_SCREEN_WIDTH) {
				iTmp = MIN_SCREEN_WIDTH;
			} else if (iTmp > MAX_SCREEN_WIDTH) {
				iTmp = MAX_SCREEN_WIDTH;
			}
			tOptionsCurr.iParagraphBreak = iTmp;
			DBG_DEC(tOptionsCurr.iParagraphBreak);
		}
	}
	if (is_locale_utf8()) {
		tOptionsCurr.eEncoding = encoding_utf8;
		strcpy(szLeafname, MAPPING_FILE_DEFAULT_8);
	} else {
		tOptionsCurr.eEncoding = encoding_iso_8859_1;
		strcpy(szLeafname, MAPPING_FILE_DEFAULT_1);
	}
/* Command line */
	while ((iChar = getopt(argc, argv, "Lhi:m:p:stw:x:")) != -1) {
		switch (iChar) {
		case 'L':
			tOptionsCurr.bUseLandscape = TRUE;
			break;
		case 'h':
			return 0;
		case 'i':
			iTmp = (int)strtol(optarg, &pcChar, 10);
			if (*pcChar != '\0') {
				break;
			}
			switch (iTmp) {
			case 0:
				tOptionsCurr.eImageLevel = level_gs_special;
				break;
			case 1:
				tOptionsCurr.eImageLevel = level_no_images;
				break;
			case 2:
				tOptionsCurr.eImageLevel = level_ps_2;
				break;
			case 3:
				tOptionsCurr.eImageLevel = level_ps_3;
				break;
			default:
				tOptionsCurr.eImageLevel = level_default;
				break;
			}
			DBG_DEC(tOptionsCurr.eImageLevel);
			break;
		case 'm':
			if (tOptionsCurr.eConversionType == conversion_xml) {
				werr(0, "XML doesn't need a mapping file");
				break;
			}
			strncpy(szLeafname, optarg, sizeof(szLeafname) - 1);
			szLeafname[sizeof(szLeafname) - 1] = '\0';
			DBG_MSG(szLeafname);
			if (STRCEQ(szLeafname, MAPPING_FILE_DEFAULT_8)) {
				tOptionsCurr.eEncoding = encoding_utf8;
			} else if (STRCEQ(szLeafname, MAPPING_FILE_DEFAULT_2)) {
				tOptionsCurr.eEncoding = encoding_iso_8859_2;
			} else {
				tOptionsCurr.eEncoding = encoding_iso_8859_1;
			}
			DBG_DEC(tOptionsCurr.eEncoding);
			break;
		case 'p':
			bFound = FALSE;
			for (pPaperSize = atPaperSizes;
			     pPaperSize->szName[0] != '\0';
			     pPaperSize++) {
				if (!STREQ(pPaperSize->szName,  optarg)) {
					continue;
				}
				DBG_DEC(pPaperSize->usWidth);
				DBG_DEC(pPaperSize->usHeight);
				tOptionsCurr.eConversionType = conversion_ps;
				tOptionsCurr.iPageHeight =
						(int)pPaperSize->usHeight;
				tOptionsCurr.iPageWidth =
						(int)pPaperSize->usWidth;
				bFound = TRUE;
				break;
			}
			if (!bFound) {
				werr(0, "-p without a valid papersize");
				return -1;
			}
			break;
		case 's':
			tOptionsCurr.bHideHiddenText = FALSE;
			break;
		case 't':
			tOptionsCurr.eConversionType = conversion_text;
			break;
		case 'w':
			iTmp = (int)strtol(optarg, &pcChar, 10);
			if (*pcChar == '\0') {
				if (iTmp != 0 && iTmp < MIN_SCREEN_WIDTH) {
					iTmp = MIN_SCREEN_WIDTH;
				} else if (iTmp > MAX_SCREEN_WIDTH) {
					iTmp = MAX_SCREEN_WIDTH;
				}
				tOptionsCurr.iParagraphBreak = iTmp;
				DBG_DEC(tOptionsCurr.iParagraphBreak);
			}
			break;
		case 'x':
			if (STREQ(optarg, "db")) {
				tOptionsCurr.iParagraphBreak = 0;
				tOptionsCurr.eConversionType = conversion_xml;
				tOptionsCurr.eEncoding = encoding_utf8;
			} else {
				werr(0, "-x %s is not supported", optarg);
				return -1;
			}
			break;
		default:
			return -1;
		}
	}

	if (tOptionsCurr.eConversionType == conversion_ps &&
	    tOptionsCurr.eEncoding == encoding_utf8) {
		werr(0,
		"The combination PostScript and UTF-8 is not supported");
		return -1;
	}

	if (tOptionsCurr.eConversionType == conversion_ps) {
		/* PostScript mode */
		if (tOptionsCurr.bUseLandscape) {
			/* Swap the page height and width */
			iTmp = tOptionsCurr.iPageHeight;
			tOptionsCurr.iPageHeight = tOptionsCurr.iPageWidth;
			tOptionsCurr.iPageWidth = iTmp;
		}
		/* The paragraph break depends on the width of the paper */
		tOptionsCurr.iParagraphBreak = iMilliPoints2Char(
			(long)tOptionsCurr.iPageWidth * 1000 -
			lDrawUnits2MilliPoints(
				PS_LEFT_MARGIN + PS_RIGHT_MARGIN));
		DBG_DEC(tOptionsCurr.iParagraphBreak);
	}

	/* Try the environment version of the mapping file */
	szAntiword = szGetAntiwordDirectory();
	if (szAntiword != NULL && szAntiword[0] != '\0') {
	    if (strlen(szAntiword) + strlen(szLeafname) <
		sizeof(szMappingFile) -
		sizeof(FILE_SEPARATOR)) {
			sprintf(szMappingFile,
				"%s" FILE_SEPARATOR "%s",
				szAntiword, szLeafname);
			DBG_MSG(szMappingFile);
			if (bReadCharacterMappingTable(szMappingFile)) {
				return optind;
			}
		} else {
			werr(0, "Environment mappingfilename ignored");
		}
	}
	/* Try the local version of the mapping file */
	szHome = szGetHomeDirectory();
	if (strlen(szHome) + strlen(szLeafname) <
	    sizeof(szMappingFile) -
	    sizeof(ANTIWORD_DIR) -
	    2 * sizeof(FILE_SEPARATOR)) {
		sprintf(szMappingFile,
			"%s" FILE_SEPARATOR ANTIWORD_DIR FILE_SEPARATOR "%s",
			szHome, szLeafname);
		DBG_MSG(szMappingFile);
		if (bReadCharacterMappingTable(szMappingFile)) {
			return optind;
		}
	} else {
		werr(0, "Local mappingfilename too long, ignored");
	}
	/* Try the global version of the mapping file */
	if (strlen(szLeafname) <
	    sizeof(szMappingFile) -
	    sizeof(GLOBAL_ANTIWORD_DIR) -
	    sizeof(FILE_SEPARATOR)) {
		sprintf(szMappingFile,
			GLOBAL_ANTIWORD_DIR FILE_SEPARATOR "%s",
			szLeafname);
		DBG_MSG(szMappingFile);
		if (bReadCharacterMappingTable(szMappingFile)) {
			return optind;
		}
	} else {
		werr(0, "Global mappingfilename too long, ignored");
	}
	werr(0, "I can't open your mapping file (%s)\n"
		"It is not in '%s" FILE_SEPARATOR ANTIWORD_DIR "' nor in '"
		GLOBAL_ANTIWORD_DIR "'.", szLeafname, szHome);
	return -1;
#endif /* __riscos */
} /* end of iReadOptions */

/*
 * vGetOptions - get a copy of the current option values
 */
void
vGetOptions(options_type *pOptions)
{
	fail(pOptions == NULL);

	*pOptions = tOptionsCurr;
} /* end of vGetOptions */

#if defined(__riscos)
/*
 * vWriteOptions - write the current options to the Options file
 */
static void
vWriteOptions(void)
{
	FILE	*pFile;
	char	*szOptionsFile;

	DBG_MSG("vWriteOptions");

	szOptionsFile = getenv("AntiWord$ChoicesSave");
	if (szOptionsFile == NULL) {
		werr(0, "Warning: Name of the Choices file not found");
		return;
	}
	if (!bMakeDirectory(szOptionsFile)) {
		werr(0,
		"Warning: I can't make a directory for the Choices file");
		return;
	}
	pFile = fopen(szOptionsFile, "w");
	if (pFile == NULL) {
		werr(0, "Warning: I can't write the Choices file");
		return;
	}
	(void)fprintf(pFile, PARAGRAPH_BREAK"\n",
		tOptionsCurr.iParagraphBreak);
	(void)fprintf(pFile, AUTOFILETYPE"\n",
		tOptionsCurr.bAutofiletypeAllowed);
	(void)fprintf(pFile, USE_OUTLINEFONTS"\n",
		tOptionsCurr.eConversionType == conversion_text ? 0 : 1);
	(void)fprintf(pFile, SHOW_IMAGES"\n",
		tOptionsCurr.eImageLevel == level_no_images ? 0 : 1);
	(void)fprintf(pFile, HIDE_HIDDEN_TEXT"\n",
		tOptionsCurr.bHideHiddenText);
	(void)fprintf(pFile, SCALE_FACTOR_START"\n",
		tOptionsCurr.iScaleFactor);
	(void)fclose(pFile);
} /* end of vWriteOptions */

/*
 * vChoicesOpenAction - action to be taken when the Choices window opens
 */
void
vChoicesOpenAction(wimp_w tWindow)
{
	DBG_MSG("vChoicesOpenAction");

	tOptionsTemp = tOptionsCurr;
	if (tOptionsTemp.iParagraphBreak == 0) {
		vUpdateRadioButton(tWindow, CHOICES_BREAK_BUTTON, FALSE);
		vUpdateRadioButton(tWindow, CHOICES_NO_BREAK_BUTTON, TRUE);
		vUpdateWriteableNumber(tWindow, CHOICES_BREAK_WRITEABLE,
					DEFAULT_SCREEN_WIDTH);
	} else {
		vUpdateRadioButton(tWindow, CHOICES_BREAK_BUTTON, TRUE);
		vUpdateRadioButton(tWindow, CHOICES_NO_BREAK_BUTTON, FALSE);
		vUpdateWriteableNumber(tWindow,
			CHOICES_BREAK_WRITEABLE,
			tOptionsTemp.iParagraphBreak);
	}
	vUpdateRadioButton(tWindow, CHOICES_AUTOFILETYPE_BUTTON,
					tOptionsTemp.bAutofiletypeAllowed);
	vUpdateRadioButton(tWindow, CHOICES_HIDDEN_TEXT_BUTTON,
					tOptionsTemp.bHideHiddenText);
	if (tOptionsTemp.eConversionType == conversion_draw) {
		vUpdateRadioButton(tWindow,
			CHOICES_WITH_IMAGES_BUTTON,
			tOptionsTemp.eImageLevel != level_no_images);
		vUpdateRadioButton(tWindow,
			CHOICES_NO_IMAGES_BUTTON,
			tOptionsTemp.eImageLevel == level_no_images);
		vUpdateRadioButton(tWindow,
			CHOICES_TEXTONLY_BUTTON, FALSE);
	} else {
		vUpdateRadioButton(tWindow,
			CHOICES_WITH_IMAGES_BUTTON, FALSE);
		vUpdateRadioButton(tWindow,
			CHOICES_NO_IMAGES_BUTTON, FALSE);
		vUpdateRadioButton(tWindow,
			CHOICES_TEXTONLY_BUTTON, TRUE);
	}
	vUpdateWriteableNumber(tWindow,
		CHOICES_SCALE_WRITEABLE, tOptionsTemp.iScaleFactor);
} /* end of vChoicesOpenAction */

/*
 * vDefaultButtonAction - action when the default button is clicked
 */
static void
vDefaultButtonAction(wimp_w tWindow)
{
	DBG_MSG("vDefaultButtonAction");

	tOptionsTemp = tOptionsDefault;
	vUpdateRadioButton(tWindow, CHOICES_BREAK_BUTTON, TRUE);
	vUpdateRadioButton(tWindow, CHOICES_NO_BREAK_BUTTON, FALSE);
	vUpdateWriteableNumber(tWindow, CHOICES_BREAK_WRITEABLE,
			tOptionsTemp.iParagraphBreak);
	vUpdateRadioButton(tWindow, CHOICES_AUTOFILETYPE_BUTTON,
			tOptionsTemp.bAutofiletypeAllowed);
	vUpdateRadioButton(tWindow, CHOICES_HIDDEN_TEXT_BUTTON,
			tOptionsTemp.bHideHiddenText);
	vUpdateRadioButton(tWindow, CHOICES_WITH_IMAGES_BUTTON,
			tOptionsTemp.eConversionType == conversion_draw &&
			tOptionsTemp.eImageLevel != level_no_images);
	vUpdateRadioButton(tWindow, CHOICES_NO_IMAGES_BUTTON,
			tOptionsTemp.eConversionType == conversion_draw &&
			tOptionsTemp.eImageLevel == level_no_images);
	vUpdateRadioButton(tWindow, CHOICES_TEXTONLY_BUTTON,
			tOptionsTemp.eConversionType == conversion_text);
	vUpdateWriteableNumber(tWindow, CHOICES_SCALE_WRITEABLE,
			tOptionsTemp.iScaleFactor);
} /* end of vDefaultButtonAction */

/*
 * vApplyButtonAction - action to be taken when the OK button is clicked
 */
static void
vApplyButtonAction(void)
{
	DBG_MSG("vApplyButtonAction");

	tOptionsCurr = tOptionsTemp;
} /* end of vApplyButtonAction */

/*
 * vSaveButtonAction - action to be taken when the save button is clicked
 */
static void
vSaveButtonAction(void)
{
	DBG_MSG("vSaveButtonAction");

	vApplyButtonAction();
	vWriteOptions();
} /* end of vSaveButtonAction */

/*
 * vSetParagraphBreak - set the paragraph break to the given number
 */
static void
vSetParagraphBreak(wimp_w tWindow, int iNumber)
{
	tOptionsTemp.iParagraphBreak = iNumber;
	if (tOptionsTemp.iParagraphBreak == 0) {
		return;
	}
	vUpdateWriteableNumber(tWindow,
			CHOICES_BREAK_WRITEABLE,
			tOptionsTemp.iParagraphBreak);
} /* end of vSetParagraphBreak */

/*
 * vChangeParagraphBreak - change the paragraph break with the given number
 */
static void
vChangeParagraphBreak(wimp_w tWindow, int iNumber)
{
	int	iTmp;

	iTmp = tOptionsTemp.iParagraphBreak + iNumber;
	if (iTmp < MIN_SCREEN_WIDTH || iTmp > MAX_SCREEN_WIDTH) {
	  	/* Ignore */
		return;
	}
	tOptionsTemp.iParagraphBreak = iTmp;
	vUpdateWriteableNumber(tWindow,
			CHOICES_BREAK_WRITEABLE,
			tOptionsTemp.iParagraphBreak);
} /* end of vChangeParagraphBreak */

/*
 * vChangeAutofiletype - invert the permission to autofiletype
 */
static void
vChangeAutofiletype(wimp_w tWindow)
{
	tOptionsTemp.bAutofiletypeAllowed =
				!tOptionsTemp.bAutofiletypeAllowed;
	vUpdateRadioButton(tWindow,
			CHOICES_AUTOFILETYPE_BUTTON,
			tOptionsTemp.bAutofiletypeAllowed);
} /* end of vChangeAutofiletype */

/*
 * vChangeHiddenText - invert the hide/show hidden text
 */
static void
vChangeHiddenText(wimp_w tWindow)
{
	tOptionsTemp.bHideHiddenText = !tOptionsTemp.bHideHiddenText;
	vUpdateRadioButton(tWindow,
			CHOICES_HIDDEN_TEXT_BUTTON,
			tOptionsTemp.bHideHiddenText);
} /* end of vChangeHiddenText */

/*
 * vUseFontsImages - use outline fonts, show images
 */
static void
vUseFontsImages(BOOL bUseOutlineFonts, BOOL bShowImages)
{
	tOptionsTemp.eConversionType =
		bUseOutlineFonts ? conversion_draw : conversion_text;
	tOptionsTemp.eImageLevel =
		bUseOutlineFonts && bShowImages ?
		level_default : level_no_images;
} /* end of vUseFontsImages */

/*
 * vSetScaleFactor - set the scale factor to the given number
 */
static void
vSetScaleFactor(wimp_w tWindow, int iNumber)
{
  	tOptionsTemp.iScaleFactor = iNumber;
	vUpdateWriteableNumber(tWindow,
			CHOICES_SCALE_WRITEABLE,
			tOptionsTemp.iScaleFactor);
} /* end of vSetScaleFactor */

/*
 * vChangeScaleFactor - change the scale factor with the given number
 */
static void
vChangeScaleFactor(wimp_w tWindow, int iNumber)
{
	int	iTmp;

	iTmp = tOptionsTemp.iScaleFactor + iNumber;
	if (iTmp < MIN_SCALE_FACTOR || iTmp > MAX_SCALE_FACTOR) {
	  	/* Ignore */
		return;
	}
	tOptionsTemp.iScaleFactor = iTmp;
	vUpdateWriteableNumber(tWindow,
			CHOICES_SCALE_WRITEABLE,
			tOptionsTemp.iScaleFactor);
} /* end of vChangeScaleFactor */

/*
 * bChoicesMouseClick - handle a mouse click in the Choices window
 */
void
vChoicesMouseClick(wimp_mousestr *m)
{
	wimp_i	tAction;
	BOOL	bCloseWindow, bLeft, bRight;

	bLeft = (m->bbits & wimp_BLEFT) == wimp_BLEFT;
	bRight = (m->bbits & wimp_BRIGHT) == wimp_BRIGHT;
	if (!bLeft && !bRight) {
		DBG_HEX(m->bbits);
		return;
	}

	/* Which action should be taken */
	tAction = m->i;
	if (bRight) {
	  	/* The right button reverses the direction */
		switch (m->i) {
		case CHOICES_BREAK_UP_BUTTON:
			tAction = CHOICES_BREAK_DOWN_BUTTON;
			break;
		case CHOICES_BREAK_DOWN_BUTTON:
			tAction = CHOICES_BREAK_UP_BUTTON;
			break;
		case CHOICES_SCALE_UP_BUTTON:
			tAction = CHOICES_SCALE_DOWN_BUTTON;
			break;
		case CHOICES_SCALE_DOWN_BUTTON:
			tAction = CHOICES_SCALE_UP_BUTTON;
			break;
		default:
			break;
		}
	}

	/* Actions */
	bCloseWindow = FALSE;
	switch (tAction) {
	case CHOICES_DEFAULT_BUTTON:
		vDefaultButtonAction(m->w);
		break;
	case CHOICES_SAVE_BUTTON:
		vSaveButtonAction();
		break;
	case CHOICES_CANCEL_BUTTON:
		bCloseWindow = TRUE;
		break;
	case CHOICES_APPLY_BUTTON:
		vApplyButtonAction();
		bCloseWindow = TRUE;
		break;
	case CHOICES_BREAK_BUTTON:
		vSetParagraphBreak(m->w, DEFAULT_SCREEN_WIDTH);
		break;
	case CHOICES_BREAK_UP_BUTTON:
		vChangeParagraphBreak(m->w, 1);
		break;
	case CHOICES_BREAK_DOWN_BUTTON:
		vChangeParagraphBreak(m->w, -1);
		break;
	case CHOICES_NO_BREAK_BUTTON:
		vSetParagraphBreak(m->w, 0);
		break;
	case CHOICES_AUTOFILETYPE_BUTTON:
		vChangeAutofiletype(m->w);
		break;
	case CHOICES_HIDDEN_TEXT_BUTTON:
		vChangeHiddenText(m->w);
		break;
	case CHOICES_WITH_IMAGES_BUTTON:
		vUseFontsImages(TRUE, TRUE);
		break;
	case CHOICES_NO_IMAGES_BUTTON:
		vUseFontsImages(TRUE, FALSE);
		break;
	case CHOICES_TEXTONLY_BUTTON:
		vUseFontsImages(FALSE, FALSE);
		break;
	case CHOICES_SCALE_UP_BUTTON:
		vChangeScaleFactor(m->w, 5);
		break;
	case CHOICES_SCALE_DOWN_BUTTON:
		vChangeScaleFactor(m->w, -5);
		break;
	default:
		DBG_DEC(m->i);
		break;
	}
	if (bCloseWindow) {
		wimpt_noerr(wimp_close_wind(m->w));
	}
} /* end of vChoicesMouseClick */

void
vChoicesKeyPressed(wimp_caretstr *c)
{
	wimp_icon	tIcon;
	char		*pcChar;
	int		iNumber;

	DBG_MSG("vChoicesKeyPressed");

	fail(c == NULL);

	wimpt_noerr(wimp_get_icon_info(c->w, c->i, &tIcon));
	if ((tIcon.flags & (wimp_ITEXT|wimp_INDIRECT)) !=
	    (wimp_ITEXT|wimp_INDIRECT)) {
		werr(1, "Icon %d must be indirected text", (int)c->i);
		return;
	}
	iNumber = (int)strtol(tIcon.data.indirecttext.buffer, &pcChar, 10);

	switch(c->i) {
	case CHOICES_BREAK_WRITEABLE:
		if (*pcChar != '\0' && *pcChar != '\r') {
			DBG_DEC(*pcChar);
			iNumber = DEFAULT_SCREEN_WIDTH;
		} else if (iNumber < MIN_SCREEN_WIDTH) {
			iNumber = MIN_SCREEN_WIDTH;
		} else if (iNumber > MAX_SCREEN_WIDTH) {
			iNumber = MAX_SCREEN_WIDTH;
		}
		vSetParagraphBreak(c->w, iNumber);
		break;
	case CHOICES_SCALE_WRITEABLE:
		if (*pcChar != '\0' && *pcChar != '\r') {
			DBG_DEC(*pcChar);
			iNumber = DEFAULT_SCALE_FACTOR;
		} else if (iNumber < MIN_SCALE_FACTOR) {
			iNumber = MIN_SCALE_FACTOR;
		} else if (iNumber > MAX_SCALE_FACTOR) {
			iNumber = MAX_SCALE_FACTOR;
		}
		vSetScaleFactor(c->w, iNumber);
		break;
	default:
		DBG_DEC(c->i);
		break;
	}
} /* end of vChoicesKeyPressed */
#endif /* __riscos */
