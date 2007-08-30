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

/* $Id: ttobjs.h,v 1.6 2004/10/06 11:32:17 igor Exp $ */

/* Changes after FreeType: cut out the TrueType instruction interpreter. */


/*******************************************************************
 *
 *  ttobjs.h                                                     1.0
 *
 *    Objects definition unit.
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

#ifndef TTOBJS_H
#define TTOBJS_H

#include "ttcommon.h"
#include "tttypes.h"
#include "tttables.h"
#include <setjmp.h>

#ifdef __cplusplus
  extern "C" {
#endif

/*                                                                       */
/*  This file contains the definitions and methods of the four           */
/*  kinds of objects managed by the FreeType engine.  These are:         */
/*                                                                       */
/*                                                                       */
/*   Face objects:                                                       */
/*                                                                       */
/*     There is always one face object per opened TrueType font          */
/*     file, and only one.  The face object contains data that is        */
/*     independent of current transform/scaling/rotation and             */
/*     pointsize, or glyph index.  This data is made of several          */
/*     critical tables that are loaded on face object creation.          */
/*                                                                       */
/*     A face object tracks all active and recycled objects of           */
/*     the instance and execution context classes.  Destroying a face    */
/*     object will automatically destroy all associated instances.       */
/*                                                                       */
/*                                                                       */
/*   Instance objects:                                                   */
/*                                                                       */
/*     An instance object always relates to a given face object,         */
/*     known as its 'parent' or 'owner', and contains only the           */
/*     data that is specific to one given pointsize/transform of         */
/*     the face.  You can only create an instance from a face object.    */
/*                                                                       */
/*     An instance's current transform/pointsize can be changed          */
/*     at any time using a single high-level API call,                   */
/*     TT_Reset_Instance().                                              */
/*                                                                       */
/*   Execution Context objects:                                          */
/*                                                                       */
/*     An execution context (or context in short) relates to a face.     */
/*     It contains the data and tables that are necessary to load        */
/*     and hint (i.e. execute the glyph instructions of) one glyph.      */
/*     A context is a transient object that is queried/created on        */
/*     the fly: client applications never deal with them directly.       */
/*                                                                       */
/*                                                                       */
/*   Glyph objects:                                                      */
/*                                                                       */
/*     A glyph object contains only the minimal glyph information        */
/*     needed to render one glyph correctly.  This means that a glyph    */
/*     object really contains tables that are sized to hold the          */
/*     contents of _any_ glyph of a given face.  A client application    */
/*     can usually create one glyph object for a given face, then use    */
/*     it for all subsequent loads.                                      */
/*                                                                       */
/*   Here is an example of a client application :                        */
/*   (NOTE: No error checking performed here!)                           */
/*                                                                       */
/*                                                                       */
/*     TT_Face       face;         -- face handle                        */
/*     TT_Instance   ins1, ins2;   -- two instance handles               */
/*     TT_Glyph      glyph;        -- glyph handle                       */
/*                                                                       */
/*     TT_Init_FreeType();                                               */
/*                                                                       */
/*     -- Initialize the engine.  This must be done prior to _any_       */
/*        operation.                                                     */
/*                                                                       */
/*     TT_Open_Face( "/some/face/name.ttf", &face );                     */
/*                                                                       */
/*     -- create the face object.  This call opens the font file         */
/*                                                                       */
/*     TT_New_Instance( face, &ins1 );                                   */
/*     TT_New_Instance( face, &ins2 );                                   */
/*                                                                       */
/*     TT_Set_Instance_PointSize( ins1, 8 );                             */
/*     TT_Set_Instance_PointSize( ins2, 12 );                            */
/*                                                                       */
/*     -- create two distinct instances of the same face                 */
/*     -- ins1  is pointsize 8 at resolution 96 dpi                      */
/*     -- ins2  is pointsize 12 at resolution 96 dpi                     */
/*                                                                       */
/*     TT_New_Glyph( face, &glyph );                                     */
/*                                                                       */
/*     -- create a new glyph object which will receive the contents      */
/*        of any glyph of 'face'                                         */
/*                                                                       */
/*     TT_Load_Glyph( ins1, glyph, 64, DEFAULT_GLYPH_LOAD );             */
/*                                                                       */
/*     -- load glyph indexed 64 at pointsize 8 in the 'glyph' object     */
/*     -- NOTE: This call will fail if the instance and the glyph        */
/*              do not relate to the same face object.                   */
/*                                                                       */
/*     TT_Get_Outline( glyph, &outline );                                */
/*                                                                       */
/*     -- extract the glyph outline from the object and copies it        */
/*        to the 'outline' record                                        */
/*                                                                       */
/*     TT_Get_Metrics( glyph, &metrics );                                */
/*                                                                       */
/*     -- extract the glyph metrics and put them into the 'metrics'      */
/*        record                                                         */
/*                                                                       */
/*     TT_Load_Glyph( ins2, glyph, 64, DEFAULT_GLYPH_LOAD );             */
/*                                                                       */
/*     -- load the same glyph at pointsize 12 in the 'glyph' object      */
/*                                                                       */
/*                                                                       */
/*     TT_Close_Face( &face );                                           */
/*                                                                       */
/*     -- destroy the face object.  This will destroy 'ins1' and         */
/*        'ins2'.  However, the glyph object will still be available     */
/*                                                                       */
/*     TT_Done_FreeType();                                               */
/*                                                                       */
/*     -- Finalize the engine.  This will also destroy all pending       */
/*        glyph objects (here 'glyph').                                  */
/*                                                                       */
/*                                                                       */

  struct _TFace;
  struct _TInstance;
  struct _TExecution_Context;
  struct _TGlyph;

#ifndef TFace_defined
#define TFace_defined
typedef struct _TFace  TFace;
#endif
  typedef TFace*         PFace;

#ifndef TInstance_defined
#define TInstance_defined
typedef struct _TInstance TInstance;
#endif
  typedef TInstance*         PInstance;

#ifndef TExecution_Context_defined
#define TExecution_Context_defined
typedef struct _TExecution_Context TExecution_Context;
#endif
  typedef TExecution_Context*         PExecution_Context;

  typedef struct _TGlyph  TGlyph;
  typedef TGlyph*         PGlyph;


  /*************************************************************/
  /*                                                           */
  /*  ADDITIONAL SUBTABLES                                     */
  /*                                                           */
  /*  These tables are not precisely defined by the specs      */
  /*  but their structures is implied by the TrueType font     */
  /*  file layout.                                             */
  /*                                                           */
  /*************************************************************/

  /* Graphics State                            */
  /*                                           */
  /* The Graphics State (GS) is managed by the */
  /* instruction field, but does not come from */
  /* the font file.  Thus, we can use 'int's   */
  /* where needed.                             */

  struct  _TGraphicsState
  {
    Int            rp0;
    Int            rp1;
    Int            rp2;

    TT_UnitVector  dualVector;
    TT_UnitVector  projVector;
    TT_UnitVector  freeVector;

    Long           loop;
    TT_F26Dot6     minimum_distance;
    Int            round_state;

    Bool           auto_flip;
    TT_F26Dot6     control_value_cutin;
    TT_F26Dot6     single_width_cutin;
    TT_F26Dot6     single_width_value;
    Int            delta_base;
    Int            delta_shift;

    Byte           instruct_control;
    Bool           scan_control;
    Int            scan_type;

    Int            gep0;
    Int            gep1;
    Int            gep2;

  };

  typedef struct _TGraphicsState  TGraphicsState;


  extern const TGraphicsState  Default_GraphicsState;


  /*************************************************************/
  /*                                                           */
  /*  EXECUTION SUBTABLES                                      */
  /*                                                           */
  /*  These sub-tables relate to instruction execution.        */
  /*                                                           */
  /*************************************************************/

#  define MAX_CODE_RANGES   3

  /* There can only be 3 active code ranges at once:   */
  /*   - the Font Program                              */
  /*   - the CVT Program                               */
  /*   - a glyph's instructions set                    */

#  define TT_CodeRange_Font  1
#  define TT_CodeRange_Cvt   2
#  define TT_CodeRange_Glyph 3


  struct  _TCodeRange
  {
    PByte  Base;
    Int    Size;
  };

  typedef struct _TCodeRange  TCodeRange;
  typedef TCodeRange*         PCodeRange;


  /* Defintion of a code range                                       */
  /*                                                                 */
  /* Code ranges can be resident to a glyph (i.e. the Font Program)  */
  /* while some others are volatile (Glyph instructions).            */
  /* Tracking the state and presence of code ranges allows function  */
  /* and instruction definitions within a code range to be forgotten */
  /* when the range is discarded.                                    */

  typedef TCodeRange  TCodeRangeTable[MAX_CODE_RANGES];

  /* defines a function/instruction definition record */

  struct  _TDefRecord
  {
    Int   Range;      /* in which code range is it located ? */
    Int   Start;      /* where does it start ?               */
    Byte  Opc;        /* function #, or instruction code     */
    Bool  Active;     /* is it active ?                      */
  };

  typedef struct _TDefRecord  TDefRecord;
  typedef TDefRecord*         PDefRecord;
  typedef TDefRecord*         PDefArray;

  /* defines a call record, used to manage function calls. */

  struct  _TCallRecord
  {
    Int  Caller_Range;
    Int  Caller_IP;
    Int  Cur_Count;
    Int  Cur_Restart;
  };

  typedef struct _TCallRecord  TCallRecord;
  typedef TCallRecord*         PCallRecord;
  typedef TCallRecord*         PCallStack;  /* defines a simple call stack */


  /* This type defining a set of glyph points will be used to represent */
  /* each zone (regular and twilight) during instructions decoding.     */
  struct  _TGlyph_Zone
  {
    int           n_points;   /* number of points in zone */
    int           n_contours; /* number of contours       */

    PCoordinates  org_x;      /* original points coordinates */
    PCoordinates  org_y;      /* original points coordinates */
    PCoordinates  cur_x;      /* current points coordinates  */
    PCoordinates  cur_y;      /* current points coordinates  */

    Byte*         touch;      /* current touch flags         */
    Short*        contours;   /* contour end points          */
  };

  typedef struct _TGlyph_Zone  TGlyph_Zone;
  typedef TGlyph_Zone         *PGlyph_Zone;


  
#ifndef TT_STATIC_INTERPRETER  /* indirect implementation */

#define EXEC_OPS   PExecution_Context exc,
#define EXEC_OP    PExecution_Context exc
#define EXEC_ARGS  exc,
#define EXEC_ARG   exc
  
#else                          /* static implementation */

#define EXEC_OPS   /* void */
#define EXEC_OP    /* void */
#define EXEC_ARGS  /* void */
#define EXEC_ARG   /* void */
  
#endif

  /* Rounding function, as used by the interpreter */
  typedef TT_F26Dot6  (*TRound_Function)( EXEC_OPS TT_F26Dot6 distance,
                                                   TT_F26Dot6 compensation );
 
  /* Point displacement along the freedom vector routine, as */
  /* used by the interpreter                                 */
  typedef void  (*TMove_Function)( EXEC_OPS PGlyph_Zone zone,
                                            Int         point,
                                            TT_F26Dot6  distance );
 
  /* Distance projection along one of the proj. vectors, as used */
  /* by the interpreter                                          */
  typedef TT_F26Dot6  (*TProject_Function)( EXEC_OPS TT_F26Dot6 Vx,
                                                     TT_F26Dot6 Vy );
  
  /* reading a cvt value. Take care of non-square pixels when needed */
  typedef TT_F26Dot6  (*TGet_CVT_Function)( EXEC_OPS  Int index );

  /* setting or moving a cvt value.  Take care of non-square pixels  */
  /* when needed                                                     */
  typedef void  (*TSet_CVT_Function)( EXEC_OPS  Int         index,
                                                TT_F26Dot6  value );

  /* subglyph transformation record */
  struct  _TTransform
  {
    TT_Fixed    xx, xy; /* transformation */
    TT_Fixed    yx, yy; /*     matrix     */
    TT_F26Dot6  ox, oy; /*    offsets     */
  };

  typedef struct _TTransform  TTransform;
  typedef TTransform         *PTransform;

  /* subglyph loading record.  Used to load composite components */
  struct  _TSubglyph_Record
  {
    Int          index;        /* subglyph index           */
    Bool         is_scaled;    /* is the subglyph scaled?  */
    Bool         is_hinted;    /* should it be hinted?     */
    Bool         preserve_pps; /* preserve phantom points? */

    Long         file_offset;

    TT_BBox      bbox;

    TGlyph_Zone  zone;

    Int          arg1;  /* first argument  */
    Int          arg2;  /* second argument */

    Int          element_flag;    /* current load element flag */

    TTransform   transform;       /* transform */

    TT_Vector    pp1, pp2;        /* phantom points */

    Int          leftBearing;     /* in FUnits */
    Int          advanceWidth;    /* in FUnits */
  };

  typedef struct _TSubglyph_Record  TSubglyph_Record;
  typedef TSubglyph_Record*         PSubglyph_Record;
  typedef TSubglyph_Record*         PSubglyph_Stack;

  /* A note regarding non-squared pixels:                                */
  /*                                                                     */
  /* (This text will probably go into some docs at some time, for        */
  /*  now, it is kept there to explain some definitions in the           */
  /*  TIns_Metrics record).                                              */
  /*                                                                     */
  /* The CVT is a one-dimensional array containing values that           */
  /* control certain important characteristics in a font, like           */
  /* the height of all capitals, all lowercase letter, default           */
  /* spacing or stem width/height.                                       */
  /*                                                                     */
  /* These values are found in FUnits in the font file, and must be      */
  /* scaled to pixel coordinates before being used by the CVT and        */
  /* glyph programs.  Unfortunately, when using distinct x and y         */
  /* resolutions (or distinct x and y pointsizes), there are two         */
  /* possible scalings.                                                  */
  /*                                                                     */
  /* A first try was to implement a 'lazy' scheme where all values       */
  /* were scaled when first used.  However, while some values are always */
  /* used in the same direction, and some other are used in many         */
  /* different circumstances and orientations.                           */
  /*                                                                     */
  /* I have found a simpler way to do the same, and it even seems to     */
  /* work in most of the cases:                                          */
  /*                                                                     */
  /* - all CVT values are scaled to the maximum ppem size                */
  /*                                                                     */
  /* - when performing a read or write in the CVT, a ratio factor        */
  /*   is used to perform adequate scaling. Example:                     */
  /*                                                                     */
  /*    x_ppem = 14                                                      */
  /*    y_ppem = 10                                                      */
  /*                                                                     */
  /*   we choose ppem = x_ppem = 14 as the CVT scaling size.  All cvt    */
  /*   entries are scaled to it.                                         */
  /*                                                                     */
  /*    x_ratio = 1.0                                                    */
  /*    y_ratio = y_ppem/ppem (< 1.0)                                    */
  /*                                                                     */
  /*   we compute the current ratio like:                                */
  /*                                                                     */
  /*     - if projVector is horizontal,                                  */
  /*         ratio = x_ratio = 1.0                                       */
  /*     - if projVector is vertical,                                    */
  /*         ratop = y_ratio                                             */
  /*     - else,                                                         */
  /*         ratio = sqrt((proj.x*x_ratio)^2 + (proj.y*y_ratio)^2)       */
  /*                                                                     */
  /*   reading a cvt value returns      ratio * cvt[index]               */
  /*   writing a cvt value in pixels    cvt[index] / ratio               */
  /*                                                                     */
  /*   the current ppem is simply       ratio * ppem                     */
  /*                                                                     */

  /* metrics used by the instance and execution context objects */
  struct  _TIns_Metrics
  {
    TT_F26Dot6  pointSize;      /* point size.  1 point = 1/72 inch. */

    Int         x_resolution;   /* device horizontal resolution in dpi. */
    Int         y_resolution;   /* device vertical resolution in dpi.   */

    Int         x_ppem;         /* horizontal pixels per EM */
    Int         y_ppem;         /* vertical pixels per EM   */

    Long        x_scale1;
    Long        x_scale2;    /* used to scale FUnits to fractional pixels */

    Long        y_scale1;
    Long        y_scale2;    /* used to scale FUnits to fractional pixels */

    /* for non-square pixels */
    Long        x_ratio;
    Long        y_ratio;

    Int         ppem;        /* maximum ppem size */
    Long        ratio;       /* current ratio     */
    Long        scale1;
    Long        scale2;      /* scale for ppem */

    TT_F26Dot6  compensations[4];  /* device-specific compensations */

    Bool        rotated;        /* `is the glyph rotated?'-flag   */
    Bool        stretched;      /* `is the glyph stretched?'-flag */
  };

  typedef struct _TIns_Metrics  TIns_Metrics;
  typedef TIns_Metrics         *PIns_Metrics;



  /***********************************************************************/
  /*                                                                     */
  /*                         FreeType Face Type                          */
  /*                                                                     */
  /***********************************************************************/

  struct  _TFace
  {
    ttfReader *r;
    ttfFont *font;

    /* maximum profile table, as found in the TrueType file */
    TMaxProfile  maxProfile;

    /* Note:                                          */
    /*  it seems that some maximum values cannot be   */
    /*  taken directly from this table, but rather by */
    /*  combining some of its fields; e.g. the max.   */
    /*  number of points seems to be given by         */
    /*  MAX( maxPoints, maxCompositePoints )          */
    /*                                                */
    /*  For this reason, we define later our own      */
    /*  max values that are used to load and allocate */
    /*  further tables.                               */

    /* The glyph locations table */
    Int       numLocations;

    /* The HMTX table data, used to compute both left */
    /* side bearing and advance width for all glyphs  */

    /* the font program, if any */
    Int    fontPgmSize;
    PByte  fontProgram;

    /* the cvt program, if any */
    Int    cvtPgmSize;
    PByte  cvtProgram;

    /* the original, unscaled, control value table */
    Int    cvtSize;
    PShort cvt;

    /* The following values _must_ be set by the */
    /* maximum profile loader                    */

    Int  numGlyphs;      /* the face's total number of glyphs */
    Int  maxPoints;      /* max glyph points number, simple and composite */
    Int  maxContours;    /* max glyph contours number, simple and composite */
    Int  maxComponents;  /* max components in a composite glyph */

  };



  /***********************************************************************/
  /*                                                                     */
  /*                       FreeType Instance Type                        */
  /*                                                                     */
  /***********************************************************************/

  struct  _TInstance
  {
    PFace            face;     /* face object */

    Bool             valid;

    TIns_Metrics     metrics;

    Int              numFDefs;  /* number of function definitions */
    PDefArray        FDefs;     /* table of FDefs entries         */

    Int              numIDefs;  /* number of instruction definitions */
    PDefArray        IDefs;     /* table of IDefs entries            */
    Int		     countIDefs;/* The number of defined IDefs (igorm). */
    Byte	     IDefPtr[256]; /* Map opcodes to indices of IDefs (igorm). */

    TCodeRangeTable  codeRangeTable;

    TGraphicsState   GS;
    TGraphicsState   default_GS;

    Int              cvtSize;   /* the scaled control value table */
    PLong            cvt;

    Int              storeSize; /* The storage area is now part of the */
    PStorage            storage;   /* instance                            */

  };


  /***********************************************************************/
  /*                                                                     */
  /*                  FreeType Execution Context Type                    */
  /*                                                                     */
  /***********************************************************************/

  struct  _TExecution_Context 
  {
    PFace           current_face;

    /* instructions state */
 
    Int             error;     /* last execution error */
  
    Int             curRange;  /* current code range number   */
    PByte           code;      /* current code range          */
    Int             IP;        /* current instruction pointer */
    Int             codeSize;  /* size of current range       */
  
    Byte            opcode;    /* current opcode              */
    Int             length;    /* length of current opcode    */
  
    Bool            step_ins;  /* true if the interpreter must */
                                /* increment IP after ins. exec */
  
    Int             numFDefs;  /* number of function defs */
    PDefRecord      FDefs;     /* table of FDefs entries  */
  
    Int             numIDefs;  /* number of instruction defs */
    PDefRecord      IDefs;     /* table of IDefs entries     */
    Int		    countIDefs;/* The number of defined IDefs (igorm). */
    Byte	    IDefPtr[256]; /* Map opcodes to indices of IDefs (igorm). */

    PByte           glyphIns;  /* glyph instructions buffer */
    Int             glyphSize; /* glyph instructions buffer size */
  
    Int             callTop,    /* top of call stack during execution */
                    callSize;   /* size of call stack */
    PCallStack      callStack;  /* call stack */
  
    TCodeRangeTable codeRangeTable;  /* table of valid coderanges */
                                     /* useful for the debugger   */
  
    Int             storeSize;  /* size of current storage */
    PStorage        storage;    /* storage area            */
  
    Int             stackSize;  /* size of exec. stack */
    Int             top;        /* top of exec. stack  */
    PStorage        stack;      /* current exec. stack */
  
    Int             args,
                    new_top;    /* new top after exec.    */
  
    TT_F26Dot6      period;     /* values used for the */
    TT_F26Dot6      phase;      /* 'SuperRounding'     */
    TT_F26Dot6      threshold;

    TIns_Metrics    metrics;       /* instance metrics */

    Int             cur_ppem;       /* ppem along the current proj vector */
    Long            scale1;         /* scaling values along the current   */
    Long            scale2;         /* projection vector too..            */
    Bool            cached_metrics; /* the ppem is computed lazily. used  */
                                    /* to trigger computation when needed */

    TGlyph_Zone     zp0,            /* zone records */
                    zp1,
                    zp2,
                    pts,
                    twilight;
  
    Bool            instruction_trap;  /* If True, the interpreter will */
                                       /* exit after each instruction   */
 
    TGraphicsState  GS;            /* current graphics state */
                                   
    TGraphicsState  default_GS;    /* graphics state resulting from  */
                                   /* the prep program               */
    Bool            is_composite;  /* ture if the glyph is composite */

    Int             cvtSize;
    PLong           cvt;
  
    /* latest interpreter additions */
  
    Long               F_dot_P;    /* dot product of freedom and projection */
                                   /* vectors                               */
    TRound_Function    func_round; /* current rounding function             */
    
    TProject_Function  func_project,   /* current projection function */
                       func_dualproj,  /* current dual proj. function */
                       func_freeProj;  /* current freedom proj. func  */
 
    TMove_Function     func_move;      /* current point move function */

    TGet_CVT_Function  func_read_cvt;  /* read a cvt entry              */
    TSet_CVT_Function  func_write_cvt; /* write a cvt entry (in pixels) */
    TSet_CVT_Function  func_move_cvt;  /* incr a cvt entry (in pixels)  */
    /* GS extension */
    jmp_buf            trap;           /* Error throw trap. */ 
    Int                n_contours;
    Int                n_points;
    Int                maxGlyphSize;
    Int                lock;
  };


  /********************************************************************/
  /*                                                                  */
  /*   Code Range Functions                                           */
  /*                                                                  */
  /********************************************************************/
 
  /* Goto a specified coderange */
  TT_Error  Goto_CodeRange( PExecution_Context  exec, Int  range, Int  IP );
  /* Unset the coderange */
  void  Unset_CodeRange( PExecution_Context  exec );
 
  /* Return a pointer to a given coderange record. */
  /* Used only by the debugger.                    */
  PCodeRange  Get_CodeRange( PExecution_Context  exec, Int  range );
 
  /* Set a given code range properties */
  TT_Error  Set_CodeRange( PExecution_Context  exec,
                           Int                 range,
                           void*               base,
                           Int                 length );
 
  /* Clear a given coderange */
  TT_Error  Clear_CodeRange( PExecution_Context  exec, Int  range );


  PExecution_Context  New_Context( PFace  face );

  TT_Error  Done_Context( PExecution_Context  exec );


  TT_Error  Context_Load( PExecution_Context  exec,
                          PInstance           ins );

  TT_Error  Context_Save( PExecution_Context  exec,
                          PInstance           ins );

  TT_Error  Context_Run( PExecution_Context  exec,
                         Bool                debug );

  TT_Error  Instance_Init( PInstance  ins );

  TT_Error  Instance_Reset( PInstance  ins,
                            Bool       debug );

  TT_Error  Instance_Create( void*  _instance,
                             void*  _face );
 
  TT_Error  Instance_Destroy( void* _instance );

  TT_Error  Context_Destroy( void*  _context );

  TT_Error  Context_Create( void*  _context, void*  _face );

  /********************************************************************/
  /*                                                                  */
  /*   Handy scaling functions                                        */
  /*                                                                  */
  /********************************************************************/

  TT_Pos   Scale_X( PIns_Metrics  metrics, TT_Pos  x );
  TT_Pos   Scale_Y( PIns_Metrics  metrics, TT_Pos  y );

  /********************************************************************/
  /*                                                                  */
  /*   Component Initializer/Finalizer                                */
  /*                                                                  */
  /*   Called from 'freetype.c'                                       */
  /*   The component must create and register the face, instance and  */
  /*   execution context cache classes before any object can be       */
  /*   managed.                                                       */
  /*                                                                  */
  /********************************************************************/

TT_Error  Face_Create( PFace  _face);
TT_Error  Face_Destroy( PFace  _face);

#ifdef __cplusplus
  }
#endif

#endif /* TTOBJS_H */


/* END */
