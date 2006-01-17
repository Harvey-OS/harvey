/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxttf.h,v 1.6 2002/06/28 05:27:22 lpd Exp $ */
/* Table definitions for TrueType fonts */

#ifndef gxttf_INCLUDED
#  define gxttf_INCLUDED

/* ------ Composite glyf component flags ------ */

#define TT_CG_ARGS_ARE_WORDS     (1<<0)
#define TT_CG_ARGS_ARE_XY_VALUES (1<<1)
#define TT_CG_ROUND_XY_TO_GRID   (1<<2)
#define TT_CG_HAVE_SCALE         (1<<3)
#define TT_CG_MORE_COMPONENTS    (1<<5)
#define TT_CG_HAVE_XY_SCALE      (1<<6)
#define TT_CG_HAVE_2X2           (1<<7)
#define TT_CG_HAVE_INSTRUCTIONS  (1<<8)
#define TT_CG_USE_MY_METRICS     (1<<9)

/* ------ head ------ */

typedef struct ttf_head_s {
    byte
	version[4],		/* version 1.0 */
	fontRevision[4],
	checkSumAdjustment[4],
	magicNumber[4],
	flags[2],
	unitsPerEm[2],
	created[8],
	modified[8],
	xMin[2],
	yMin[2],
	xMax[2],
	yMax[2],
	macStyle[2],
	lowestRecPPM[2],
	fontDirectionHint[2],
	indexToLocFormat[2],
	glyphDataFormat[2];
} ttf_head_t;

/* ------ hhea ------ */

typedef struct ttf_hhea_s {
    byte
	version[4],		/* version 1.0 */
	ascender[2],		/* FWord */
	descender[2],		/* FWord */
	lineGap[2],		/* FWord */
	advanceWidthMax[2],	/* uFWord */
	minLeftSideBearing[2],	/* FWord */
	minRightSideBearing[2],	/* FWord */
	xMaxExtent[2],		/* FWord */
	caretSlopeRise[2],
	caretSlopeRun[2],
	caretOffset[2],
	reserved[8],
	metricDataFormat[2],	/* 0 */
	numHMetrics[2];
} ttf_hhea_t;

/* ------ hmtx ------ */

typedef struct longHorMetric_s {
    byte
	advanceWidth[2],	/* uFWord */
	lsb[2];			/* FWord */
} longHorMetric_t;

/* ------ maxp ------ */

typedef struct ttf_maxp_s {
    byte
	version[4],		/* 1.0 */
	numGlyphs[2],
	maxPoints[2],
	maxContours[2],
	maxCompositePoints[2],
	maxCompositeContours[2],
	maxZones[2],
	maxTwilightPoints[2],
	maxStorage[2],
	maxFunctionDefs[2],
	maxInstructionDefs[2],
	maxStackElements[2],
	maxSizeOfInstructions[2],
	maxComponentElements[2],
	maxComponentDepth[2];
} ttf_maxp_t;

/* ------ OS/2 ------ */

typedef struct ttf_OS_2_s {
    byte
	version[2],		/* version 1 */
	xAvgCharWidth[2],
	usWeightClass[2],
	usWidthClass[2],
	fsType[2],
	ySubscriptXSize[2],
	ySubscriptYSize[2],
	ySubscriptXOffset[2],
	ySubscriptYOffset[2],
	ySuperscriptXSize[2],
	ySuperscriptYSize[2],
	ySuperscriptXOffset[2],
	ySuperscriptYOffset[2],
	yStrikeoutSize[2],
	yStrikeoutPosition[2],
	sFamilyClass[2],
	/*panose:*/
	    bFamilyType, bSerifStyle, bWeight, bProportion, bContrast,
	    bStrokeVariation, bArmStyle, bLetterform, bMidline, bXHeight,
	ulUnicodeRanges[16],
	achVendID[4],
	fsSelection[2],
	usFirstCharIndex[2],
	usLastCharIndex[2],
	sTypoAscender[2],
	sTypoDescender[2],
	sTypoLineGap[2],
	usWinAscent[2],
	usWinDescent[2],
	ulCodePageRanges[8];
} ttf_OS_2_t;

/* ------ vhea ------ */

typedef struct ttf_vhea_s {
    byte
	version[4],		/* version 1.0 */
	ascent[2],		/* FWord */
	descent[2],		/* FWord */
	lineGap[2],		/* FWord */
	advanceHeightMax[2],	/* uFWord */
	minTopSideBearing[2],	/* FWord */
	minBottomSideBearing[2],  /* FWord */
	yMaxExtent[2],		/* FWord */
	caretSlopeRise[2],
	caretSlopeRun[2],
	caretOffset[2],
	reserved[8],
	metricDataFormat[2],	/* 0 */
	numVMetrics[2];
} ttf_vhea_t;

/* ------ vmtx ------ */

typedef struct longVerMetric_s {
    byte
	advanceHeight[2],	/* uFWord */
	topSideBearing[2];	/* FWord */
} longVerMetric_t;

#endif /* gxttf_INCLUDED */
