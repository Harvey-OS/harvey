/* Copyright (C) 1994-2001 artofcode LLC.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gdevmacxf.c,v 1.6 2002/06/09 23:08:22 lpd Exp $ */
/* External font (xfont) implementation for Classic/Carbon MacOS. */

#include "gdevmac.h"
#include "gdevmacttf.h"


/* if set to 1, new carbon supported FontManager calls are used */
/* if set to 0, old FM calls that are "not recommended" for carbon are used */
/* for now, we'll set it to 0, as classic and carbon targets don't generate link errors, */
/* but the carbon target would be better built with this macro set to 1 */
/* In the case that it is set, the classic target should link in FontManager(Lib) */
#define USE_RECOMMENDED_CARBON_FONTMANAGER_CALLS 1



extern const byte gs_map_std_to_iso[256];
extern const byte gs_map_iso_to_std[256];


const byte gs_map_std_to_mac[256] =
{
/*			  x0	x1	  x2	x3	  x4	x5	  x6	x7	  x8	x9	  xA	xB	  xC	xD	  xE	xF	*/
/* 0x */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 1x */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 2x */	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
/* 3x */	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
/* 4x */	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
/* 5x */	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
/* 6x */	0xD4, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
/* 7x */	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
/* 8x */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 9x */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* Ax */	0x00, 0xC1, 0xA2, 0xA3, 0xDA, 0xB4, 0xC4, 0xA4, 0xDB, 0x27, 0xD2, 0xC7, 0xDC, 0xDD, 0xDE, 0xDF,
/* Bx */	0x00, 0xD0, 0xA0, 0xE0, 0xE1, 0x00, 0xA6, 0xA5, 0xE2, 0xE3, 0xD3, 0xC8, 0xC9, 0xE4, 0x00, 0xC0,
/* Cx */	0x00, 0x60, 0xAB, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xAC, 0x00, 0xFB, 0xFC, 0x00, 0xFD, 0xFE, 0xFF,
/* Dx */	0xD1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* Ex */	0x00, 0xAE, 0x00, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAF, 0xCE, 0xBC, 0x00, 0x00, 0x00, 0x00,
/* Fx */	0x00, 0xBE, 0x00, 0x00, 0x00, 0xF5, 0x00, 0x00, 0x00, 0xBF, 0xCF, 0xA7, 0x00, 0x00, 0x00, 0x00
};

const byte gs_map_mac_to_std[256] =
{
};

const byte gs_map_iso_to_mac[256] =
{
/*			  x0	x1	  x2	x3	  x4	x5	  x6	x7	  x8	x9	  xA	xB	  xC	xD	  xE	xF	*/
/* 0x */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 1x */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 2x */	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
/* 3x */	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
/* 4x */	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
/* 5x */	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
/* 6x */	0xD4, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
/* 7x */	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
/* 8x */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 9x */	0xF5, 0x60, 0xAB, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xAC, 0x00, 0xFB, 0xFC, 0x00, 0xFD, 0xFE, 0xFF,
/* Ax */	0x00, 0xC1, 0xA2, 0xA3, 0xDB, 0xB4, 0x00, 0xA4, 0xAC, 0xA9, 0xBB, 0xC7, 0xC2, 0x2D, 0xA8, 0xF8,
/* Bx */	0xA1, 0xB1, 0x00, 0x00, 0xAB, 0xB5, 0xA6, 0xE1, 0xFC, 0x00, 0xBC, 0xC8, 0x00, 0x00, 0x00, 0xC0,
/* Cx */	0xCB, 0xE7, 0xE5, 0xCC, 0x80, 0x81, 0xAE, 0x82, 0xE9, 0x83, 0xE6, 0xE8, 0xEA, 0xED, 0xEB, 0xEC,
/* Dx */	0x00, 0x84, 0xF1, 0xEE, 0xEF, 0xCD, 0x85, 0x00, 0xAF, 0xF4, 0xF2, 0xF3, 0x86, 0x00, 0x00, 0xA7,
/* Ex */	0x88, 0x87, 0x89, 0x8B, 0x8A, 0x8C, 0xBE, 0x8D, 0x8F, 0x8E, 0x90, 0x91, 0x93, 0x92, 0x94, 0x95,
/* Fx */	0x00, 0x96, 0x98, 0x97, 0x99, 0x9B, 0x9A, 0xD6, 0xBF, 0x9D, 0x9C, 0x9E, 0x9F, 0x00, 0x00, 0xD8
};

const byte gs_map_mac_to_iso[256] =
{
};





/* The xfont procedure record. */

private const gx_xfont_procs mac_xfont_procs =
{
    mac_lookup_font,
    mac_char_xglyph,
    mac_char_metrics,
    mac_render_char,
    mac_release
};


gs_private_st_dev_ptrs1(st_mac_xfont, mac_xfont, "mac_xfont", mac_xfont_enum_ptrs,
						mac_xfont_reloc_ptrs, dev);





/* Return the xfont procedure record. */

const gx_xfont_procs *
mac_get_xfont_procs(gx_device *dev)
{
#pragma unused(dev)
    return &mac_xfont_procs;
}



/* lookup_font */

private gx_xfont *
mac_lookup_font(gx_device *dev, const byte *fname, uint len,
				int encoding_index, const gs_uid *puid,
				const gs_matrix *pmat, gs_memory_t *mem)
{
#pragma unused(encoding_index,puid)
	mac_xfont		*macxf;
	
	CGrafPort		*currentPort;
	int				txFont, txSize, txMode;
	StyleField		txFace;
	Fixed			spExtra;
	
	/* are XFonts enabled? */
	if (((gx_device_macos*) dev)->useXFonts == false)
		return NULL;
	
	/* we can handle only requests from these encodings */
	if (encoding_index != ENCODING_INDEX_MACROMAN && encoding_index != ENCODING_INDEX_ISOLATIN1 &&
			encoding_index != ENCODING_INDEX_STANDARD)
		return NULL;
	
	/* Don't render very small fonts */
	if (fabs(pmat->xx * 1000.0) < 3.0)
		return NULL;
	
	/* Only handle simple cases for now (no transformations). */
	if (fabs(pmat->xy) > 0.0001 || fabs(pmat->yx) > 0.0001 || pmat->xx <= 0)
		return NULL;
	
	/* allocate memory for gx_xfont */
	macxf = gs_alloc_struct(mem, mac_xfont, &st_mac_xfont, "mac_lookup_font");
	if (macxf == NULL) {
		return NULL;
	}
	
	/* set default values */
	macxf->common.procs = &mac_xfont_procs;
	macxf->dev = dev;
	
	/* find the specified font */
	mac_find_font_family(fname, len, &(macxf->fontID), &(macxf->fontFace));
	
	/* no font found */
	if (macxf->fontID == 0)
		return NULL;

	FMGetFontFamilyName(macxf->fontID, macxf->fontName);
	macxf->fontSize = (short)(pmat->xx * 1000.0);
	macxf->fontEncoding = mac_get_font_encoding(macxf);
	
	/* we can handle only fonts with these encodings for now (all original Mac fonts have MacRoman encoding!) */
	if (macxf->fontEncoding != ENCODING_INDEX_MACROMAN && macxf->fontEncoding != ENCODING_INDEX_ISOLATIN1)
		return NULL;
	
	/* get font metrics */
	
	/* save current GrafPort's font information */
	GetPort(&((GrafPort*) currentPort));
	txFont  = currentPort->txFont;
	txSize  = currentPort->txSize;
	txFace  = currentPort->txFace;
	txMode  = currentPort->txMode;
	spExtra = currentPort->spExtra;
	
	/* set values for measuring */
	TextFont(macxf->fontID);
	TextSize(macxf->fontSize);
	TextFace(macxf->fontFace);
	TextMode(srcOr);
	SpaceExtra(0);
	
	/* measure font */
	FontMetrics(&(macxf->fontMetrics));
	
	/* restore current GrafPort's font information */
	currentPort->txFont  = txFont;
	currentPort->txSize  = txSize;
	currentPort->txFace  = txFace;
	currentPort->txMode  = txMode;
	currentPort->spExtra = spExtra;
	
	return (gx_xfont*) macxf;
}



/* char_xglyph */

private gx_xglyph
mac_char_xglyph(gx_xfont *xf, gs_char chr, int encoding_index,
		gs_glyph glyph, const gs_const_string *glyph_name)
{
#pragma unused(glyph_name,glyph)
	mac_xfont			* macxf = (mac_xfont*) xf;
	
	/* can't look up names yet */
	if (chr == gs_no_char)
		return gx_no_xglyph;
	
	if (macxf->fontEncoding == ENCODING_INDEX_MACROMAN) {
		switch (encoding_index) {
			case ENCODING_INDEX_MACROMAN:	return chr;
			case ENCODING_INDEX_STANDARD:	return gs_map_std_to_mac[chr];
			case ENCODING_INDEX_ISOLATIN1:	return gs_map_iso_to_mac[chr];
		}
	} else if (macxf->fontEncoding == ENCODING_INDEX_ISOLATIN1) {
		switch (encoding_index) {
			case ENCODING_INDEX_MACROMAN:	return gs_map_mac_to_iso[chr];
			case ENCODING_INDEX_STANDARD:	return gs_map_std_to_iso[chr];
			case ENCODING_INDEX_ISOLATIN1:	return chr;
		}
	}
	
	return gx_no_xglyph;
}



/* char_metrics */

private int
mac_char_metrics(gx_xfont *xf, gx_xglyph xg, int wmode,
				 gs_point *pwidth, gs_int_rect *pbbox)
{
#pragma unused(xg)
	mac_xfont			* macxf = (mac_xfont*) xf;
	
	if (wmode != 0)
		return gs_error_undefined;
	
	pbbox->p.x = 0;
	pbbox->q.x = Fix2Long(macxf->fontMetrics.widMax);
	pbbox->p.y = -Fix2Long(macxf->fontMetrics.ascent);
	pbbox->q.y = Fix2Long(macxf->fontMetrics.descent);
	pwidth->x = pbbox->q.x;
	pwidth->y = 0.0;
	
	return 0;
}



/* render_char */

private int
mac_render_char(gx_xfont *xf, gx_xglyph xg, gx_device *dev,
				int xo, int yo, gx_color_index color, int required)
{
#pragma unused(dev,required)
	mac_xfont			* macxf = (mac_xfont*) xf;
	gx_device_macos		* mdev = (gx_device_macos*) macxf->dev;
	
	Str255				character;
	int					i, found;
	
	CheckMem(10*1024, 100*1024);
	ResetPage();
	
	character[0] = 1;
	character[1] = xg;
	
	GSSetFgCol(macxf->dev, mdev->currPicPos, color);
	
	found = 0;
	for (i=0; i<mdev->numUsedFonts; i++)
		if (mdev->usedFontIDs[i] == macxf->fontID)	found = 1;
	
	if (!found) {
		mdev->usedFontIDs[mdev->numUsedFonts++] = macxf->fontID;
		PICT_fontName(mdev->currPicPos, macxf->fontID, macxf->fontName);
	}
	if (mdev->lastFontID != macxf->fontID) {
		PICT_TxFont(mdev->currPicPos, macxf->fontID);
		mdev->lastFontID = macxf->fontID;
	}
	if (mdev->lastFontSize != macxf->fontSize) {
		PICT_TxSize(mdev->currPicPos, macxf->fontSize);
		mdev->lastFontSize = macxf->fontSize;
	}
	if (mdev->lastFontFace != macxf->fontFace) {
		PICT_TxFace(mdev->currPicPos, macxf->fontFace);
		mdev->lastFontFace = macxf->fontFace;
	}
	PICT_LongText(mdev->currPicPos, xo, yo, character);
	PICT_OpEndPicGoOn(mdev->currPicPos);
	
	return 0;
}



/* release */

private int
mac_release(gx_xfont *xf, gs_memory_t *mem)
{
	if (mem != NULL)
		gs_free_object(mem, xf, "mac_release");
	
	return 0;
}



/* try to extract font family and style from name and find a suitable font */

private void
mac_find_font_family(ConstStringPtr fname, int len, FMFontFamily *fontID, FMFontStyle *fontFace)
{
	char			fontNameStr[512];
	char			*fontFamilyName;
	char			*fontStyleName;
	int				i;
	
	*fontID   = 0;
	*fontFace = 0;
	
	/* first try the full fontname */
	fontNameStr[0] = len;
	memcpy(fontNameStr+1, fname, len);
	*fontID = FMGetFontFamilyFromName((StringPtr) fontNameStr);
	if (*fontID > 0)	return;
	
	/* try to find the font without the dashes */
	fontNameStr[0] = len;
	memcpy(fontNameStr+1, fname, len);
	for (i=1; i<=len; i++)
		if (fontNameStr[i] == '-')	fontNameStr[i] = ' ';
	*fontID = FMGetFontFamilyFromName((StringPtr) fontNameStr);
	if (*fontID > 0)	return;
	
	/* we should read some default fontname mappings from a file here */
	if (*fontID > 0)	return;
	
	/* try to extract font basename and style names */
	memcpy(fontNameStr, fname, len);
	fontNameStr[len] = 0;
	
	fontFamilyName = strtok(fontNameStr, "- ");
	while ((fontStyleName = strtok(NULL, "- ")) != NULL) {
		if (!strcmp(fontStyleName, "Italic") || !strcmp(fontStyleName, "Oblique") || !strcmp(fontStyleName, "It"))
			*fontFace |= italic;
		if (!strcmp(fontStyleName, "Bold") || !strcmp(fontStyleName, "Bd"))
			*fontFace |= bold;
		if (!strcmp(fontStyleName, "Narrow") || !strcmp(fontStyleName, "Condensed"))
			*fontFace |= condense;
	}
	
	if (fontFamilyName == NULL) {
		return;
	} else {
		Str255	fontName;
		
		fontName[0] = strlen(fontFamilyName);
		strcpy((char*)(fontName+1), fontFamilyName);
		*fontID = FMGetFontFamilyFromName((StringPtr) fontNameStr);
		if (*fontID > 0) return;
	}
}



/* extract a font's platform id (encoding) */

private int
mac_get_font_encoding(mac_xfont *macxf)
{
	int			encoding = ENCODING_INDEX_UNKNOWN;
	ResType		resType;
	short		resID;
	
	mac_get_font_resource(macxf, &resType, &resID);
	
	if (resType == 'sfnt') {
		Handle				fontHandle;
		TTFontDir			*fontDir;
		TTFontNamingTable	*fontNamingTable;
		int					i;
		
		/* load resource */
		if ((fontHandle = GetResource(resType, resID)) == NULL)
			return encoding;
		HLock(fontHandle);
		
		/* walk through the font directory and find the font naming table */
		fontDir = (TTFontDir*) *fontHandle;
		if (fontDir != NULL && fontDir->version == 'true') {
			for (i=0; i<fontDir->numTables; i++) {
				if (fontDir->components[i].tagName == TTF_FONT_NAMING_TABLE) {
					fontNamingTable = (TTFontNamingTable*) ((long)(fontDir->components[i].offset) + (long)fontDir);
					switch (fontNamingTable->platformID) {
						//case 0:		encoding = ENCODING_INDEX_STANDARD;		break;	/* Unicode */
						case 1:		encoding = ENCODING_INDEX_MACROMAN;		break;
						case 2:		encoding = ENCODING_INDEX_ISOLATIN1;	break;
						//case 3:		encoding = ENCODING_INDEX_WINANSI;		break;
					}
					break;
				}
			}
		}
		
		HUnlock(fontHandle);
		ReleaseResource(fontHandle);
	}
	
	return encoding;
}



/* get a handle to a font resource */

private void
mac_get_font_resource(mac_xfont *macxf, ResType *resType, short *resID)
{
	FMInput		fontInput = {0, 0, 0, true, 0, {1,1}, {1,1}};
	FMOutputPtr	fontOutput;
	
	Str255		resName;
	
	fontInput.family	= macxf->fontID;
	fontInput.size		= macxf->fontSize;
	fontInput.face		= macxf->fontFace;

	fontOutput = FMSwapFont(&fontInput);
	
	if (fontOutput == NULL || fontOutput->fontHandle == NULL)
		return;
	
	GetResInfo(fontOutput->fontHandle, resID, resType, resName);
}



#if !USE_RECOMMENDED_CARBON_FONTMANAGER_CALLS
/* wrap the old Classic MacOS font manager calls to fake support for the
   new FontManager API on older systems */

OSStatus
FMGetFontFamilyName(FMFontFamily fontFamilyID, Str255 fontNameStr)
{
	GetFontName(fontFamilyID, fontNameStr);
	return noErr;
}

FMFontFamily
FMGetFontFamilyFromName(ConstStr255Param fontNameStr)
{
    int	fontID;
    GetFNum(fontNameStr, &fontID);

    return (FMFontFamily)fontID;
}
#endif
