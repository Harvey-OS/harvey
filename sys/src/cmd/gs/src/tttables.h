/* Copyright (C) 2003 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: tttables.h,v 1.2 2003/11/21 20:01:22 giles Exp $ */

/* Changes after FreeType: cut out the TrueType instruction interpreter. */


/*******************************************************************
 *
 *  tttables.h                                                  1.1
 *
 *    TrueType Tables structures and handling (specification).
 *
 *  Copyright 1996-1998 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT. By continuing to use, modify, or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 ******************************************************************/

#ifndef TTTABLES_H
#define TTTABLES_H

#include "tttypes.h"

#ifdef __cplusplus
  extern "C" {
#endif

  /***********************************************************************/
  /*                                                                     */
  /*                      TrueType Table Types                           */
  /*                                                                     */
  /***********************************************************************/

  /* TrueType Collection Header */

  struct  _TTTCHeader
  {
    Long      Tag;
    TT_Fixed  version;
    ULong     DirCount;
    PULong    TableDirectory;
  };

  typedef struct _TTTCHeader  TTTCHeader;
  typedef TTTCHeader*         PTTCHeader;


  /* TrueType Table Directory type */

  struct  _TTableDir
  {
    TT_Fixed  version;      /* should be 0x10000 */
    UShort    numTables;    /* number of tables  */

    UShort  searchRange;    /* These parameters are only used  */
    UShort  entrySelector;  /* for a dichotomy search in the   */
    UShort  rangeShift;     /* directory. We ignore them.      */
  };

  typedef struct _TTableDir  TTableDir;
  typedef TTableDir*         PTableDir;


  /* The 'TableDir' is followed by 'numTables' TableDirEntries */

  struct  _TTableDirEntry
  {
    Long  Tag;        /*        table type */
    Long  CheckSum;   /*    table checksum */
    Long  Offset;     /* table file offset */
    Long  Length;     /*      table length */
  };

  typedef struct _TTableDirEntry  TTableDirEntry;
  typedef TTableDirEntry*         PTableDirEntry;


  /* 'cmap' tables */

  struct  _TCMapDir
  {
    UShort  tableVersionNumber;
    UShort  numCMaps;
  };

  typedef struct _TCMapDir  TCMapDir;
  typedef TCMapDir*         PCMapDir;

  struct  _TCMapDirEntry
  {
    UShort  platformID;
    UShort  platformEncodingID;
    Long    offset;
  };

  typedef struct _TCMapDirEntry  TCMapDirEntry;
  typedef TCMapDirEntry*         PCMapDirEntries;


  /* 'maxp' Maximum Profiles table */

  struct  _TMaxProfile
  {
    TT_Fixed  version;
    UShort    numGlyphs,
              maxPoints,
              maxContours,
              maxCompositePoints,
              maxCompositeContours,
              maxZones,
              maxTwilightPoints,
              maxStorage,
              maxFunctionDefs,
              maxInstructionDefs,
              maxStackElements,
              maxSizeOfInstructions,
              maxComponentElements,
              maxComponentDepth;
  };

  typedef struct _TMaxProfile  TMaxProfile;
  typedef TMaxProfile*         PMaxProfile;


  /* table "gasp" */

#  define GASP_GRIDFIT  0x01
#  define GASP_DOGRAY   0x02

  struct  _GaspRange
  {
    UShort  maxPPEM;
    UShort  gaspFlag;
  };

  typedef struct _GaspRange  GaspRange;


  struct  _TGasp
  {
    UShort      version;
    UShort      numRanges;
    GaspRange*  gaspRanges;
  };

  typedef struct _TGasp  TGasp;


  /* table "head" - now defined in freetype.h */
  /* table "hhea" - now defined in freetype.h */


  /* table "HMTX" */

  struct  _TLongHorMetric
  {
    UShort  advance_Width;
    Short   lsb;
  };

  typedef struct _TLongHorMetric  TLongHorMetric;
  typedef TLongHorMetric*         PTableHorMetrics;


  /* 'OS/2' table - now defined in freetype.h */
  /* "post" table - now defined in freetype.h */


  /* 'loca' location table type */

  struct  _TLoca
  {
    UShort    Size;
    PStorage  Table;
  };

  typedef struct _TLoca  TLoca;


  /* table "name" */

  struct  _TNameRec
  {
    UShort  platformID;
    UShort  encodingID;
    UShort  languageID;
    UShort  nameID;
    UShort  stringLength;
    UShort  stringOffset;

    /* this last field is not defined in the spec */
    /* but used by the FreeType engine            */

    PByte   string;
  };

  typedef struct _TNameRec  TNameRec;


  struct  _TName_Table
  {
    UShort     format;
    UShort     numNameRecords;
    UShort     storageOffset;
    TNameRec*  names;
    PByte      storage;
  };

  typedef struct _TName_Table  TName_Table;


#ifdef __cplusplus
  }
#endif

#endif /* TTTABLES_H */


/* END */
