/* Copyright (C) 2003 artofcode LLC.  All rights reserved.
  
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
/* $Id: ttfsfnt.h,v 1.6 2003/12/09 21:17:59 giles Exp $ */
/* Changes after Apple: replaced non-portable types with ISO/IEC 988:1999 exact-size types */

/*
	File 'sfnt.h'
	
	Contains 'sfnt' resource structure description
	
	Copyright 1991 Apple Computer, Inc.
	
*/

#ifndef sfntIncludes
#define sfntIncludes

#include "stdint_.h" /* make sure stdint types are available */
                            
typedef   uint8_t    uint8; /* 8-bit unsigned integer */
typedef    int8_t     int8; /* 8-bit signed integer */
typedef  uint16_t   uint16; /* 16-bit unsigned integer */
typedef   int16_t    int16; /* 16-bit signed integer */
typedef  uint32_t   uint32; /* 32-bit unsigned integer */
typedef   int32_t    int32; /* 32-bit signed integer */
#if 0
typedef   int32_t    Fixed; /* 16.16 32-bit signed fixed-point number */
#endif
typedef   int16_t    FUnit; /* Smallest measurable distance in em space (16-bit signed integer) */
typedef   int16_t    FWord; /* 16-bit signed integer that describes a quantity in FUnits */
typedef  uint16_t   uFWord; /* 16-bit unsigned integer that describes a quantity in FUnits */
typedef   int16_t  F2Dot14; /* 2.14 16-bit signed fixed-point number */
#if 0
typedef   int32_t  F26Dot6; /* 26.6 32-bit signed fixed-point number */
#endif

typedef struct {
	uint32 bc;
	uint32 ad;
} BigDate;

typedef struct {
	uint32 tag;
	uint32 checkSum;
	uint32 offset;
	uint32 length;
} sfnt_DirectoryEntry;

#define SFNT_VERSION	0x10000

/*
 *	The search fields limits numOffsets to 4096.
 */
typedef struct {
	Fixed version;					/* 1.0 */
	uint16 numOffsets;		/* number of tables */
	uint16 searchRange;		/* (max2 <= numOffsets)*16 */
	uint16 entrySelector;		/* log2(max2 <= numOffsets) */
	uint16 rangeShift;			/* numOffsets*16-searchRange*/
	sfnt_DirectoryEntry table[1];		/* table[numOffsets] */
} sfnt_OffsetTable;

#define OFFSETTABLESIZE		12		/* not including any entries */

typedef enum sfntHeaderFlagBits {
	Y_POS_SPECS_BASELINE = 1,
	X_POS_SPECS_LSB = 2,
	HINTS_USE_POINTSIZE = 4,
	USE_INTEGER_SCALING = 8,
	INSTRUCTIONS_CHANGE_ADVANCEWIDTHS = 16,
	X_POS_SPECS_BASELINE = 32,
	Y_POS_SPECS_TSB = 64
} sfntHeaderFlagBits;

#define SFNT_MAGIC					0x5F0F3CF5
#define SHORT_INDEX_TO_LOC_FORMAT		0
#define LONG_INDEX_TO_LOC_FORMAT		1
#define GLYPH_DATA_FORMAT				0
#define FONT_HEADER_VERSION			0x10000

typedef struct {
	Fixed	version;		/* for this table, set to 1.0 */
	Fixed	fontRevision;		/* For Font Manufacturer */
	uint32	checkSumAdjustment;
	uint32	magicNumber; 		/* signature, should always be 0x5F0F3CF5  == MAGIC */
	uint16	flags;
	uint16	unitsPerEm;		/* Specifies how many in Font Units we have per EM */

	BigDate		created;
	BigDate		modified;

	/** This is the font wide bounding box in ideal space
	(baselines and metrics are NOT worked into these numbers) **/
	int16		xMin;
	int16		yMin;
	int16		xMax;
	int16		yMax;

	uint16	macStyle;	/* macintosh style word */
	uint16	lowestRecPPEM; 	/* lowest recommended pixels per Em */

	/* 0: fully mixed directional glyphs, 1: only strongly L->R or T->B glyphs, 
	   -1: only strongly R->L or B->T glyphs, 2: like 1 but also contains neutrals,
	   -2: like -1 but also contains neutrals */
	int16		fontDirectionHint;

	int16		indexToLocFormat;
	int16		glyphDataFormat;
} sfnt_FontHeader;

#define METRIC_HEADER_FORMAT		0x10000

typedef struct {
	Fixed	version;				/* for this table, set to 1.0 */
	int16		ascender;
	int16		descender;
	int16		lineGap;				/* linespacing = ascender - descender + linegap */
	uint16	advanceMax;	
	int16		sideBearingMin;		/* left or top */
	int16		otherSideBearingMin;	/* right or bottom */
	int16		extentMax; 			/* Max of ( SB[i] + bounds[i] ), i loops through all glyphs */
	int16		caretSlopeNumerator;
	int16		caretSlopeDenominator;
	int16		caretOffset;

	uint32	reserved1, reserved2;	/* set to 0 */

	int16		metricDataFormat;		/* set to 0 for current format */
	uint16	numberLongMetrics;		/* if format == 0 */
} sfnt_MetricsHeader;

typedef sfnt_MetricsHeader sfnt_HorizontalHeader;
typedef sfnt_MetricsHeader sfnt_VerticalHeader;

#define MAX_PROFILE_VERSION		0x10000

typedef struct {
	Fixed			version;				/* for this table, set to 1.0 */
	uint16		numGlyphs;
	uint16		maxPoints;			/* in an individual glyph */
	uint16		maxContours;			/* in an individual glyph */
	uint16		maxCompositePoints;	/* in an composite glyph */
	uint16		maxCompositeContours;	/* in an composite glyph */
	uint16		maxElements;			/* set to 2, or 1 if no twilightzone points */
	uint16		maxTwilightPoints;		/* max points in element zero */
	uint16		maxStorage;			/* max number of storage locations */
	uint16		maxFunctionDefs;		/* max number of FDEFs in any preprogram */
	uint16		maxInstructionDefs;		/* max number of IDEFs in any preprogram */
	uint16		maxStackElements;		/* max number of stack elements for any individual glyph */
	uint16		maxSizeOfInstructions;	/* max size in bytes for any individual glyph */
	uint16		maxComponentElements;	/* number of glyphs referenced at top level */
	uint16		maxComponentDepth;	/* levels of recursion, 1 for simple components */
} sfnt_maxProfileTable;


typedef struct {
	uint16	advance;
	int16 	sideBearing;
} sfnt_GlyphMetrics;

typedef sfnt_GlyphMetrics sfnt_HorizontalMetrics;
typedef sfnt_GlyphMetrics sfnt_VerticalMetrics;

typedef int16 sfnt_ControlValue;

/*
 *	Char2Index structures, including platform IDs
 */
typedef struct {
	uint16	format;
	uint16	length;
	uint16	version;
} sfnt_mappingTable;

typedef struct {
	uint16	platformID;
	uint16	specificID;
	uint32	offset;
} sfnt_platformEntry;

typedef struct {
	uint16	version;
	uint16	numTables;
	sfnt_platformEntry platform[1];	/* platform[numTables] */
} sfnt_char2IndexDirectory;
#define SIZEOFCHAR2INDEXDIR		4

typedef struct {
	uint16 platformID;
	uint16 specificID;
	uint16 languageID;
	uint16 nameID;
	uint16 length;
	uint16 offset;
} sfnt_NameRecord;

typedef struct {
	uint16 format;
	uint16 count;
	uint16 stringOffset;
/*	sfnt_NameRecord[count]	*/
} sfnt_NamingTable;

#define DEVWIDTHEXTRA	2	/* size + max */
/*
 *	Each record is n+2 bytes, padded to long word alignment.
 *	First byte is ppem, second is maxWidth, rest are widths for each glyph
 */
typedef struct {
	int16				version;
	int16				numRecords;
	int32				recordSize;
	/* Byte widths[numGlyphs+DEVWIDTHEXTRA] * numRecords */
} sfnt_DeviceMetrics;

#define	stdPostTableFormat		0x10000
#define	wordPostTableFormat	0x20000
#define	bytePostTableFormat	0x28000
#define	richardsPostTableFormat	0x30000

typedef struct {
	Fixed	version;
	Fixed	italicAngle;
	int16	underlinePosition;
	int16	underlineThickness;
	int16	isFixedPitch;
	int16	pad;
	uint32	minMemType42;
	uint32	maxMemType42;
	uint32	minMemType1;
	uint32	maxMemType1;
/* if version == 2.0
	{
		numberGlyphs;
		unsigned short[numberGlyphs];
		pascalString[numberNewNames];
	}
	else if version == 2.5
	{
		numberGlyphs;
		int8[numberGlyphs];
	}
*/		
} sfnt_PostScriptInfo;

typedef enum outlinePacking {
	ONCURVE = 1,
	XSHORT = 2,
	YSHORT = 4,
	REPEAT_FLAGS = 8,
/* IF XSHORT */
	SHORT_X_IS_POS = 16,		/* the short vector is positive */
/* ELSE */
	NEXT_X_IS_ZERO = 16,		/* the relative x coordinate is zero */
/* ENDIF */
/* IF YSHORT */
	SHORT_Y_IS_POS = 32,		/* the short vector is positive */
/* ELSE */
	NEXT_Y_IS_ZERO = 32		/* the relative y coordinate is zero */
/* ENDIF */
} outlinePacking;

typedef enum componentPacking {
	COMPONENTCTRCOUNT = -1,
	ARG_1_AND_2_ARE_WORDS = 1,		/* if not, they are bytes */
	ARGS_ARE_XY_VALUES = 2,			/* if not, they are points */
	ROUND_XY_TO_GRID = 4,
	WE_HAVE_A_SCALE = 8,				/* if not, Sx = Sy = 1.0 */
	NON_OVERLAPPING = 16,
	MORE_COMPONENTS = 32,			/* if not, this is the last one */
	WE_HAVE_AN_X_AND_Y_SCALE = 64,	/* Sx != Sy */
	WE_HAVE_A_TWO_BY_TWO = 128,		/* t00, t01, t10, t11 */
	WE_HAVE_INSTRUCTIONS = 256,		/* short count followed by instructions follows */
	USE_MY_METRICS = 512				/* use my metrics for parent glyph */
} componentPacking;

typedef struct {
	uint16 firstCode;
	uint16 entryCount;
	int16 idDelta;
	uint16 idRangeOffset;
} sfnt_subheader;

typedef struct {
	uint16 segCountX2;
	uint16 searchRange;
	uint16 entrySelector;
	uint16 rangeShift;
} sfnt_4_subheader;

/* sfnt_enum.h */

typedef enum {
	plat_Unicode,
	plat_Macintosh,
	plat_ISO,
	plat_MS
} platformEnums;

#define tag_FontHeader			  'daeh'  /* (*(LPDWORD)"head") */
#define tag_HoriHeader			  'aehh'  /* (*(LPDWORD)"hhea") */
#define tag_VertHeader			  'aehv'  /* (*(LPDWORD)"vhea") */
#define tag_IndexToLoc			  'acol'  /* (*(LPDWORD)"loca") */
#define tag_MaxProfile			  'pxam'  /* (*(LPDWORD)"maxp") */
#define tag_ControlValue		  ' tvc'  /* (*(LPDWORD)"cvt ") */
#define tag_PreProgram			  'perp'  /* (*(LPDWORD)"prep") */
#define tag_GlyphData			    'fylg'  /* (*(LPDWORD)"glyf") */
#define tag_HorizontalMetrics	'xtmh'  /* (*(LPDWORD)"hmtx") */
#define tag_VerticalMetrics	  'xtmv'  /* (*(LPDWORD)"vmtx") */
#define tag_CharToIndexMap	  'pamc'  /* (*(LPDWORD)"cmap") */
#define tag_FontProgram			  'mgpf'  /* (*(LPDWORD)"fpgm") */

#define tag_Kerning				    'nrek'  /* (*(LPDWORD)"kern") */
#define tag_HoriDeviceMetrics	'xmdh'  /* (*(LPDWORD)"hdmx") */
#define tag_NamingTable			  'eman'  /* (*(LPDWORD)"name") */
#define tag_PostScript			  'tsop'  /* (*(LPDWORD)"post") */

#if 0
/* access.h */
#define fNoError			 0
#define fTableNotFound		-1
#define fNameNotFound		-2
#define fMemoryError		-3
#define fUnimplemented		-4
#define fCMapNotFound		-5
#define fGlyphNotFound		-6

typedef int32 FontError;
#endif

typedef struct FontTableInfo {
	int32		offset;			/* from beginning of sfnt to beginning of the table */
	int32		length;			/* length of the table */
	int32		checkSum;		/* checkSum of the table */
} FontTableInfo;

#define RAW_TRUE_TYPE_SIZE 512

#endif
