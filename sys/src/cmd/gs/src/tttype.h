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

/* $Id: tttype.h,v 1.3 2005/05/31 13:05:20 igor Exp $ */

/* Changes after FreeType: cut out the TrueType instruction interpreter. */


/*******************************************************************
 *
 *  tttype.h
 *
 *    High-level interface specification.
 *
 *  Copyright 1996-1998 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project and may only be used,
 *  modified, and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT.  By continuing to use, modify, or distribute 
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 *  Notes:
 *
 *    This is the only file that should be included by client            
 *    application sources for the final release.  All other types
 *    and functions defined in the "tt*.h" files are library  
 *    internals, and should not be included (except of course
 *    during development, as now).
 *
 *    FreeType is still in beta!
 *
 ******************************************************************/

#ifndef FREETYPE_H
#define FREETYPE_H

#ifdef __cplusplus
  extern "C" {
#endif


  /*******************************************************************/
  /*                                                                 */
  /*  FreeType types definitions.                                    */
  /*                                                                 */
  /*  All these begin with a 'TT_' prefix.                           */
  /*                                                                 */
  /*******************************************************************/

#if   ARCH_LOG2_SIZEOF_LONG == 2
  typedef signed long     TT_Fixed;   /* Signed Fixed 16.16 Float */
#elif ARCH_LOG2_SIZEOF_INT  == 2
  typedef signed int      TT_Fixed;   /* Signed Fixed 16.16 Float */
#else
#error "No appropriate type for Fixed 16.16 Floats"
#endif

  typedef signed short    TT_FWord;   /* Distance in FUnits */
  typedef unsigned short  TT_UFWord;  /* Unsigned distance */

  typedef signed short    TT_Short;
  typedef unsigned short  TT_UShort;
  typedef signed long     TT_Long;
  typedef unsigned long   TT_ULong;

  typedef signed short    TT_F2Dot14; /* Signed fixed float 2.14 used for */
                                      /* unary vectors, with layout:      */
                                      /*                                  */
                                      /*  s : 1  -- sign bit              */
                                      /*  m : 1  -- mantissa bit          */
                                      /*  f : 14 -- unsigned fractional   */
                                      /*                                  */
                                      /*  's:m' is the 2-bit signed int   */
                                      /*  value to which the positive     */
                                      /*  fractional part should be       */
                                      /*  added.                          */
                                      /*                                  */

#if   ARCH_LOG2_SIZEOF_LONG == 2
  typedef signed long     TT_F26Dot6; /* 26.6 fixed float, used for       */
#elif ARCH_LOG2_SIZEOF_INT  == 2
  typedef signed int      TT_F26Dot6; /* 26.6 fixed float, used for       */
                                      /* glyph points pixel coordinates.  */
#else
#error "No appropriate type for Fixed 26.6 Floats"
#endif

#if   ARCH_LOG2_SIZEOF_LONG == 2
  typedef signed long     TT_Pos;     /* point position, expressed either */
#elif ARCH_LOG2_SIZEOF_INT  == 2
  typedef signed int     TT_Pos;      /* point position, expressed either */
                                      /* in fractional pixels or notional */
                                      /* units, depending on context. For */
                                      /* example, glyph coordinates       */
                                      /* returned by TT_Load_Glyph are    */
                                      /* expressed in font units when     */
                                      /* scaling wasn't requested, and    */
                                      /* in 26.6 fractional pixels if it  */
                                      /* was                              */
                                      /*                                  */
#else
#error "No appropriate type for point position"
#endif

  struct  _TT_UnitVector      /* guess what...  */
  { 
    TT_F2Dot14  x;
    TT_F2Dot14  y;
  };

  typedef struct _TT_UnitVector  TT_UnitVector;


  struct  _TT_Vector          /* Simple vector type */
  {
    TT_F26Dot6  x;
    TT_F26Dot6  y;
  };

  typedef struct _TT_Vector  TT_Vector;


  /* A simple 2x2 matrix used for transformations. */
  /* You should use 16.16 fixed floats             */
  /*                                               */
  /*  x' = xx*x + xy*y                             */
  /*  y' = yx*x + yy*y                             */
  /*                                               */

  struct  _TT_Matrix                       
  {
    TT_Fixed  xx, xy;
    TT_Fixed  yx, yy;
  };

  typedef struct _TT_Matrix  TT_Matrix;


  /* A structure used to describe the source glyph to the renderer. */

  struct  _TT_Outline
  {
    unsigned int     contours;   /* number of contours in glyph          */
    unsigned int     points;     /* number of points in the glyph        */

    unsigned short*  conEnds;    /* points to an array of each contour's */
                                 /* start point index                    */
    TT_Pos*          xCoord;     /* table of x coordinates               */
    TT_Pos*          yCoord;     /* table of y coordinates               */
    unsigned char*   flag;       /* table of flags                       */

    /* the following flag indicates that the outline owns the arrays it  */
    /* refers to. Typically, this is true of outlines created from the   */
    /* "TT_New_Outline" API, while it isn't for those returned by        */
    /* "TT_Get_Glyph_Outline"                                            */

    int              owner;      /* the outline owns the coordinates,    */
                                 /* flags and contours array it uses     */

    /* the following flags are set automatically by TT_Get_Glyph_Outline */
    /* their meaning is the following :                                  */
    /*                                                                   */
    /*  high_precision   when true, the scan-line converter will use     */
    /*                   a higher precision to render bitmaps (i.e. a    */
    /*                   1/1024 pixel precision). This is important for  */
    /*                   small ppem sizes.                               */
    /*                                                                   */
    /*  second_pass      when true, the scan-line converter performs     */
    /*                   a second sweep phase dedicated to find          */
    /*                   vertical drop-outs. If false, only horizontal   */
    /*                   drop-outs will be checked during the first      */
    /*                   vertical sweep (yes, this is a bit confusing    */
    /*                   but it's really the way it should work).        */
    /*                   This is important for small ppems too.          */
    /*                                                                   */
    /*  dropout_mode     specifies the TrueType drop-out mode to         */
    /*                   use for continuity checking. valid values       */
    /*                   are 0 (no check), 1, 2, 4 and 5                 */
    /*                                                                   */
    /*  Most of the engine's users will safely ignore these fields..     */
    /*                                                                   */

    int              high_precision;  /* high precision rendering */
    int              second_pass;     /* two sweeps rendering     */
    char             dropout_mode;    /* dropout mode */
  };

  typedef struct _TT_Outline  TT_Outline;

 
  /* A structure used to describe a simple bounding box */

  struct _TT_BBox
  {
    TT_Pos  xMin;
    TT_Pos  yMin;
    TT_Pos  xMax;
    TT_Pos  yMax;
  };

  typedef struct _TT_BBox  TT_BBox;

  /* A structure used to return glyph metrics. */

  /* The "bearingX" isn't called "left-side bearing" anymore because    */
  /* it has different meanings depending on the glyph's orientation     */
  /*                                                                    */
  /* The same goes true for "bearingY", which is the top-side bearing   */
  /* defined by the TT_Spec, i.e. the distance from the baseline to the */
  /* top of the glyph's bbox. According to our current convention, this */
  /* is always the same as "bbox.yMax" but we make it appear for        */
  /* consistency in its proper field.                                   */
  /*                                                                    */
  /* the "advance" width is the advance width for horizontal layout,    */
  /* and advance height for vertical layouts.                           */
  /*                                                                    */
  /* Finally, the library (1.0) doesn't support vertical text yet       */
  /* but these changes were introduced to accomodate it, as it will     */
  /* most certainly be introduced in later releases.                    */
  /*                                                                    */

  struct  _TT_Glyph_Metrics
  {
    TT_BBox  bbox;      /* glyph bounding box */

    TT_Pos   bearingX;  /* left-side bearing                    */
    TT_Pos   bearingY;  /* top-side bearing, per se the TT spec */

    TT_Pos   advance;   /* advance width (or height) */
  };

  /* A structure used to return horizontal _and_ vertical glyph metrics */
  /*                                                                    */
  /* A same glyph can be used either in a horizontal or vertical layout */
  /* Its glyph metrics vary with orientation. The Big_Glyph_Metrics     */
  /* is used to return _all_ metrics in one call.                       */
  /*                                                                    */
  /* This structure is currently unused..                               */
  /*                                                                    */

  struct _TT_Big_Glyph_Metrics
  {
    TT_BBox  bbox;          /* glyph bounding box */

    TT_Pos   horiBearingX;  /* left side bearing in horizontal layouts */
    TT_Pos   horiBearingY;  /* top side bearing in horizontal layouts  */

    TT_Pos   vertBearingX;  /* left side bearing in vertical layouts */
    TT_Pos   vertBearingY;  /* top side bearing in vertical layouts  */

    TT_Pos   horiAdvance;   /* advance width for horizontal layout */
    TT_Pos   vertAdvance;   /* advance height for vertical layout  */
  };

  typedef struct _TT_Glyph_Metrics      TT_Glyph_Metrics;
  typedef struct _TT_Big_Glyph_Metrics  TT_Big_Glyph_Metrics;


  /* A structure used to return instance metrics. */

  struct  _TT_Instance_Metrics
  {
    int       pointSize;     /* char. size in points (1pt = 1/72 inch) */

    int       x_ppem;        /* horizontal pixels per EM square */
    int       y_ppem;        /* vertical pixels per EM square   */

    TT_Fixed  x_scale;       /* 16.16 to convert from EM units to 26.6 pix */
    TT_Fixed  y_scale;       /* 16.16 to convert from EM units to 26.6 pix */

    int       x_resolution;  /* device horizontal resolution in dpi */
    int       y_resolution;  /* device vertical resolution in dpi   */
  };

  typedef struct _TT_Instance_Metrics  TT_Instance_Metrics;


  /* Flow constants:                                             */
  /*                                                             */
  /* The flow of a bitmap refers to the way lines are oriented   */
  /* within the bitmap data, i.e., the orientation of the Y      */
  /* coordinate axis.                                            */

  /* for example, if the first bytes of the bitmap pertain to    */
  /* its top-most line, then the flow is 'down'.  If these bytes */
  /* pertain to its lowest line, the the flow is 'up'.           */

#define TT_Flow_Down  -1  /* bitmap is oriented from top to bottom */
#define TT_Flow_Up     1  /* bitmap is oriented from bottom to top */
#define TT_Flow_Error  0  /* an error occurred during rendering    */


  /* A structure used to describe the target bitmap or pixmap to the   */
  /* renderer.  Note that there is nothing in this structure that      */
  /* gives the nature of the buffer.                                   */

  /* IMPORTANT NOTE:                                                   */
  /*                                                                   */
  /*   A pixmap's width _must_ be a multiple of 4.  Clipping           */
  /*   problems will arise otherwise, if not page faults!              */
  /*                                                                   */
  /*   The typical settings are:                                       */
  /*                                                                   */
  /*   - for an WxH bitmap:                                            */
  /*                                                                   */
  /*       rows  = H                                                   */
  /*       cols  = (W+7)/8                                             */
  /*       width = W                                                   */
  /*       flow  = your_choice                                         */
  /*                                                                   */
  /*   - for an WxH pixmap:                                            */
  /*                                                                   */
  /*       rows  = H                                                   */
  /*       cols  = (W+3) & ~3                                          */
  /*       width = cols                                                */
  /*       flow  = your_choice                                         */

  struct  _TT_Raster_Map
  {
    int    rows;    /* number of rows                    */
    int    cols;    /* number of columns (bytes) per row */
    int    width;   /* number of pixels per line         */
    int    flow;    /* bitmap orientation                */

    void*  bitmap;  /* bit/pixmap buffer                 */
    long   size;    /* bit/pixmap size in bytes          */
  };

  typedef struct _TT_Raster_Map  TT_Raster_Map;



  /* ------- The font header TrueType table structure ----- */

  struct  _TT_Header
  {
    TT_Fixed   Table_Version;
    TT_Fixed   Font_Revision;

    TT_Long    CheckSum_Adjust;
    TT_Long    Magic_Number;

    TT_UShort  Flags;
    TT_UShort  Units_Per_EM;

    TT_Long    Created [2];
    TT_Long    Modified[2];

    TT_FWord   xMin;
    TT_FWord   yMin;
    TT_FWord   xMax;
    TT_FWord   yMax;

    TT_UShort  Mac_Style;
    TT_UShort  Lowest_Rec_PPEM;

    TT_Short   Font_Direction;
    TT_Short   Index_To_Loc_Format;
    TT_Short   Glyph_Data_Format;
  };

  typedef struct _TT_Header  TT_Header;


  /* ------- The horizontal header TrueType table structure ----- */

  struct  _TT_Horizontal_Header
  {
    TT_Fixed   Version;
    TT_FWord   Ascender;
    TT_FWord   Descender;
    TT_FWord   Line_Gap;

    TT_UFWord  advance_Width_Max;

    TT_FWord   min_Left_Side_Bearing;
    TT_FWord   min_Right_Side_Bearing;
    TT_FWord   xMax_Extent;
    TT_FWord   caret_Slope_Rise;
    TT_FWord   caret_Slope_Run;

    TT_Short   Reserved[5];

    TT_Short   metric_Data_Format;
    TT_UShort  number_Of_HMetrics;
  };

  typedef struct _TT_Horizontal_Header  TT_Horizontal_Header;


  /* ----------- OS/2 Table ----------------------------- */

  struct  _TT_OS2
  {
    TT_UShort  version;                /* 0x0001 */
    TT_FWord   xAvgCharWidth;
    TT_UShort  usWeightClass;
    TT_UShort  usWidthClass;
    TT_Short   fsType;
    TT_FWord   ySubscriptXSize;
    TT_FWord   ySubscriptYSize;
    TT_FWord   ySubscriptXOffset;
    TT_FWord   ySubscriptYOffset;
    TT_FWord   ySuperscriptXSize;
    TT_FWord   ySuperscriptYSize;
    TT_FWord   ySuperscriptXOffset;
    TT_FWord   ySuperscriptYOffset;
    TT_FWord   yStrikeoutSize;
    TT_FWord   yStrikeoutPosition;
    TT_Short   sFamilyClass;

    char       panose[10];

    TT_ULong   ulUnicodeRange1;        /* Bits 0-31   */
    TT_ULong   ulUnicodeRange2;        /* Bits 32-63  */
    TT_ULong   ulUnicodeRange3;        /* Bits 64-95  */
    TT_ULong   ulUnicodeRange4;        /* Bits 96-127 */

    char       achVendID[4];

    TT_UShort  fsSelection;
    TT_UShort  usFirstCharIndex;
    TT_UShort  usLastCharIndex;
    TT_UShort  sTypoAscender;
    TT_UShort  sTypoDescender;
    TT_UShort  sTypoLineGap;
    TT_UShort  usWinAscent;
    TT_UShort  usWinDescent;

    /* only version 1 tables: */

    TT_ULong   ulCodePageRange1;       /* Bits 0-31   */
    TT_ULong   ulCodePageRange2;       /* Bits 32-63  */
  };

  typedef struct _TT_OS2  TT_OS2;


  /* ----------- Postscript table ------------------------ */

  struct  _TT_Postscript
  {
    TT_Fixed  FormatType;
    TT_Fixed  italicAngle;
    TT_FWord  underlinePosition;
    TT_FWord  underlineThickness;
    TT_ULong  isFixedPitch;
    TT_ULong  minMemType42;
    TT_ULong  maxMemType42;
    TT_ULong  minMemType1;
    TT_ULong  maxMemType1;

    /* glyph names follow in the file, but we don't */
    /* load them by default.                        */
  };

  typedef struct _TT_Postscript  TT_Postscript;


  /* ------------ horizontal device metrics "hdmx" ---------- */

  struct  _TT_Hdmx_Record
  {
    unsigned char   ppem;
    unsigned char   max_width;
    unsigned char*  widths;
  };

  typedef struct _TT_Hdmx_Record  TT_Hdmx_Record;


  struct  _TT_Hdmx
  {
    TT_UShort        version;
    TT_Short         num_records;
    TT_Hdmx_Record*  records;
  };

  typedef struct _TT_Hdmx  TT_Hdmx;


  /* A structure used to describe face properties. */

  struct  _TT_Face_Properties
  {
    int  num_Glyphs;   /* number of glyphs in face              */
    int  max_Points;   /* maximum number of points in a glyph   */
    int  max_Contours; /* maximum number of contours in a glyph */

    int  num_Faces;    /* 0 for normal TrueType files, and the  */
                       /* number of embedded faces minus 1 for  */
                       /* TrueType collections                  */

    TT_Header*             header;        /* TrueType header table      */
    TT_Horizontal_Header*  horizontal;    /* TrueType horizontal header */
    TT_OS2*                os2;           /* TrueType OS/2 table        */
    TT_Postscript*         postscript;    /* TrueType Postscript table  */
    TT_Hdmx*               hdmx;
  };

  typedef struct _TT_Face_Properties  TT_Face_Properties;

  /* Here are the definitions of the handle types used for FreeType's */
  /* most common objects accessed by the client application.  We use  */
  /* a simple trick there:                                            */
  /*                                                                  */
  /*   Each handle type is a structure that only contains one         */
  /*   pointer.  The advantage of structures is that there are        */
  /*   mutually exclusive types.  We could have defined the           */
  /*   following types:                                               */
  /*                                                                  */
  /*     typedef void*  TT_Stream;                                    */
  /*     typedef void*  TT_Face;                                      */
  /*     typedef void*  TT_Instance;                                  */
  /*     typedef void*  TT_Glyph;                                     */
  /*     typedef void*  TT_CharMap;                                   */
  /*                                                                  */
  /*   but these would have allowed lines like:                       */
  /*                                                                  */
  /*      stream = instance;                                          */
  /*                                                                  */
  /*   in the client code this would be a severe bug, unnoticed       */
  /*   by the compiler!                                               */
  /*                                                                  */
  /*   Thus, we enforce type checking with a simple language          */
  /*   trick...                                                       */
  /*                                                                  */
  /*   NOTE:  Some macros are defined in tttypes.h to perform         */
  /*          automatic type conversions for library hackers...       */
  /*                                                                  */

  struct _TT_Engine   { void*  z; };
  struct _TT_Stream   { void*  z; };
  struct _TT_Face     { void*  z; };
  struct _TT_Instance { void*  z; };
  struct _TT_Glyph    { void*  z; };
  struct _TT_CharMap  { void*  z; };

  typedef struct _TT_Engine    TT_Engine;    /* engine instance           */
  typedef struct _TT_Stream    TT_Stream;    /* stream handle type        */
  typedef struct _TT_Face      TT_Face;      /* face handle type          */
  typedef struct _TT_Instance  TT_Instance;  /* instance handle type      */
  typedef struct _TT_Glyph     TT_Glyph;     /* glyph handle type         */
  typedef struct _TT_CharMap   TT_CharMap;   /* character map handle type */

  typedef int  TT_Error;

  extern const TT_Instance   TT_Null_Instance;

  /*******************************************************************/
  /*                                                                 */
  /*  FreeType API                                                   */
  /*                                                                 */
  /*  All these begin with a 'TT_' prefix.                           */
  /*                                                                 */
  /*  Most of them are implemented in the 'ttapi.c' source file.     */
  /*                                                                 */
  /*******************************************************************/

  /* Initialize the engine. */

  TT_Error  TT_Init_FreeType( TT_Engine*  engine );


  /* Finalize the engine, and release all allocated objects. */
  TT_Error  TT_Done_FreeType( TT_Engine  engine );


  /* Set the gray level palette.  This is an array of 5 bytes used */
  /* to produce the font smoothed pixmaps.  By convention:         */
  /*                                                               */
  /*  palette[0] = background (white)                              */
  /*  palette[1] = light                                           */
  /*  palette[2] = medium                                          */
  /*  palette[3] = dark                                            */
  /*  palette[4] = foreground (black)                              */
  /*                                                               */

  TT_Error  TT_Set_Raster_Gray_Palette( TT_Engine  engine, char*  palette );
  
  /* ----------------------- face management ----------------------- */

  /* Open a new TrueType font file, and returns a handle for */
  /* it in variable '*face'.                                 */

  /* Note: the file can be either a TrueType file (*.ttf) or  */
  /*       a TrueType collection (*.ttc), in this case, only  */
  /*       the first face is opened.  The number of faces in  */
  /*       the same collection can be obtained in the face's  */
  /*       properties, through TT_Get_Face_Properties and the */
  /*       'max_Faces' field.                                 */

  TT_Error  TT_Open_Face( TT_Engine    engine,
                          const char*  fontpathname,
                          TT_Face*     face );


  /* Open a TrueType font file located inside a collection. */
  /* The font is designed by its index in 'fontIndex'.      */

  TT_Error  TT_Open_Collection( TT_Engine    engine,
                                const char*  collectionpathname,
                                int          fontIndex,
                                TT_Face*     face );


  /* Return face properties in the 'properties' structure. */

  TT_Error  TT_Get_Face_Properties( TT_Face              face,
                                    TT_Face_Properties*  properties );


  /* Set a face object's generic pointer */
  TT_Error  TT_Set_Face_Pointer( TT_Face  face,
                                 void*    data );

  /* Get a face object's geneeric pointer */
  void*     TT_Get_Face_Pointer( TT_Face  face );

  /* Close a face's file handle to save system resources. The file */
  /* will be re-opened automatically on the next disk access       */
  TT_Error  TT_Flush_Face( TT_Face  face );

  /* Close a given font object, destroying all associated */
  /* instances.                                           */

  TT_Error  TT_Close_Face( TT_Face  face );

  /* Get Font or Table Data */

  TT_Error  TT_Get_Font_Data( TT_Face  face,
                              long     tag,
                              long     offset,
                              void*    buffer,
                              long*    length );

  /* A simply macro to build table tags from ascii chars */
#  define MAKE_TT_TAG( _x1, _x2, _x3, _x4 ) \
            (_x1 << 24 | _x2 << 16 | _x3 << 8 | _x4)

  /* ----------------------- instance management -------------------- */

  /* Open a new font instance and returns an instance handle */
  /* for it in '*instance'.                                  */

  TT_Error  TT_New_Instance( TT_Face       face,
                             TT_Instance*  instance );


  /* Set device resolution for a given instance.  The values are      */
  /* given in dpi (Dots Per Inch).  Default is 96 in both directions. */

  /* NOTE:  y_resolution is currently ignored, and the library */
  /*        assumes square pixels.                             */

  TT_Error  TT_Set_Instance_Resolutions( TT_Instance  instance,
                                         int          x_resolution,
                                         int          y_resolution );


  /* Set the pointsize for a given instance.  Default is 10pt. */

  TT_Error  TT_Set_Instance_CharSize( TT_Instance  instance,
                                      TT_F26Dot6   charSize );

  TT_Error  TT_Set_Instance_CharSizes( TT_Instance  instance,
                                       TT_F26Dot6   charWidth,
                                       TT_F26Dot6   charHeight );

#define TT_Set_Instance_PointSize( ins, ptsize )   \
            TT_Set_Instance_CharSize( ins, ptsize*64 )

  TT_Error  TT_Set_Instance_PixelSizes( TT_Instance  instance,
                                        int          pixelWidth,
                                        int          pixelHeight,
                                        TT_F26Dot6   pointSize );

  /* We do not provide direct support for rotation or stretching  */
  /* in the glyph loader. This means that you must perform these  */
  /* operations yourself through TT_Transform_Outline  before     */
  /* calling TT_Get_Glyph_Bitmap.  However, the loader needs to   */
  /* know what kind of text you're displaying.  The following     */
  /* boolean flags inform the interpreter that:                   */
  /*                                                              */
  /*   rotated  : the glyphs will be rotated                      */
  /*   stretched: the glyphs will be stretched                    */
  /*                                                              */
  /* These setting only affect the hinting process!               */
  /*                                                              */
  /* NOTE: 'stretched' means any transform that distorts the      */
  /*       glyph (including skewing and 'linear stretching')      */
  /*                                                              */

  TT_Error  TT_Set_Instance_Transform_Flags( TT_Instance  instance,
                                             int          rotated,
                                             int          stretched );

  /* Return instance metrics in 'metrics'. */

  TT_Error  TT_Get_Instance_Metrics( TT_Instance           instance,
                                     TT_Instance_Metrics*  metrics );


  /* Set an instance's generic pointer */

  TT_Error  TT_Set_Instance_Pointer( TT_Instance  instance,
                                     void*        data );

  /* Get an instance's generic pointer */

  void*     TT_Get_Instance_Pointer( TT_Instance  instance );


  /* Close a given instance object, destroying all associated */
  /* data.                                                    */

  TT_Error  TT_Done_Instance( TT_Instance  instance );


  /* ----------------------- glyph management ----------------------- */

  /* Create a new glyph object related to the given 'face'. */

  TT_Error  TT_New_Glyph( TT_Face    face,
                          TT_Glyph*  glyph );


  /* Discard (and destroy) a given glyph object. */

  TT_Error  TT_Done_Glyph( TT_Glyph  glyph );


#define TTLOAD_SCALE_GLYPH  1
#define TTLOAD_HINT_GLYPH   2

#define TTLOAD_DEFAULT  (TTLOAD_SCALE_GLYPH | TTLOAD_HINT_GLYPH)

  /* load and process (scale/transform and hint) a glyph from the */
  /* given 'instance'.  The glyph and instance handles must be    */
  /* related to the same face object.  The glyph index can be     */
  /* computed with a call to TT_Char_Index().                     */

  /* the 'load_flags' argument is a combination of the macros     */
  /* TTLOAD_SCALE_GLYPH and TTLOAD_HINT_GLYPH.  Hinting will be   */
  /* applied only if the scaling is selected.                     */

  /* When scaling is off (i.e. load_flags = 0), the returned      */
  /* outlines are in EM square coordinates (also called FUnits),  */
  /* extracted directly from the font with no hinting.            */
  /* Other glyph metrics are also in FUnits.                      */

  /* When scaling is on, the returned outlines are in fractional  */
  /* pixel units (i.e. TT_F26Dot6 = 26.6 fixed floats).           */

  /* NOTE:  the glyph index must be in the range 0..num_glyphs-1  */
  /*        where 'num_glyphs' is the total number of glyphs in   */
  /*        the font file (given in the face properties).         */

  TT_Error  TT_Load_Glyph( TT_Instance  instance,
                           TT_Glyph     glyph,
                           int          glyph_index,
                           int          load_flags );


  /* Return glyph outline pointers in 'outline'.  Note that the returned */
  /* pointers are owned by the glyph object, and will be destroyed with  */
  /* it.  The client application should _not_ change the pointers.       */
  
  TT_Error  TT_Get_Glyph_Outline( TT_Glyph     glyph,
                                  TT_Outline*  outline );

  /* Copy the glyph metrics into 'metrics'. */

  TT_Error  TT_Get_Glyph_Metrics( TT_Glyph           glyph,
                                  TT_Glyph_Metrics*  metrics );


  /* Render the glyph into a bitmap, with given position offsets.     */

  /*  Note : Only use integer pixel offsets to preserve the fine      */
  /*         hinting of the glyph and the 'correct' anti-aliasing     */
  /*         (where vertical and horizontal stems aren't grayed).     */
  /*         This means that x_offset and y_offset must be multiples  */
  /*         of 64!                                                   */
  /*                                                                  */

  TT_Error  TT_Get_Glyph_Bitmap( TT_Glyph        glyph,
                                 TT_Raster_Map*  raster_map,
                                 TT_F26Dot6      x_offset,
                                 TT_F26Dot6      y_offset );


  /* Render the glyph into a pixmap, with given position offsets.     */

  /*  Note : Only use integer pixel offsets to preserve the fine      */
  /*         hinting of the glyph and the 'correct' anti-aliasing     */
  /*         (where vertical and horizontal stems aren't grayed).     */
  /*         This means that x_offset and y_offset must be multiples  */
  /*         of 64!                                                   */
  /*                                                                  */

  TT_Error  TT_Get_Glyph_Pixmap( TT_Glyph        glyph,
                                 TT_Raster_Map*  raster_map,
                                 TT_F26Dot6      x_offset,
                                 TT_F26Dot6      y_offset );

  /* ----------------------- outline support ------------------------ */

  /* Allocate a new outline. Reserve space for 'num_points' and */
  /* 'num_contours'                                             */

  TT_Error  TT_New_Outline( int          num_points,
                            int          num_contours,
                            TT_Outline*  outline );

  /* Release an outline */

  TT_Error  TT_Done_Outline( TT_Outline*  outline );

  /* Copy an outline into another one */

  TT_Error  TT_Copy_Outline( TT_Outline*  source,
                             TT_Outline*  target );

  /* Render an outline into a bitmap */

  TT_Error  TT_Get_Outline_Bitmap( TT_Engine       engine,
                                   TT_Outline*     outline,
                                   TT_Raster_Map*  raster_map );

  /* Render an outline into a pixmap - note that this function uses */
  /* a different pixel scale, where 1.0 pixels = 128          XXXX  */

  TT_Error  TT_Get_Outline_Pixmap( TT_Engine       engine,
                                   TT_Outline*     outline,
                                   TT_Raster_Map*  raster_map );

  /* return an outline's bounding box - this function is slow as it */
  /* performs a complete scan-line process, without drawing, to get */
  /* the most accurate values                                 XXXX  */

  TT_Error  TT_Get_Outline_BBox( TT_Outline*  outline,
                                 TT_BBox*     bbox );

  /* Apply a transformation to a glyph outline */

  void      TT_Transform_Outline( TT_Outline*  outline,
                                  TT_Matrix*   matrix );

  /* backwards compatibility macro */
#  define TT_Appy_Outline_Matrix  TT_Transform_Matrix;

  /* Apply a translation to a glyph outline */

  void      TT_Translate_Outline( TT_Outline*  outline,
                                  TT_F26Dot6   x_offset,
                                  TT_F26Dot6   y_offset );

  /* backwards compatibility */
#  define TT_Apply_Outline_Translation  TT_Translate_Outline

  /* Apply a transformation to a vector */

  void      TT_Transform_Vector( TT_F26Dot6*  x,
                                 TT_F26Dot6*  y,
                                 TT_Matrix*   matrix );
  /* backwards compatibility */
#  define TT_Apply_Vector_Matrix( x, y, m )   \
               TT_Transform_Vector( x, y, m )

  /* Multiply a matrix with another - computes "b := a*b" */

  void      TT_Matrix_Multiply( TT_Matrix*  a,
                                TT_Matrix*  b );
                 
  /* Invert a transformation matrix */

  TT_Error  TT_Matrix_Invert( TT_Matrix*  matrix );



  /* ----------------- character mappings support ------------- */

  /* Return the number of character mappings found in this file. */
  /* Returns -1 in case of failure (invalid face handle).        */

  int  TT_Get_CharMap_Count( TT_Face  face );


  /* Return the ID of charmap number 'charmapIndex' of a given face */
  /* used to enumerate the charmaps present in a TrueType file.     */

  TT_Error  TT_Get_CharMap_ID( TT_Face  face,
                               int      charmapIndex,
                               short*   platformID,
                               short*   encodingID );


  /* Look up the character maps found in 'face' and return a handle */
  /* for the one matching 'platformID' and 'platformEncodingID'     */
  /* (see the TrueType specs relating to the 'cmap' table for       */
  /* information on these ID numbers).  Returns an error code.      */
  /* In case of failure, the handle is set to NULL and is invalid.  */

  TT_Error  TT_Get_CharMap( TT_Face      face,
                            int          charmapIndex,
                            TT_CharMap*  charMap );


  /* Translate a character code through a given character map   */
  /* and return the corresponding glyph index to be used in     */
  /* a TT_Load_Glyph call.  This function returns -1 in case of */
  /* failure.                                                   */

  int  TT_Char_Index( TT_CharMap      charMap,
                      unsigned short  charCode );



  /* --------------------- names table support ------------------- */

  /* Return the number of name strings found in the name table. */
  /* Returns -1 in case of failure (invalid face handle).       */

  int  TT_Get_Name_Count( TT_Face  face );


  /* Return the ID of the name number 'nameIndex' of a given face */
  /* used to enumerate the charmaps present in a TrueType file.   */

  TT_Error  TT_Get_Name_ID( TT_Face  face,
                            int      nameIndex,
                            short*   platformID,
                            short*   encodingID,
                            short*   languageID,
                            short*   nameID );


  /* Return the address and length of the name number 'nameIndex' */
  /* of a given face.  The string is part of the face object and  */
  /* shouldn't be written to or released.                         */

  /* Note that if the string platformID is not in the range 0..3, */
  /* a null pointer will be returned                              */

  TT_Error  TT_Get_Name_String( TT_Face  face,
                                int      nameIndex,
                                char**   stringPtr, /* pointer address     */
                                int*     length );  /* str. length address */



  /************************ callback definition ******************/

  /* NOTE : Callbacks are still in beta stage, they'll be used to */
  /*        perform efficient glyph outline caching.              */

  /* There is currently only one callback defined */
#define  TT_Callback_Glyph_Outline_Load  0

  /* The glyph loader callback type defines a function that will */
  /* be called by the TrueType glyph loader to query an already  */
  /* cached glyph outline from higher-level libraries. Normal    */
  /* clients of the TrueType engine shouldn't worry about this   */

  typedef  int (*TT_Glyph_Loader_Callback)( void*        instance_ptr,
                                            int          glyph_index,
                                            TT_Outline*  outline,
                                            TT_F26Dot6*  lsb,
                                            TT_F26Dot6*  aw );

  /* Register a new callback to the TrueType engine - this should */
  /* only be used by higher-level libraries, not typical clients  */

  TT_Error  TT_Register_Callback( TT_Engine  engine,
                                  int        callback_id,
                                  void*      callback_ptr );

  /************************ error codes declaration **************/
  
  /* The error codes are grouped in 'classes' used to indicate the */
  /* 'level' at which the error happened.                          */
  /* The class is given by an error code's high byte.              */


  /* ------------- Success is always 0 -------- */

#define TT_Err_Ok                       0

  
  /* -------- High-level API error codes ------ */
  
#define TT_Err_Invalid_Face_Handle      0x001
#define TT_Err_Invalid_Instance_Handle  0x002
#define TT_Err_Invalid_Glyph_Handle     0x003
#define TT_Err_Invalid_CharMap_Handle   0x004
#define TT_Err_Invalid_Result_Address   0x005
#define TT_Err_Invalid_Glyph_Index      0x006
#define TT_Err_Invalid_Argument         0x007
#define TT_Err_Could_Not_Open_File      0x008
#define TT_Err_File_Is_Not_Collection   0x009
  
#define TT_Err_Table_Missing            0x00A
#define TT_Err_Invalid_Horiz_Metrics    0x00B
#define TT_Err_Invalid_CharMap_Format   0x00C
#define TT_Err_Invalid_PPem             0x00D

#define TT_Err_Invalid_File_Format      0x010

#define TT_Err_Invalid_Engine           0x020
#define TT_Err_Too_Many_Extensions      0x021
#define TT_Err_Extensions_Unsupported   0x022
#define TT_Err_Invalid_Extension_Id     0x023

#define TT_Err_Max_Profile_Missing      0x080
#define TT_Err_Header_Table_Missing     0x081
#define TT_Err_Horiz_Header_Missing     0x082
#define TT_Err_Locations_Missing        0x083
#define TT_Err_Name_Table_Missing       0x084
#define TT_Err_CMap_Table_Missing       0x085
#define TT_Err_Hmtx_Table_Missing       0x086
#define TT_Err_OS2_Table_Missing        0x087
#define TT_Err_Post_Table_Missing       0x088

  
  /* -------- Memory component error codes ---- */

  /* this error indicates that an operation cannot */
  /* be performed due to memory exhaustion.        */

#define TT_Err_Out_Of_Memory            0x100

  
  /* -------- File component error codes ------ */
  
  /* these error codes indicate that the file could */
  /* not be accessed properly.  Usually, this means */
  /* a broken font file!                            */

#define TT_Err_Invalid_File_Offset      0x200
#define TT_Err_Invalid_File_Read        0x201
#define TT_Err_Invalid_Frame_Access     0x202

  
  /* -------- Glyph loader error codes -------- */
  
  /* Produced only by the glyph loader, these error */
  /* codes indicate a broken glyph in a font file.  */

#define TT_Err_Too_Many_Points          0x300
#define TT_Err_Too_Many_Contours        0x301
#define TT_Err_Invalid_Composite        0x302
#define TT_Err_Too_Many_Ins             0x303

  
  /* --- bytecode interpreter error codes ----- */
  
  /* These error codes are produced by the TrueType */
  /* bytecode interpreter.  They usually indicate a */
  /* broken font file, a broken glyph within a font */
  /* file, or a bug in the interpreter!             */

#define TT_Err_Invalid_Opcode           0x400
#define TT_Err_Too_Few_Arguments        0x401
#define TT_Err_Stack_Overflow           0x402
#define TT_Err_Code_Overflow            0x403
#define TT_Err_Bad_Argument             0x404
#define TT_Err_Divide_By_Zero           0x405
#define TT_Err_Storage_Overflow         0x406
#define TT_Err_Cvt_Overflow             0x407
#define TT_Err_Invalid_Reference        0x408
#define TT_Err_Invalid_Distance         0x409
#define TT_Err_Interpolate_Twilight     0x40A
#define TT_Err_Debug_OpCode             0x40B
#define TT_Err_ENDF_In_Exec_Stream      0x40C
#define TT_Err_Out_Of_CodeRanges        0x40D
#define TT_Err_Nested_DEFS              0x40E 
#define TT_Err_Invalid_CodeRange        0x40F
#define TT_Err_Invalid_Displacement     0x410


  /* ------ internal failure error codes ----- */

  /* These error codes are produced when an incoherent */
  /* library state has been detected.  These reflect a */
  /* severe bug in the engine! (or a major overwrite   */
  /* of your application into the library's data).     */
  
#define TT_Err_Nested_Frame_Access      0x500
#define TT_Err_Invalid_Cache_List       0x501
#define TT_Err_Could_Not_Find_Context   0x502
#define TT_Err_Unlisted_Object          0x503

  
  /* ---- scan-line converter error codes ----- */
  
  /* These error codes are produced by the raster component.  */
  /* They indicate that an outline structure was incoherently */
  /* setup, or that you're trying to render an horribly       */
  /* complex glyph!                                           */

#define TT_Err_Raster_Pool_Overflow     0x600
#define TT_Err_Raster_Negative_Height   0x601
#define TT_Err_Raster_Invalid_Value     0x602
#define TT_Err_Raster_Not_Initialized   0x603
  

#ifdef __cplusplus
  }
#endif

#endif /* FREETYPE_H */


/* END */
