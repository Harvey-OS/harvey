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

/* $Id: ttinterp.c,v 1.18 2005/08/02 11:12:32 igor Exp $ */

/* Changes after FreeType: cut out the TrueType instruction interpreter. */
/* Patented algorithms are replaced with THROW_PATENTED. */

/*******************************************************************
 *
 *  ttinterp.c                                              2.3
 *
 *  TrueType bytecode intepreter.
 *
 *  Copyright 1996-1998 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT.  By continuing to use, modify, or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 *
 *  TODO:
 *
 *  - Fix the non-square pixel case (or how to manage the CVT to
 *    detect horizontal and vertical scaled FUnits ?)
 *
 *
 *  Changes between 2.3 and 2.2:
 *
 *  - added support for rotation, stretching, instruction control
 *
 *  - added support for non-square pixels.  However, this doesn't
 *    work perfectly yet...
 *
 *  Changes between 2.2 and 2.1:
 *
 *  - a small bugfix in the Push opcodes
 *
 *  Changes between 2.1 and 2.0:
 *
 *  - created the TTExec component to take care of all execution
 *    context management.  The interpreter has now one single
 *    function.
 *
 *  - made some changes to support re-entrancy.  The re-entrant
 *    interpreter is smaller!
 *
 ******************************************************************/

#include "ttmisc.h"

#include "ttfoutl.h"
#include "tttypes.h"
#include "ttcalc.h"
#include "ttinterp.h"
#include "ttfinp.h"


#ifdef DEBUG
#  define DBG_PAINT    CUR.current_face->font->DebugRepaint(CUR.current_face->font);

#  define DBG_PRT_FUN CUR.current_face->font->DebugPrint
#  define DBG_PRT (void)(!DBG_PRT_FUN ? 0 : DBG_PRT_FUN(CUR.current_face->font
#  define DBG_PRINT(fmt) DBG_PRT, fmt))
#  define DBG_PRINT1(fmt, a) DBG_PRT, fmt, a))
#  define DBG_PRINT3(fmt, a, b, c) DBG_PRT, fmt, a, b, c))
#  define DBG_PRINT4(fmt, a, b, c, d) DBG_PRT, fmt, a, b, c, d))
#else
#  define DBG_PRT_FUN NULL
#  define DBG_PAINT
#  define DBG_PRINT(fmt)
#  define DBG_PRINT1(fmt, a)
#  define DBG_PRINT3(fmt, a, b, c)
#  define DBG_PRINT4(fmt, a, b, c, d)
#endif

static int nInstrCount=0;


/* There are two kinds of implementations there:              */
/*                                                            */
/* a. static implementation:                                  */
/*                                                            */
/*    The current execution context is a static variable,     */
/*    which fields are accessed directly by the interpreter   */
/*    during execution.  The context is named 'cur'.          */
/*                                                            */
/*    This version is non-reentrant, of course.               */
/*                                                            */
/*                                                            */
/* b. indirect implementation:                                */
/*                                                            */
/*    The current execution context is passed to _each_       */
/*    function as its first argument, and each field is       */
/*    thus accessed indirectly.                               */
/*                                                            */
/*    This version is, however, fully re-entrant.             */
/*                                                            */
/*                                                            */
/*  The idea is that an indirect implementation may be        */
/*  slower to execute on the low-end processors that are      */
/*  used in some systems (like 386s or even 486s).            */
/*                                                            */
/*  When the interpreter started, we had no idea of the       */
/*  time that glyph hinting (i.e. executing instructions)     */
/*  could take in the whole process of rendering a glyph,     */
/*  and a 10 to 30% performance penalty on low-end systems    */
/*  didn't seem much of a good idea.  This question led us    */
/*  to provide two distinct builds of the C version from      */
/*  a single source, with the use of macros (again).          */
/*                                                            */
/*  Now that the engine is working (and working really        */
/*  well!), it seems that the greatest time-consuming         */
/*  factors are: file i/o, glyph loading, rasterizing and     */
/*  _then_ glyph hinting!                                     */
/*                                                            */
/*  Tests performed with two versions of the 'fttimer'        */
/*  program seem to indicate that hinting takes less than 5%  */
/*  of the rendering process, which is dominated by glyph     */
/*  loading and scan-line conversion by an high order of      */
/*  magnitude.                                                */
/*                                                            */
/*  As a consequence, the indirect implementation is now the  */
/*  default, as its performance costs can be considered       */
/*  negligible in our context. Note, however, that we         */
/*  kept the same source with macros because:                 */
/*                                                            */
/*    - the code is kept very close in design to the          */
/*      Pascal one used for development.                      */
/*                                                            */
/*    - it's much more readable that way!                     */
/*                                                            */
/*    - it's still open to later experimentation and tuning   */


#ifndef TT_STATIC_INTERPRETER      /* indirect implementation */

#define CUR (*exc)                 /* see ttobjs.h */

#else                              /* static implementation */

#define CUR cur

  static TExecution_Context  cur;  /* static exec. context variable */

  /* apparently, we have a _lot_ of direct indexing when accessing  */
  /* the static 'cur', which makes the code bigger (due to all the  */
  /* four bytes addresses).                                         */

#endif


#define INS_ARG         EXEC_OPS PStorage args  /* see ttexec.h */

#define SKIP_Code()     SkipCode( EXEC_ARG )

#define GET_ShortIns()  GetShortIns( EXEC_ARG )

#define COMPUTE_Funcs() Compute_Funcs( EXEC_ARG )

#define NORMalize( x, y, v )  Normalize( EXEC_ARGS x, y, v )

#define SET_SuperRound( scale, flags ) \
                        SetSuperRound( EXEC_ARGS scale, flags )

#define INS_Goto_CodeRange( range, ip ) \
                        Ins_Goto_CodeRange( EXEC_ARGS range, ip )

#define CUR_Func_project( x, y )   CUR.func_project( EXEC_ARGS x, y )
#define CUR_Func_move( z, p, d )   CUR.func_move( EXEC_ARGS z, p, d )
#define CUR_Func_dualproj( x, y )  CUR.func_dualproj( EXEC_ARGS x, y )
#define CUR_Func_freeProj( x, y )  CUR.func_freeProj( EXEC_ARGS x, y )
#define CUR_Func_round( d, c )     CUR.func_round( EXEC_ARGS d, c )

#define CUR_Func_read_cvt( index )       \
          CUR.func_read_cvt( EXEC_ARGS index )

#define CUR_Func_write_cvt( index, val ) \
          CUR.func_write_cvt( EXEC_ARGS index, val )

#define CUR_Func_move_cvt( index, val )  \
          CUR.func_move_cvt( EXEC_ARGS index, val )

#define CURRENT_Ratio()  Current_Ratio( EXEC_ARG )
#define CURRENT_Ppem()   Current_Ppem( EXEC_ARG )

#define CALC_Length()  Calc_Length( EXEC_ARG )

#define INS_SxVTL( a, b, c, d ) Ins_SxVTL( EXEC_ARGS a, b, c, d )

#define COMPUTE_Point_Displacement( a, b, c, d ) \
           Compute_Point_Displacement( EXEC_ARGS a, b, c, d )

#define MOVE_Zp2_Point( a, b, c, t )  Move_Zp2_Point( EXEC_ARGS a, b, c, t )

#define CUR_Ppem()  Cur_PPEM( EXEC_ARG )

  /* Instruction dispatch function, as used by the interpreter */
  typedef void  (*TInstruction_Function)( INS_ARG );

#define BOUNDS(x,n)  ( x < 0 || x >= n )

#ifndef ABS
#define ABS(x)  ( (x) < 0 ? -(x) : (x) )
#endif

/* The following macro is used to disable algorithms,
   which could cause Apple's patent infringement. */
#define THROW_PATENTED longjmp(CUR.trap, TT_Err_Invalid_Engine)



/*********************************************************************/
/*                                                                   */
/*  Before an opcode is executed, the interpreter verifies that      */
/*  there are enough arguments on the stack, with the help of        */
/*  the Pop_Push_Count table.                                        */
/*                                                                   */
/*  For each opcode, the first column gives the number of arguments  */
/*  that are popped from the stack; the second one gives the number  */
/*  of those that are pushed in result.                              */
/*                                                                   */
/*  Note that for opcodes with a varying number of parameters,       */
/*  either 0 or 1 arg is verified before execution, depending        */
/*  on the nature of the instruction:                                */
/*                                                                   */
/*   - if the number of arguments is given by the bytecode           */
/*     stream or the loop variable, 0 is chosen.                     */
/*                                                                   */
/*   - if the first argument is a count n that is followed           */
/*     by arguments a1..an, then 1 is chosen.                        */
/*                                                                   */
/*********************************************************************/

  static unsigned char Pop_Push_Count[512] =
  {
    /* opcodes are gathered in groups of 16 */
    /* please keep the spaces as they are   */

    /*  SVTCA  y  */  0, 0,
    /*  SVTCA  x  */  0, 0,
    /*  SPvTCA y  */  0, 0,
    /*  SPvTCA x  */  0, 0,
    /*  SFvTCA y  */  0, 0,
    /*  SFvTCA x  */  0, 0,
    /*  SPvTL //  */  2, 0,
    /*  SPvTL +   */  2, 0,
    /*  SFvTL //  */  2, 0,
    /*  SFvTL +   */  2, 0,
    /*  SPvFS     */  2, 0,
    /*  SFvFS     */  2, 0,
    /*  GPV       */  0, 2,
    /*  GFV       */  0, 2,
    /*  SFvTPv    */  0, 0,
    /*  ISECT     */  5, 0,

    /*  SRP0      */  1, 0,
    /*  SRP1      */  1, 0,
    /*  SRP2      */  1, 0,
    /*  SZP0      */  1, 0,
    /*  SZP1      */  1, 0,
    /*  SZP2      */  1, 0,
    /*  SZPS      */  1, 0,
    /*  SLOOP     */  1, 0,
    /*  RTG       */  0, 0,
    /*  RTHG      */  0, 0,
    /*  SMD       */  1, 0,
    /*  ELSE      */  0, 0,
    /*  JMPR      */  1, 0,
    /*  SCvTCi    */  1, 0,
    /*  SSwCi     */  1, 0,
    /*  SSW       */  1, 0,

    /*  DUP       */  1, 2,
    /*  POP       */  1, 0,
    /*  CLEAR     */  0, 0,
    /*  SWAP      */  2, 2,
    /*  DEPTH     */  0, 1,
    /*  CINDEX    */  1, 1,
    /*  MINDEX    */  1, 0,
    /*  AlignPTS  */  2, 0,
    /*  INS_$28   */  0, 0,
    /*  UTP       */  1, 0,
    /*  LOOPCALL  */  2, 0,
    /*  CALL      */  1, 0,
    /*  FDEF      */  1, 0,
    /*  ENDF      */  0, 0,
    /*  MDAP[0]   */  1, 0,
    /*  MDAP[1]   */  1, 0,

    /*  IUP[0]    */  0, 0,
    /*  IUP[1]    */  0, 0,
    /*  SHP[0]    */  0, 0,
    /*  SHP[1]    */  0, 0,
    /*  SHC[0]    */  1, 0,
    /*  SHC[1]    */  1, 0,
    /*  SHZ[0]    */  1, 0,
    /*  SHZ[1]    */  1, 0,
    /*  SHPIX     */  1, 0,
    /*  IP        */  0, 0,
    /*  MSIRP[0]  */  2, 0,
    /*  MSIRP[1]  */  2, 0,
    /*  AlignRP   */  0, 0,
    /*  RTDG      */  0, 0,
    /*  MIAP[0]   */  2, 0,
    /*  MIAP[1]   */  2, 0,

    /*  NPushB    */  0, 0,
    /*  NPushW    */  0, 0,
    /*  WS        */  2, 0,
    /*  RS        */  1, 1,
    /*  WCvtP     */  2, 0,
    /*  RCvt      */  1, 1,
    /*  GC[0]     */  1, 1,
    /*  GC[1]     */  1, 1,
    /*  SCFS      */  2, 0,
    /*  MD[0]     */  2, 1,
    /*  MD[1]     */  2, 1,
    /*  MPPEM     */  0, 1,
    /*  MPS       */  0, 1,
    /*  FlipON    */  0, 0,
    /*  FlipOFF   */  0, 0,
    /*  DEBUG     */  1, 0,

    /*  LT        */  2, 1,
    /*  LTEQ      */  2, 1,
    /*  GT        */  2, 1,
    /*  GTEQ      */  2, 1,
    /*  EQ        */  2, 1,
    /*  NEQ       */  2, 1,
    /*  ODD       */  1, 1,
    /*  EVEN      */  1, 1,
    /*  IF        */  1, 0,
    /*  EIF       */  0, 0,
    /*  AND       */  2, 1,
    /*  OR        */  2, 1,
    /*  NOT       */  1, 1,
    /*  DeltaP1   */  1, 0,
    /*  SDB       */  1, 0,
    /*  SDS       */  1, 0,

    /*  ADD       */  2, 1,
    /*  SUB       */  2, 1,
    /*  DIV       */  2, 1,
    /*  MUL       */  2, 1,
    /*  ABS       */  1, 1,
    /*  NEG       */  1, 1,
    /*  FLOOR     */  1, 1,
    /*  CEILING   */  1, 1,
    /*  ROUND[0]  */  1, 1,
    /*  ROUND[1]  */  1, 1,
    /*  ROUND[2]  */  1, 1,
    /*  ROUND[3]  */  1, 1,
    /*  NROUND[0] */  1, 1,
    /*  NROUND[1] */  1, 1,
    /*  NROUND[2] */  1, 1,
    /*  NROUND[3] */  1, 1,

    /*  WCvtF     */  2, 0,
    /*  DeltaP2   */  1, 0,
    /*  DeltaP3   */  1, 0,
    /*  DeltaCn[0] */ 1, 0,
    /*  DeltaCn[1] */ 1, 0,
    /*  DeltaCn[2] */ 1, 0,
    /*  SROUND    */  1, 0,
    /*  S45Round  */  1, 0,
    /*  JROT      */  2, 0,
    /*  JROF      */  2, 0,
    /*  ROFF      */  0, 0,
    /*  INS_$7B   */  0, 0,
    /*  RUTG      */  0, 0,
    /*  RDTG      */  0, 0,
    /*  SANGW     */  1, 0,
    /*  AA        */  1, 0,

    /*  FlipPT    */  0, 0,
    /*  FlipRgON  */  2, 0,
    /*  FlipRgOFF */  2, 0,
    /*  INS_$83   */  0, 0,
    /*  INS_$84   */  0, 0,
    /*  ScanCTRL  */  1, 0,
    /*  SDVPTL[0] */  2, 0,
    /*  SDVPTL[1] */  2, 0,
    /*  GetINFO   */  1, 1,
    /*  IDEF      */  1, 0,
    /*  ROLL      */  3, 3,
    /*  MAX       */  2, 1,
    /*  MIN       */  2, 1,
    /*  ScanTYPE  */  1, 0,
    /*  InstCTRL  */  2, 0,
    /*  INS_$8F   */  0, 0,

    /*  INS_$90  */   0, 0,
    /*  INS_$91  */   0, 0,
    /*  INS_$92  */   0, 0,
    /*  INS_$93  */   0, 0,
    /*  INS_$94  */   0, 0,
    /*  INS_$95  */   0, 0,
    /*  INS_$96  */   0, 0,
    /*  INS_$97  */   0, 0,
    /*  INS_$98  */   0, 0,
    /*  INS_$99  */   0, 0,
    /*  INS_$9A  */   0, 0,
    /*  INS_$9B  */   0, 0,
    /*  INS_$9C  */   0, 0,
    /*  INS_$9D  */   0, 0,
    /*  INS_$9E  */   0, 0,
    /*  INS_$9F  */   0, 0,

    /*  INS_$A0  */   0, 0,
    /*  INS_$A1  */   0, 0,
    /*  INS_$A2  */   0, 0,
    /*  INS_$A3  */   0, 0,
    /*  INS_$A4  */   0, 0,
    /*  INS_$A5  */   0, 0,
    /*  INS_$A6  */   0, 0,
    /*  INS_$A7  */   0, 0,
    /*  INS_$A8  */   0, 0,
    /*  INS_$A9  */   0, 0,
    /*  INS_$AA  */   0, 0,
    /*  INS_$AB  */   0, 0,
    /*  INS_$AC  */   0, 0,
    /*  INS_$AD  */   0, 0,
    /*  INS_$AE  */   0, 0,
    /*  INS_$AF  */   0, 0,

    /*  PushB[0]  */  0, 1,
    /*  PushB[1]  */  0, 2,
    /*  PushB[2]  */  0, 3,
    /*  PushB[3]  */  0, 4,
    /*  PushB[4]  */  0, 5,
    /*  PushB[5]  */  0, 6,
    /*  PushB[6]  */  0, 7,
    /*  PushB[7]  */  0, 8,
    /*  PushW[0]  */  0, 1,
    /*  PushW[1]  */  0, 2,
    /*  PushW[2]  */  0, 3,
    /*  PushW[3]  */  0, 4,
    /*  PushW[4]  */  0, 5,
    /*  PushW[5]  */  0, 6,
    /*  PushW[6]  */  0, 7,
    /*  PushW[7]  */  0, 8,

    /*  MDRP[00]  */  1, 0,
    /*  MDRP[01]  */  1, 0,
    /*  MDRP[02]  */  1, 0,
    /*  MDRP[03]  */  1, 0,
    /*  MDRP[04]  */  1, 0,
    /*  MDRP[05]  */  1, 0,
    /*  MDRP[06]  */  1, 0,
    /*  MDRP[07]  */  1, 0,
    /*  MDRP[08]  */  1, 0,
    /*  MDRP[09]  */  1, 0,
    /*  MDRP[10]  */  1, 0,
    /*  MDRP[11]  */  1, 0,
    /*  MDRP[12]  */  1, 0,
    /*  MDRP[13]  */  1, 0,
    /*  MDRP[14]  */  1, 0,
    /*  MDRP[15]  */  1, 0,

    /*  MDRP[16]  */  1, 0,
    /*  MDRP[17]  */  1, 0,
    /*  MDRP[18]  */  1, 0,
    /*  MDRP[19]  */  1, 0,
    /*  MDRP[20]  */  1, 0,
    /*  MDRP[21]  */  1, 0,
    /*  MDRP[22]  */  1, 0,
    /*  MDRP[23]  */  1, 0,
    /*  MDRP[24]  */  1, 0,
    /*  MDRP[25]  */  1, 0,
    /*  MDRP[26]  */  1, 0,
    /*  MDRP[27]  */  1, 0,
    /*  MDRP[28]  */  1, 0,
    /*  MDRP[29]  */  1, 0,
    /*  MDRP[30]  */  1, 0,
    /*  MDRP[31]  */  1, 0,

    /*  MIRP[00]  */  2, 0,
    /*  MIRP[01]  */  2, 0,
    /*  MIRP[02]  */  2, 0,
    /*  MIRP[03]  */  2, 0,
    /*  MIRP[04]  */  2, 0,
    /*  MIRP[05]  */  2, 0,
    /*  MIRP[06]  */  2, 0,
    /*  MIRP[07]  */  2, 0,
    /*  MIRP[08]  */  2, 0,
    /*  MIRP[09]  */  2, 0,
    /*  MIRP[10]  */  2, 0,
    /*  MIRP[11]  */  2, 0,
    /*  MIRP[12]  */  2, 0,
    /*  MIRP[13]  */  2, 0,
    /*  MIRP[14]  */  2, 0,
    /*  MIRP[15]  */  2, 0,

    /*  MIRP[16]  */  2, 0,
    /*  MIRP[17]  */  2, 0,
    /*  MIRP[18]  */  2, 0,
    /*  MIRP[19]  */  2, 0,
    /*  MIRP[20]  */  2, 0,
    /*  MIRP[21]  */  2, 0,
    /*  MIRP[22]  */  2, 0,
    /*  MIRP[23]  */  2, 0,
    /*  MIRP[24]  */  2, 0,
    /*  MIRP[25]  */  2, 0,
    /*  MIRP[26]  */  2, 0,
    /*  MIRP[27]  */  2, 0,
    /*  MIRP[28]  */  2, 0,
    /*  MIRP[29]  */  2, 0,
    /*  MIRP[30]  */  2, 0,
    /*  MIRP[31]  */  2, 0
  };


/*******************************************************************
 *
 *  Function    :  Norm
 *
 *  Description :  Returns the norm (length) of a vector.
 *
 *  Input  :  X, Y   vector
 *
 *  Output :  Returns length in F26dot6.
 *
 *****************************************************************/

  static TT_F26Dot6  Norm( TT_F26Dot6  X, TT_F26Dot6  Y )
  {
    Int64       T1, T2;


    MUL_64( X, X, T1 );
    MUL_64( Y, Y, T2 );

    ADD_64( T1, T2, T1 );

    return (TT_F26Dot6)SQRT_64( T1 );
  }


/*******************************************************************
 *
 *  Function    :  FUnits_To_Pixels
 *
 *  Description :  Scale a distance in FUnits to pixel coordinates.
 *
 *  Input  :  Distance in FUnits
 *
 *  Output :  Distance in 26.6 format.
 *
 *****************************************************************/

  static TT_F26Dot6  FUnits_To_Pixels( EXEC_OPS  Int  distance )
  {
    return MulDiv_Round( distance,
                         CUR.metrics.scale1,
                         CUR.metrics.scale2 );
  }


/*******************************************************************
 *
 *  Function    :  Current_Ratio
 *
 *  Description :  Return the current aspect ratio scaling factor
 *                 depending on the projection vector's state and
 *                 device resolutions.
 *
 *  Input  :  None
 *
 *  Output :  Aspect ratio in 16.16 format, always <= 1.0 .
 *
 *****************************************************************/

  static Long  Current_Ratio( EXEC_OP )
  {
    if ( CUR.metrics.ratio )
      return CUR.metrics.ratio;

    if ( CUR.GS.projVector.y == 0 )
      CUR.metrics.ratio = CUR.metrics.x_ratio;

    else if ( CUR.GS.projVector.x == 0 )
      CUR.metrics.ratio = CUR.metrics.y_ratio;

    else
    {
      Long  x, y;


      x = MulDiv_Round( CUR.GS.projVector.x, CUR.metrics.x_ratio, 0x4000 );
      y = MulDiv_Round( CUR.GS.projVector.y, CUR.metrics.y_ratio, 0x4000 );
      CUR.metrics.ratio = Norm( x, y );
    }

    return CUR.metrics.ratio;
  }


  static Int  Current_Ppem( EXEC_OP )
  {
    return MulDiv_Round( CUR.metrics.ppem, CURRENT_Ratio(), 0x10000 );
  }


  static TT_F26Dot6  Read_CVT( EXEC_OPS Int  index )
  {
    return CUR.cvt[index];
  }


  static TT_F26Dot6  Read_CVT_Stretched( EXEC_OPS Int  index )
  {
    return MulDiv_Round( CUR.cvt[index], CURRENT_Ratio(), 0x10000 );
  }


  static void  Write_CVT( EXEC_OPS Int  index, TT_F26Dot6  value )
  {
    int ov=CUR.cvt[index];
    CUR.cvt[index] = value;
    DBG_PRINT3(" cvt[%d]%d:=%d", index, ov, CUR.cvt[index]);
}

  static void  Write_CVT_Stretched( EXEC_OPS Int  index, TT_F26Dot6  value )
  {
    int ov=CUR.cvt[index];
    CUR.cvt[index] = MulDiv_Round( value, 0x10000, CURRENT_Ratio() );
    DBG_PRINT3(" cvt[%d]%d:=%d", index, ov, CUR.cvt[index]);
  }


  static void  Move_CVT( EXEC_OPS  Int index, TT_F26Dot6 value )
  {
    int ov=CUR.cvt[index];
    CUR.cvt[index] += value;
    DBG_PRINT3(" cvt[%d]%d:=%d", index, ov, CUR.cvt[index]);
  }

  static void  Move_CVT_Stretched( EXEC_OPS  Int index, TT_F26Dot6  value )
  {
    int ov=CUR.cvt[index];
    CUR.cvt[index] += MulDiv_Round( value, 0x10000, CURRENT_Ratio() );
    DBG_PRINT3(" cvt[%d]%d:=%d", index, ov, CUR.cvt[index]);
  }


/******************************************************************
 *
 *  Function    :  Calc_Length
 *
 *  Description :  Computes the length in bytes of current opcode.
 *
 *****************************************************************/

  static Bool  Calc_Length( EXEC_OP )
  {
    CUR.opcode = CUR.code[CUR.IP];

    switch ( CUR.opcode )
    {
    case 0x40:
      if ( CUR.IP + 1 >= CUR.codeSize )
        return FAILURE;

      CUR.length = CUR.code[CUR.IP + 1] + 2;
      break;

    case 0x41:
      if ( CUR.IP + 1 >= CUR.codeSize )
        return FAILURE;

      CUR.length = CUR.code[CUR.IP + 1] * 2 + 2;
      break;

    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
    case 0xB4:
    case 0xB5:
    case 0xB6:
    case 0xB7:
      CUR.length = CUR.opcode - 0xB0 + 2;
      break;

    case 0xB8:
    case 0xB9:
    case 0xBA:
    case 0xBB:
    case 0xBC:
    case 0xBD:
    case 0xBE:
    case 0xBF:
      CUR.length = (CUR.opcode - 0xB8) * 2 + 3;
      break;

    default:
      CUR.length = 1;
      break;
    }

    /* make sure result is in range */

    if ( CUR.IP + CUR.length > CUR.codeSize )
      return FAILURE;

    return SUCCESS;
  }


/*******************************************************************
 *
 *  Function    :  GetShortIns
 *
 *  Description :  Returns a short integer taken from the instruction
 *                 stream at address IP.
 *
 *  Input  :  None
 *
 *  Output :  Short read at Code^[IP..IP+1]
 *
 *  Notes  :  This one could become a Macro in the C version.
 *
 *****************************************************************/

  static Short  GetShortIns( EXEC_OP )
  {
    /* Reading a byte stream so there is no endianess (DaveP) */
    CUR.IP += 2;
    return ( CUR.code[CUR.IP-2] << 8) +
             CUR.code[CUR.IP-1];
  }


/*******************************************************************
 *
 *  Function    :  Ins_Goto_CodeRange
 *
 *  Description :  Goes to a certain code range in the instruction
 *                 stream.
 *
 *
 *  Input  :  aRange
 *            aIP
 *
 *  Output :  SUCCESS or FAILURE.
 *
 *****************************************************************/

  static Bool  Ins_Goto_CodeRange( EXEC_OPS Int  aRange, Int  aIP )
  {
    TCodeRange*  WITH;


    if ( aRange < 1 || aRange > 3 )
    {
      CUR.error = TT_Err_Bad_Argument;
      return FAILURE;
    }

    WITH = &CUR.codeRangeTable[aRange - 1];

    if ( WITH->Base == NULL )     /* invalid coderange */
    {
      CUR.error = TT_Err_Invalid_CodeRange;
      return FAILURE;
    }

    /* NOTE: Because the last instruction of a program may be a CALL */
    /*       which will return to the first byte *after* the code    */
    /*       range, we test for AIP <= Size, instead of AIP < Size.  */

    if ( aIP > WITH->Size )
    {
      CUR.error = TT_Err_Code_Overflow;
      return FAILURE;
    }

    CUR.code     = WITH->Base;
    CUR.codeSize = WITH->Size;
    CUR.IP       = aIP;
    CUR.curRange = aRange;

    return SUCCESS;
  }


/*******************************************************************
 *
 *  Function    :  Direct_Move
 *
 *  Description :  Moves a point by a given distance along the
 *                 freedom vector.
 *
 *  Input  : Vx, Vy      point coordinates to move
 *           touch       touch flag to modify
 *           distance
 *
 *  Output :  None
 *
 *****************************************************************/

  static void  Direct_Move( EXEC_OPS PGlyph_Zone zone,
                                     Int         point,
                                     TT_F26Dot6  distance )
  {
    TT_F26Dot6 v;


    v = CUR.GS.freeVector.x;

    if ( v != 0 )
    {
      zone->cur_x[point] += MulDiv_Round( distance,
                                          v * 0x10000L,
                                          CUR.F_dot_P );

      zone->touch[point] |= TT_Flag_Touched_X;
    }

    v = CUR.GS.freeVector.y;

    if ( v != 0 )
    {
      zone->cur_y[point] += MulDiv_Round( distance,
                                          v * 0x10000L,
                                          CUR.F_dot_P );

      zone->touch[point] |= TT_Flag_Touched_Y;
    }
  }


/******************************************************************/
/*                                                                */
/* The following versions are used whenever both vectors are both */
/* along one of the coordinate unit vectors, i.e. in 90% cases.   */
/*                                                                */
/******************************************************************/

/*******************************************************************
 * Direct_Move_X
 *
 *******************************************************************/

  static void  Direct_Move_X( EXEC_OPS PGlyph_Zone  zone,
                                       Int         point,
                                       TT_F26Dot6  distance )
  { (void)exc;
    zone->cur_x[point] += distance;
    zone->touch[point] |= TT_Flag_Touched_X;
  }


/*******************************************************************
 * Direct_Move_Y
 *
 *******************************************************************/

  static void  Direct_Move_Y( EXEC_OPS PGlyph_Zone  zone,
                                       Int         point,
                                       TT_F26Dot6  distance )
  { (void)exc;
    zone->cur_y[point] += distance;
    zone->touch[point] |= TT_Flag_Touched_Y;
  }


/*******************************************************************
 *
 *  Function    :  Round_None
 *
 *  Description :  Does not round, but adds engine compensation.
 *
 *  Input  :  distance      : distance to round
 *            compensation  : engine compensation
 *
 *  Output :  rounded distance.
 *
 *  NOTE : The spec says very few about the relationship between
 *         rounding and engine compensation.  However, it seems
 *         from the description of super round that we should
 *         should add the compensation before rounding.
 *
 ******************************************************************/

  static TT_F26Dot6  Round_None( EXEC_OPS TT_F26Dot6  distance,
                                          TT_F26Dot6  compensation )
  {
    TT_F26Dot6  val;
    (void)exc;

    if ( distance >= 0 )
    {
      val = distance + compensation;
      if ( val < 0 )
        val = 0;
    }
    else {
      val = distance - compensation;
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


/*******************************************************************
 *
 *  Function    :  Round_To_Grid
 *
 *  Description :  Rounds value to grid after adding engine
 *                 compensation
 *
 *  Input  :  distance      : distance to round
 *            compensation  : engine compensation
 *
 *  Output :  Rounded distance.
 *
 *****************************************************************/

  static TT_F26Dot6  Round_To_Grid( EXEC_OPS TT_F26Dot6  distance,
                                             TT_F26Dot6  compensation )
  {
    TT_F26Dot6  val;
    (void)exc;

    if ( distance >= 0 )
    {
      val = (distance + compensation + 32) & (-64);
      if ( val < 0 )
        val = 0;
    }
    else
    {
      val = -( (compensation - distance + 32) & (-64) );
      if ( val > 0 )
        val = 0;
    }


    return  val;
  }


/*******************************************************************
 *
 *  Function    :  Round_To_Half_Grid
 *
 *  Description :  Rounds value to half grid after adding engine
 *                 compensation.
 *
 *  Input  :  distance      : distance to round
 *            compensation  : engine compensation
 *
 *  Output :  Rounded distance.
 *
 *****************************************************************/

  static TT_F26Dot6  Round_To_Half_Grid( EXEC_OPS TT_F26Dot6  distance,
                                                  TT_F26Dot6  compensation )
  {
    TT_F26Dot6  val;
     (void)exc;

    if ( distance >= 0 )
    {
      val = ((distance + compensation) & (-64)) + 32;
      if ( val < 0 )
        val = 0;
    }
    else
    {
      val = -( ((compensation - distance) & (-64)) + 32 );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


/*******************************************************************
 *
 *  Function    :  Round_Down_To_Grid
 *
 *  Description :  Rounds value down to grid after adding engine
 *                 compensation.
 *
 *  Input  :  distance      : distance to round
 *            compensation  : engine compensation
 *
 *  Output :  Rounded distance.
 *
 *****************************************************************/

  static TT_F26Dot6  Round_Down_To_Grid( EXEC_OPS TT_F26Dot6  distance,
                                                  TT_F26Dot6  compensation )
  {
    TT_F26Dot6  val;
    (void)exc;

    if ( distance >= 0 )
    {
      val = (distance + compensation) & (-64);
      if ( val < 0 )
        val = 0;
    }
    else
    {
      val = -( (compensation - distance) & (-64) );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


/*******************************************************************
 *
 *  Function    :  Round_Up_To_Grid
 *
 *  Description :  Rounds value up to grid after adding engine
 *                 compensation.
 *
 *  Input  :  distance      : distance to round
 *            compensation  : engine compensation
 *
 *  Output :  Rounded distance.
 *
 *****************************************************************/

  static TT_F26Dot6  Round_Up_To_Grid( EXEC_OPS TT_F26Dot6  distance,
                                                TT_F26Dot6  compensation )
  {
    TT_F26Dot6  val;
    (void)exc;

    if ( distance >= 0 )
    {
      val = (distance + compensation + 63) & (-64);
      if ( val < 0 )
        val = 0;
    }
    else
    {
      val = -( (compensation - distance + 63) & (-64) );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


/*******************************************************************
 *
 *  Function    :  Round_To_Double_Grid
 *
 *  Description :  Rounds value to double grid after adding engine
 *                 compensation.
 *
 *  Input  :  distance      : distance to round
 *            compensation  : engine compensation
 *
 *  Output :  Rounded distance.
 *
 *****************************************************************/

  static TT_F26Dot6  Round_To_Double_Grid( EXEC_OPS TT_F26Dot6  distance,
                                                    TT_F26Dot6  compensation )
  {
    TT_F26Dot6 val;
    (void)exc;

    if ( distance >= 0 )
    {
      val = (distance + compensation + 16) & (-32);
      if ( val < 0 )
        val = 0;
    }
    else
    {
      val = -( (compensation - distance + 16) & (-32) );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


/*******************************************************************
 *
 *  Function    :  Round_Super
 *
 *  Description :  Super-rounds value to grid after adding engine
 *                 compensation.
 *
 *  Input  :  distance      : distance to round
 *            compensation  : engine compensation
 *
 *  Output :  Rounded distance.
 *
 *  NOTE : The spec says very few about the relationship between
 *         rounding and engine compensation.  However, it seems
 *         from the description of super round that we should
 *         should add the compensation before rounding.
 *
 *****************************************************************/

  static TT_F26Dot6  Round_Super( EXEC_OPS TT_F26Dot6  distance,
                                           TT_F26Dot6  compensation )
  {
    TT_F26Dot6  val;


    if ( distance >= 0 )
    {
      val = (distance - CUR.phase + CUR.threshold + compensation) &
              (-CUR.period);
      if ( val < 0 )
        val = 0;
      val += CUR.phase;
    }
    else
    {
      val = -( (CUR.threshold - CUR.phase - distance + compensation) &
               (-CUR.period) );
      if ( val > 0 )
        val = 0;
      val -= CUR.phase;
    }

    return val;
  }


/*******************************************************************
 *
 *  Function    :  Round_Super_45
 *
 *  Description :  Super-rounds value to grid after adding engine
 *                 compensation.
 *
 *  Input  :  distance      : distance to round
 *            compensation  : engine compensation
 *
 *  Output :  Rounded distance.
 *
 *  NOTE : There is a separate function for Round_Super_45 as we
 *         may need a greater precision.
 *
 *****************************************************************/

  static TT_F26Dot6  Round_Super_45( EXEC_OPS TT_F26Dot6  distance,
                                              TT_F26Dot6  compensation )
  {
    TT_F26Dot6  val;


    if ( distance >= 0 )
    {
      val = ( (distance - CUR.phase + CUR.threshold + compensation) /
                CUR.period ) * CUR.period;
      if ( val < 0 )
        val = 0;
      val += CUR.phase;
    }
    else
    {
      val = -( ( (CUR.threshold - CUR.phase - distance + compensation) /
                   CUR.period ) * CUR.period );
      if ( val > 0 )
        val = 0;
      val -= CUR.phase;
    }

    return val;
  }


/*******************************************************************
 * Compute_Round
 *
 *****************************************************************/

  static void  Compute_Round( EXEC_OPS Byte  round_mode )
  {
    switch ( round_mode )
    {
    case TT_Round_Off:
      CUR.func_round = (TRound_Function)Round_None;
      break;

    case TT_Round_To_Grid:
      CUR.func_round = (TRound_Function)Round_To_Grid;
      break;

    case TT_Round_Up_To_Grid:
      CUR.func_round = (TRound_Function)Round_Up_To_Grid;
      break;

    case TT_Round_Down_To_Grid:
      CUR.func_round = (TRound_Function)Round_Down_To_Grid;
      break;

    case TT_Round_To_Half_Grid:
      CUR.func_round = (TRound_Function)Round_To_Half_Grid;
      break;

    case TT_Round_To_Double_Grid:
      CUR.func_round = (TRound_Function)Round_To_Double_Grid;
      break;

    case TT_Round_Super:
      CUR.func_round = (TRound_Function)Round_Super;
      break;

    case TT_Round_Super_45:
      CUR.func_round = (TRound_Function)Round_Super_45;
      break;
    }
  }


/*******************************************************************
 *
 *  Function    :  SetSuperRound
 *
 *  Description :  Sets Super Round parameters.
 *
 *  Input  :  GridPeriod   Grid period
 *            OpCode       SROUND opcode
 *
 *  Output :  None.
 *
 *****************************************************************/

  static void  SetSuperRound( EXEC_OPS TT_F26Dot6  GridPeriod,
                                       Long        selector )
  {
    switch ( selector & 0xC0 )
    {
      case 0:
        CUR.period = GridPeriod / 2;
        break;

      case 0x40:
        CUR.period = GridPeriod;
        break;

      case 0x80:
        CUR.period = GridPeriod * 2;
        break;

      /* This opcode is reserved, but... */

      case 0xC0:
        CUR.period = GridPeriod;
        break;
    }

    switch ( selector & 0x30 )
    {
    case 0:
      CUR.phase = 0;
      break;

    case 0x10:
      CUR.phase = CUR.period / 4;
      break;

    case 0x20:
      CUR.phase = CUR.period / 2;
      break;

    case 0x30:
      CUR.phase = GridPeriod * 3 / 4;
      break;
    }

    if ( (selector & 0x0F) == 0 )
      CUR.threshold = CUR.period - 1;
    else
      CUR.threshold = ( (Int)(selector & 0x0F) - 4L ) * CUR.period / 8;

    CUR.period    /= 256;
    CUR.phase     /= 256;
    CUR.threshold /= 256;
  }
/*******************************************************************
 *
 *  Function    :  Project
 *
 *  Description :  Computes the projection of (Vx,Vy) along the
 *                 current projection vector.
 *
 *  Input  :  Vx, Vy    input vector
 *
 *  Output :  Returns distance in F26dot6.
 *
 *****************************************************************/

  static TT_F26Dot6  Project( EXEC_OPS TT_F26Dot6  Vx, TT_F26Dot6  Vy )
  {
    THROW_PATENTED;
    return 0;
  }


/*******************************************************************
 *
 *  Function    :  Dual_Project
 *
 *  Description :  Computes the projection of (Vx,Vy) along the
 *                 current dual vector.
 *
 *  Input  :  Vx, Vy    input vector
 *
 *  Output :  Returns distance in F26dot6.
 *
 *****************************************************************/

  static TT_F26Dot6  Dual_Project( EXEC_OPS TT_F26Dot6  Vx, TT_F26Dot6  Vy )
  {
    THROW_PATENTED;
    return 0;
  }


/*******************************************************************
 *
 *  Function    :  Free_Project
 *
 *  Description :  Computes the projection of (Vx,Vy) along the
 *                 current freedom vector.
 *
 *  Input  :  Vx, Vy    input vector
 *
 *  Output :  Returns distance in F26dot6.
 *
 *****************************************************************/

  static TT_F26Dot6  Free_Project( EXEC_OPS TT_F26Dot6  Vx, TT_F26Dot6  Vy )
  {
    THROW_PATENTED;
    return 0;
  }



/*******************************************************************
 *
 *  Function    :  Project_x
 *
 *  Input  :  Vx, Vy    input vector
 *
 *  Output :  Returns Vx.
 *
 *  Note :    Used as a dummy function.
 *
 *****************************************************************/

  static TT_F26Dot6  Project_x( EXEC_OPS TT_F26Dot6  Vx, TT_F26Dot6  Vy )
  { (void)exc; (void)Vy;
    return Vx;
  }


/*******************************************************************
 *
 *  Function    :  Project_y
 *
 *  Input  :  Vx, Vy    input vector
 *
 *  Output :  Returns Vy.
 *
 *  Note :    Used as a dummy function.
 *
 *****************************************************************/

  static TT_F26Dot6  Project_y( EXEC_OPS TT_F26Dot6  Vx, TT_F26Dot6  Vy )
  { (void)exc; (void)Vx;
    return Vy;
  }


/*******************************************************************
 *
 *  Function    :  Compute_Funcs
 *
 *  Description :  Computes the projections and movement function
 *                 pointers according to the current graphics state.
 *
 *  Input  :  None
 *
 *****************************************************************/

  static void  Compute_Funcs( EXEC_OP )
  {
    if ( CUR.GS.freeVector.x == 0x4000 )
    {
      CUR.func_freeProj = (TProject_Function)Project_x;
      CUR.F_dot_P       = CUR.GS.projVector.x * 0x10000L;
    }
    else
    {
      if ( CUR.GS.freeVector.y == 0x4000 )
      {
        CUR.func_freeProj = (TProject_Function)Project_y;
        CUR.F_dot_P       = CUR.GS.projVector.y * 0x10000L;
      }
      else
      {
        CUR.func_move     = (TMove_Function)Direct_Move;
        CUR.func_freeProj = (TProject_Function)Free_Project;
        CUR.F_dot_P = (Long)CUR.GS.projVector.x * CUR.GS.freeVector.x * 4 +
                      (Long)CUR.GS.projVector.y * CUR.GS.freeVector.y * 4;
      }
    }

    CUR.cached_metrics = FALSE;

    if ( CUR.GS.projVector.x == 0x4000 )
      CUR.func_project = (TProject_Function)Project_x;
    else
    {
      if ( CUR.GS.projVector.y == 0x4000 )
        CUR.func_project = (TProject_Function)Project_y;
      else
        CUR.func_project = (TProject_Function)Project;
    }

    if ( CUR.GS.dualVector.x == 0x4000 )
      CUR.func_dualproj = (TProject_Function)Project_x;
    else
    {
      if ( CUR.GS.dualVector.y == 0x4000 )
        CUR.func_dualproj = (TProject_Function)Project_y;
      else
        CUR.func_dualproj = (TProject_Function)Dual_Project;
    }

    CUR.func_move = (TMove_Function)Direct_Move;

    if ( CUR.F_dot_P == 0x40000000L )
    {
      if ( CUR.GS.freeVector.x == 0x4000 )
        CUR.func_move = (TMove_Function)Direct_Move_X;
      else
      {
        if ( CUR.GS.freeVector.y == 0x4000 )
          CUR.func_move = (TMove_Function)Direct_Move_Y;
      }
    }

    /* at small sizes, F_dot_P can become too small, resulting   */
    /* in overflows and 'spikes' in a number of glyphs like 'w'. */

    if ( ABS( CUR.F_dot_P ) < 0x4000000L )
      CUR.F_dot_P = 0x40000000L;

    /* Disable cached aspect ratio */
    CUR.metrics.ratio = 0;
  }

/*******************************************************************
 *
 *  Function    :  Normalize
 *
 *  Description :  Norms a vector
 *
 *  Input  :  Vx, Vy    input vector
 *            R         unit vector
 *
 *  Output :  Returns FAILURE if a vector parameter is zero.
 *
 *****************************************************************/

  static Bool  Normalize( EXEC_OPS TT_F26Dot6      Vx,
                                   TT_F26Dot6      Vy,
                                   TT_UnitVector*  R )
  {
    TT_F26Dot6  W;
    Bool        S1, S2;


    if ( ABS( Vx ) < 0x10000L && ABS( Vy ) < 0x10000L )
    {
      Vx *= 0x100;
      Vy *= 0x100;

      W = Norm( Vx, Vy );

      if ( W == 0 )
      {
        /* XXX : Undocumented. It seems that it's possible to try  */
        /*       to normalize the vector (0,0). Return immediately */
        return SUCCESS;
      }

      R->x = (TT_F2Dot14)MulDiv_Round( Vx, 0x4000L, W );
      R->y = (TT_F2Dot14)MulDiv_Round( Vy, 0x4000L, W );

      return SUCCESS;
    }

    W = Norm( Vx, Vy );

    if ( W <= 0 )
    {
      CUR.error = TT_Err_Divide_By_Zero;
      return FAILURE;
    }

    Vx = MulDiv_Round( Vx, 0x4000L, W );
    Vy = MulDiv_Round( Vy, 0x4000L, W );

    W = Vx * Vx + Vy * Vy;

    /* Now, we want that Sqrt( W ) = 0x4000 */
    /* Or 0x10000000 <= W < 0x10004000      */

    if ( Vx < 0 )
    {
      Vx = -Vx;
      S1 = TRUE;
    }
    else
      S1 = FALSE;

    if ( Vy < 0 )
    {
      Vy = -Vy;
      S2 = TRUE;
    }
    else
      S2 = FALSE;

    while ( W < 0x10000000L )
    {
      /* We need to increase W, by a minimal amount */
      if ( Vx < Vy )
        Vx++;
      else
        Vy++;

      W = Vx * Vx + Vy * Vy;
    }

    while ( W >= 0x10004000L )
    {
      /* We need to decrease W, by a minimal amount */
      if ( Vx < Vy )
        Vx--;
      else
        Vy--;

      W = Vx * Vx + Vy * Vy;
    }

    /* Note that in various cases, we can only  */
    /* compute a Sqrt(W) of 0x3FFF, eg. Vx = Vy */

    if ( S1 )
      Vx = -Vx;

    if ( S2 )
      Vy = -Vy;

    R->x = (TT_F2Dot14)Vx;   /* Type conversion */
    R->y = (TT_F2Dot14)Vy;   /* Type conversion */

    return SUCCESS;
  }



/****************************************************************/
/*                                                              */
/* MANAGING THE STACK                                           */
/*                                                              */
/*  Instructions appear in the specs' order.                    */
/*                                                              */
/****************************************************************/

/*******************************************/
/* DUP[]     : Duplicate top stack element */
/* CodeRange : $20                         */

  static void  Ins_DUP( INS_ARG )
  { (void)exc;
    args[1] = args[0];
  }


/*******************************************/
/* POP[]     : POPs the stack's top elt.   */
/* CodeRange : $21                         */

  static void  Ins_POP( INS_ARG )
  { (void)exc; (void)args;
    /* nothing to do */
  }


/*******************************************/
/* CLEAR[]   : Clear the entire stack      */
/* CodeRange : $22                         */

  static void  Ins_CLEAR( INS_ARG )
  { (void)args;
    CUR.new_top = 0;
  }


/*******************************************/
/* SWAP[]    : Swap the top two elements   */
/* CodeRange : $23                         */

  static void  Ins_SWAP( INS_ARG )
  {
    Long  L;
    (void)exc;

    L       = args[0];
    args[0] = args[1];
    args[1] = L;
  }


/*******************************************/
/* DEPTH[]   : return the stack depth      */
/* CodeRange : $24                         */

  static void  Ins_DEPTH( INS_ARG )
  {
    args[0] = CUR.top;
  }


/*******************************************/
/* CINDEX[]  : copy indexed element        */
/* CodeRange : $25                         */

  static void  Ins_CINDEX( INS_ARG )
  {
    Long  L;


    L = args[0];

    if ( L<0 || L > CUR.args )
      CUR.error = TT_Err_Invalid_Reference;
    else
      args[0] = CUR.stack[CUR.args - L];
  }


/*******************************************/
/* MINDEX[]  : move indexed element        */
/* CodeRange : $26                         */

  static void  Ins_MINDEX( INS_ARG )
  {
    Long  L, K;


    L = args[0];

    if ( L<0 || L > CUR.args )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    K = CUR.stack[CUR.args - L];

    memmove( (&CUR.stack[CUR.args - L    ]),
              (&CUR.stack[CUR.args - L + 1]),
              (L - 1) * sizeof ( Long ) );

    CUR.stack[ CUR.args-1 ] = K;
  }


/*******************************************/
/* ROLL[]    : roll top three elements     */
/* CodeRange : $8A                         */

  static void  Ins_ROLL( INS_ARG )
  {
    Long  A, B, C;
    (void)exc;

    A = args[2];
    B = args[1];
    C = args[0];

    args[2] = C;
    args[1] = A;
    args[0] = B;
  }



/****************************************************************/
/*                                                              */
/* MANAGING THE FLOW OF CONTROL                                 */
/*                                                              */
/*  Instructions appear in the specs' order.                    */
/*                                                              */
/****************************************************************/

  static Bool  SkipCode( EXEC_OP )
  {
    CUR.IP += CUR.length;

    if ( CUR.IP < CUR.codeSize )
      if ( CALC_Length() == SUCCESS )
        return SUCCESS;

    CUR.error = TT_Err_Code_Overflow;
    return FAILURE;
  }


/*******************************************/
/* IF[]      : IF test                     */
/* CodeRange : $58                         */

  static void  Ins_IF( INS_ARG )
  {
    Int   nIfs;
    Bool  Out;


    if ( args[0] != 0 )
      return;

    nIfs = 1;
    Out = 0;

    do
    {
      if ( SKIP_Code() == FAILURE )
        return;

      switch ( CUR.opcode )
      {
      case 0x58:      /* IF */
        nIfs++;
        break;

      case 0x1b:      /* ELSE */
        Out = (nIfs == 1);
        break;

      case 0x59:      /* EIF */
        nIfs--;
        Out = (nIfs == 0);
        break;
      }
    } while ( Out == 0 );
  }


/*******************************************/
/* ELSE[]    : ELSE                        */
/* CodeRange : $1B                         */

  static void  Ins_ELSE( INS_ARG )
  {
    Int  nIfs;
    (void)args;

    nIfs = 1;

    do
    {
      if ( SKIP_Code() == FAILURE )
        return;

      switch ( CUR.opcode )
      {
      case 0x58:    /* IF */
        nIfs++;
        break;

      case 0x59:    /* EIF */
        nIfs--;
        break;
      }
    } while ( nIfs != 0 );
  }


/*******************************************/
/* EIF[]     : End IF                      */
/* CodeRange : $59                         */

  static void  Ins_EIF( INS_ARG )
  { (void)exc; (void)args;
    /* nothing to do */
  }


/*******************************************/
/* JROT[]    : Jump Relative On True       */
/* CodeRange : $78                         */

  static void  Ins_JROT( INS_ARG )
  {
    if ( args[1] != 0 )
    {
      CUR.IP      += (Int)(args[0]);
      CUR.step_ins = FALSE;
    }
  }


/*******************************************/
/* JMPR[]    : JuMP Relative               */
/* CodeRange : $1C                         */

  static void  Ins_JMPR( INS_ARG )
  {
    CUR.IP      += (Int)(args[0]);
    CUR.step_ins = FALSE;
  }


/*******************************************/
/* JROF[]    : Jump Relative On False      */
/* CodeRange : $79                         */

  static void  Ins_JROF( INS_ARG )
  {
    if ( args[1] == 0 )
    {
      CUR.IP      += (Int)(args[0]);
      CUR.step_ins = FALSE;
    }
  }



/****************************************************************/
/*                                                              */
/* LOGICAL FUNCTIONS                                            */
/*                                                              */
/*  Instructions appear in the specs' order.                    */
/*                                                              */
/****************************************************************/

/*******************************************/
/* LT[]      : Less Than                   */
/* CodeRange : $50                         */

  static void  Ins_LT( INS_ARG )
  { (void)exc;
    if ( args[0] < args[1] )
      args[0] = 1;
    else
      args[0] = 0;
  }


/*******************************************/
/* LTEQ[]    : Less Than or EQual          */
/* CodeRange : $51                         */

  static void  Ins_LTEQ( INS_ARG )
  { (void)exc;
    if ( args[0] <= args[1] )
      args[0] = 1;
    else
      args[0] = 0;
  }


/*******************************************/
/* GT[]      : Greater Than                */
/* CodeRange : $52                         */

  static void  Ins_GT( INS_ARG )
  { (void)exc;
    if ( args[0] > args[1] )
      args[0] = 1;
    else
      args[0] = 0;
  }


/*******************************************/
/* GTEQ[]    : Greater Than or EQual       */
/* CodeRange : $53                         */

  static void  Ins_GTEQ( INS_ARG )
  { (void)exc;
    if ( args[0] >= args[1] )
      args[0] = 1;
    else
      args[0] = 0;
  }


/*******************************************/
/* EQ[]      : EQual                       */
/* CodeRange : $54                         */

  static void  Ins_EQ( INS_ARG )
  { (void)exc;
    if ( args[0] == args[1] )
      args[0] = 1;
    else
      args[0] = 0;
  }


/*******************************************/
/* NEQ[]     : Not EQual                   */
/* CodeRange : $55                         */

  static void  Ins_NEQ( INS_ARG )
  { (void)exc;
    if ( args[0] != args[1] )
      args[0] = 1;
    else
      args[0] = 0;
  }


/*******************************************/
/* ODD[]     : Odd                         */
/* CodeRange : $56                         */

  static void  Ins_ODD( INS_ARG )
  {
    if ( (CUR_Func_round( args[0], 0L ) & 127) == 64 )
      args[0] = 1;
    else
      args[0] = 0;
  }


/*******************************************/
/* EVEN[]    : Even                        */
/* CodeRange : $57                         */

  static void  Ins_EVEN( INS_ARG )
  {
    if ( (CUR_Func_round( args[0], 0L ) & 127) == 0 )
      args[0] = 1;
    else
      args[0] = 0;
  }


/*******************************************/
/* AND[]     : logical AND                 */
/* CodeRange : $5A                         */

  static void  Ins_AND( INS_ARG )
  { (void)exc;
    if ( args[0] != 0 && args[1] != 0 )
      args[0] = 1;
    else
      args[0] = 0;
  }


/*******************************************/
/* OR[]      : logical OR                  */
/* CodeRange : $5B                         */

  static void  Ins_OR( INS_ARG )
  { (void)exc;
    if ( args[0] != 0 || args[1] != 0 )
      args[0] = 1;
    else
      args[0] = 0;
  }


/*******************************************/
/* NOT[]     : logical NOT                 */
/* CodeRange : $5C                         */

  static void  Ins_NOT( INS_ARG )
  { (void)exc;
    if ( args[0] != 0 )
      args[0] = 0;
    else
      args[0] = 1;
  }



/****************************************************************/
/*                                                              */
/* ARITHMETIC AND MATH INSTRUCTIONS                             */
/*                                                              */
/*  Instructions appear in the specs' order.                    */
/*                                                              */
/****************************************************************/

/*******************************************/
/* ADD[]     : ADD                         */
/* CodeRange : $60                         */

  static void  Ins_ADD( INS_ARG )
  { (void)exc;
    args[0] += args[1];
  }


/*******************************************/
/* SUB[]     : SUBstract                   */
/* CodeRange : $61                         */

  static void  Ins_SUB( INS_ARG )
  { (void)exc;
    args[0] -= args[1];
  }


/*******************************************/
/* DIV[]     : DIVide                      */
/* CodeRange : $62                         */

  static void  Ins_DIV( INS_ARG )
  {
    if ( args[1] == 0 )
    {
      CUR.error = TT_Err_Divide_By_Zero;
      return;
    }

    args[0] = MulDiv_Round( args[0], 64L, args[1] );
    DBG_PRINT1(" %d", args[0]);
  }


/*******************************************/
/* MUL[]     : MULtiply                    */
/* CodeRange : $63                         */

  static void  Ins_MUL( INS_ARG )
  { (void)exc;
    args[0] = MulDiv_Round( args[0], args[1], 64L );
  }


/*******************************************/
/* ABS[]     : ABSolute value              */
/* CodeRange : $64                         */

  static void  Ins_ABS( INS_ARG )
  { (void)exc;
    args[0] = ABS( args[0] );
  }


/*******************************************/
/* NEG[]     : NEGate                      */
/* CodeRange : $65                         */

  static void  Ins_NEG( INS_ARG )
  { (void)exc;
    args[0] = -args[0];
  }


/*******************************************/
/* FLOOR[]   : FLOOR                       */
/* CodeRange : $66                         */

  static void  Ins_FLOOR( INS_ARG )
  { (void)exc;
    args[0] &= -64;
  }


/*******************************************/
/* CEILING[] : CEILING                     */
/* CodeRange : $67                         */

  static void  Ins_CEILING( INS_ARG )
  { (void)exc;
    args[0] = (args[0] + 63) & (-64);
  }


/*******************************************/
/* MAX[]     : MAXimum                     */
/* CodeRange : $68                         */

  static void  Ins_MAX( INS_ARG )
  { (void)exc;
    if ( args[1] > args[0] )
      args[0] = args[1];
  }


/*******************************************/
/* MIN[]     : MINimum                     */
/* CodeRange : $69                         */

  static void  Ins_MIN( INS_ARG )
  { (void)exc;
    if ( args[1] < args[0] )
      args[0] = args[1];
  }



/****************************************************************/
/*                                                              */
/* COMPENSATING FOR THE ENGINE CHARACTERISTICS                  */
/*                                                              */
/*  Instructions appear in the specs' order.                    */
/*                                                              */
/****************************************************************/

/*******************************************/
/* ROUND[ab] : ROUND value                 */
/* CodeRange : $68-$6B                     */

  static void  Ins_ROUND( INS_ARG )
  {
    args[0] = CUR_Func_round( args[0],
                              CUR.metrics.compensations[CUR.opcode - 0x68] );
  }


/*******************************************/
/* NROUND[ab]: No ROUNDing of value        */
/* CodeRange : $6C-$6F                     */

  static void  Ins_NROUND( INS_ARG )
  {
    args[0] = Round_None( EXEC_ARGS
                          args[0],
                          CUR.metrics.compensations[CUR.opcode - 0x6C] );
  }



/****************************************************************/
/*                                                              */
/* DEFINING AND USING FUNCTIONS AND INSTRUCTIONS                */
/*                                                              */
/*  Instructions appear in the specs' order.                    */
/*                                                              */
/****************************************************************/

/* Skip the whole function definition. */
  static void skip_FDEF( EXEC_OP )
  {
    /* We don't allow nested IDEFS & FDEFs.    */

    while ( SKIP_Code() == SUCCESS )
    {
      switch ( CUR.opcode )
      {
      case 0x89:    /* IDEF */
      case 0x2c:    /* FDEF */
        CUR.error = TT_Err_Nested_DEFS;
        return;

      case 0x2d:   /* ENDF */
        return;
      }
    }
  }

/*******************************************/
/* FDEF[]    : Function DEFinition         */
/* CodeRange : $2C                         */

  static void  Ins_FDEF( INS_ARG )
  {
    PDefRecord  pRec;


    if ( BOUNDS( args[0], CUR.numFDefs ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    pRec = &CUR.FDefs[args[0]];

    pRec->Range  = CUR.curRange;
    pRec->Opc    = (Byte)(args[0]);
    pRec->Start  = CUR.IP + 1;
    pRec->Active = TRUE;

    skip_FDEF(EXEC_ARG);
  }


/*******************************************/
/* ENDF[]    : END Function definition     */
/* CodeRange : $2D                         */

  static void  Ins_ENDF( INS_ARG )
  {
    PCallRecord  pRec;
     (void)args;

    if ( CUR.callTop <= 0 )     /* We encountered an ENDF without a call */
    {
      CUR.error = TT_Err_ENDF_In_Exec_Stream;
      return;
    }

    CUR.callTop--;

    pRec = &CUR.callStack[CUR.callTop];

    pRec->Cur_Count--;

    CUR.step_ins = FALSE;

    if ( pRec->Cur_Count > 0 )
    {
      CUR.callTop++;
      CUR.IP = pRec->Cur_Restart;
    }
    else
      /* Loop through the current function */
      INS_Goto_CodeRange( pRec->Caller_Range,
                          pRec->Caller_IP );

    /* Exit the current call frame.                       */

    /* NOTE: When the last intruction of a program        */
    /*       is a CALL or LOOPCALL, the return address    */
    /*       is always out of the code range.  This is    */
    /*       a valid address, and it's why we do not test */
    /*       the result of Ins_Goto_CodeRange() here!     */
  }


/*******************************************/
/* CALL[]    : CALL function               */
/* CodeRange : $2B                         */

  static void  Ins_CALL( INS_ARG )
  {
    PCallRecord  pCrec;


    if ( BOUNDS( args[0], CUR.numFDefs ) || !CUR.FDefs[args[0]].Active )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( CUR.callTop >= CUR.callSize )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    DBG_PRINT1("%d", args[0]);

    pCrec = &CUR.callStack[CUR.callTop];

    pCrec->Caller_Range = CUR.curRange;
    pCrec->Caller_IP    = CUR.IP + 1;
    pCrec->Cur_Count    = 1;
    pCrec->Cur_Restart  = CUR.FDefs[args[0]].Start;

    CUR.callTop++;

    INS_Goto_CodeRange( CUR.FDefs[args[0]].Range,
                        CUR.FDefs[args[0]].Start );

    CUR.step_ins = FALSE;
  }


/*******************************************/
/* LOOPCALL[]: LOOP and CALL function      */
/* CodeRange : $2A                         */

  static void  Ins_LOOPCALL( INS_ARG )
  {
    PCallRecord  pTCR;

    if ( BOUNDS( args[1], CUR.numFDefs ) || !CUR.FDefs[args[1]].Active )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( CUR.callTop >= CUR.callSize )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    if ( args[0] > 0 )
    {
      pTCR = &CUR.callStack[CUR.callTop];

      pTCR->Caller_Range = CUR.curRange;
      pTCR->Caller_IP    = CUR.IP + 1;
      pTCR->Cur_Count    = (Int)(args[0]);
      pTCR->Cur_Restart  = CUR.FDefs[args[1]].Start;

      CUR.callTop++;

      INS_Goto_CodeRange( CUR.FDefs[args[1]].Range,
                          CUR.FDefs[args[1]].Start );

      CUR.step_ins = FALSE;
    }
  }


/*******************************************/
/* IDEF[]    : Instruction DEFinition      */
/* CodeRange : $89                         */

  static void Ins_IDEF( INS_ARG )
  {
    if (CUR.countIDefs >= CUR.numIDefs || args[0] > 255)
	CUR.error = TT_Err_Storage_Overflow;
    else 
      {
	PDefRecord  pTDR;

	CUR.IDefPtr[(Byte)(args[0])] = CUR.countIDefs;
	pTDR = &CUR.IDefs[CUR.countIDefs++];
        pTDR->Opc    = (Byte)(args[0]);
        pTDR->Start  = CUR.IP + 1;
        pTDR->Range  = CUR.curRange;
        pTDR->Active = TRUE;
	skip_FDEF(EXEC_ARG);
      }
  }



/****************************************************************/
/*                                                              */
/* PUSHING DATA ONTO THE INTERPRETER STACK                      */
/*                                                              */
/*  Instructions appear in the specs' order.                    */
/*                                                              */
/****************************************************************/

/*******************************************/
/* NPUSHB[]  : PUSH N Bytes                */
/* CodeRange : $40                         */

  static void  Ins_NPUSHB( INS_ARG )
  {
    Int  L, K;

    L = (Int)CUR.code[CUR.IP + 1];

    if ( BOUNDS( L, CUR.stackSize+1-CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    for ( K = 1; K <= L; K++ )
      { args[K - 1] = CUR.code[CUR.IP + K + 1];
        DBG_PRINT1(" %d", args[K - 1]);
      }

    CUR.new_top += L;
  }


/*******************************************/
/* NPUSHW[]  : PUSH N Words                */
/* CodeRange : $41                         */

  static void  Ins_NPUSHW( INS_ARG )
  {
    Int  L, K;


    L = (Int)CUR.code[CUR.IP + 1];

    if ( BOUNDS( L, CUR.stackSize+1-CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    CUR.IP += 2;

    for ( K = 0; K < L; K++ )
      { args[K] = GET_ShortIns();
        DBG_PRINT1(" %d", args[K]);
      }

    CUR.step_ins = FALSE;
    CUR.new_top += L;
  }


/*******************************************/
/* PUSHB[abc]: PUSH Bytes                  */
/* CodeRange : $B0-$B7                     */

  static void  Ins_PUSHB( INS_ARG )
  {
    Int  L, K;

    L = ((Int)CUR.opcode - 0xB0 + 1);

    if ( BOUNDS( L, CUR.stackSize+1-CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    for ( K = 1; K <= L; K++ )
      { args[K - 1] = CUR.code[CUR.IP + K];
        DBG_PRINT1(" %d", args[K - 1]);
      }
  }


/*******************************************/
/* PUSHW[abc]: PUSH Words                  */
/* CodeRange : $B8-$BF                     */

  static void  Ins_PUSHW( INS_ARG )
  {
    Int  L, K;


    L = CUR.opcode - 0xB8 + 1;

    if ( BOUNDS( L, CUR.stackSize+1-CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    CUR.IP++;

    for ( K = 0; K < L; K++ )
      { args[K] = GET_ShortIns();
        DBG_PRINT1(" %d", args[K]);
      }

    CUR.step_ins = FALSE;
  }



/****************************************************************/
/*                                                              */
/* MANAGING THE STORAGE AREA                                    */
/*                                                              */
/*  Instructions appear in the specs' order.                    */
/*                                                              */
/****************************************************************/

/*******************************************/
/* RS[]      : Read Store                  */
/* CodeRange : $43                         */

  static void  Ins_RS( INS_ARG )
  {
    if ( BOUNDS( args[0], CUR.storeSize ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    args[0] = CUR.storage[args[0]];
  }


/*******************************************/
/* WS[]      : Write Store                 */
/* CodeRange : $42                         */

  static void  Ins_WS( INS_ARG )
  {
    if ( BOUNDS( args[0], CUR.storeSize ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.storage[args[0]] = args[1];
  }


/*******************************************/
/* WCVTP[]   : Write CVT in Pixel units    */
/* CodeRange : $44                         */

  static void  Ins_WCVTP( INS_ARG )
  {
    if ( BOUNDS( args[0], CUR.cvtSize ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR_Func_write_cvt( args[0], args[1] );
  }


/*******************************************/
/* WCVTF[]   : Write CVT in FUnits         */
/* CodeRange : $70                         */

  static void  Ins_WCVTF( INS_ARG )
  {
    int ov;

    if ( BOUNDS( args[0], CUR.cvtSize ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    ov = CUR.cvt[args[0]];
    CUR.cvt[args[0]] = FUnits_To_Pixels( EXEC_ARGS args[1] );
    DBG_PRINT3(" cvt[%d]%d:=%d", args[0], ov, CUR.cvt[args[0]]);
  }


/*******************************************/
/* RCVT[]    : Read CVT                    */
/* CodeRange : $45                         */

  static void  Ins_RCVT( INS_ARG )
  {
    int index;
    if ( BOUNDS( args[0], CUR.cvtSize ) )
    {
#if 0
      CUR.error = TT_Err_Invalid_Reference;
      return;
#else
      /* A workaround for the Ghostscript Bug 687604. 
         Ported from FreeType 2 : !FT_LOAD_PEDANTIC by default. */
      index=args[0];
      args[0] = 0;
      DBG_PRINT1(" cvt[%d] stubbed with 0", index);
#endif
    }
    index=args[0];
    args[0] = CUR_Func_read_cvt( index );
    DBG_PRINT3(" cvt[%d]%d:%d", index, CUR.cvt[index], args[0]);
  }



/****************************************************************/
/*                                                              */
/* MANAGING THE GRAPHICS STATE                                  */
/*                                                              */
/*  Instructions appear in the specs' order.                    */
/*                                                              */
/****************************************************************/

/*******************************************/
/* SVTCA[a]  : Set F and P vectors to axis */
/* CodeRange : $00-$01                     */

  static void  Ins_SVTCA( INS_ARG )
  {
    Short  A, B;
    (void)args;

    if ( CUR.opcode & 1 )
        A = 0x4000;
    else
        A = 0;

    B = A ^ 0x4000;

    CUR.GS.freeVector.x = A;
    CUR.GS.projVector.x = A;
    CUR.GS.dualVector.x = A;

    CUR.GS.freeVector.y = B;
    CUR.GS.projVector.y = B;
    CUR.GS.dualVector.y = B;

    COMPUTE_Funcs();
  }


/*******************************************/
/* SPVTCA[a] : Set PVector to Axis         */
/* CodeRange : $02-$03                     */

  static void  Ins_SPVTCA( INS_ARG )
  {
    Short  A, B;
    (void)args;
    if ( CUR.opcode & 1 )
      A = 0x4000;
    else
      A = 0;

    B = A ^ 0x4000;

    CUR.GS.projVector.x = A;
    CUR.GS.dualVector.x = A;

    CUR.GS.projVector.y = B;
    CUR.GS.dualVector.y = B;

    COMPUTE_Funcs();
  }


/*******************************************/
/* SFVTCA[a] : Set FVector to Axis         */
/* CodeRange : $04-$05                     */

  static void  Ins_SFVTCA( INS_ARG )
  {
    Short  A, B;
    (void)args;

    if ( CUR.opcode & 1 )
      A = 0x4000;
    else
      A = 0;

    B = A ^ 0x4000;

    CUR.GS.freeVector.x = A;
    CUR.GS.freeVector.y = B;

    COMPUTE_Funcs();
  }


  static Bool  Ins_SxVTL( EXEC_OPS  Int             aIdx1,
                                    Int             aIdx2,
                                    Int             aOpc,
                                    TT_UnitVector*  Vec )
  {
    Long  A, B, C;

    if ( BOUNDS( aIdx1, CUR.zp2.n_points ) ||
         BOUNDS( aIdx2, CUR.zp1.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return FAILURE;
    }

    A = CUR.zp1.cur_x[aIdx2] - CUR.zp2.cur_x[aIdx1];
    B = CUR.zp1.cur_y[aIdx2] - CUR.zp2.cur_y[aIdx1];

    if ( (aOpc & 1) != 0 )
    {
      C =  B;   /* CounterClockwise rotation */
      B =  A;
      A = -C;
    }

    if ( NORMalize( A, B, Vec ) == FAILURE )
    {
      /* When the vector is too small or zero! */

      CUR.error = TT_Err_Ok;
      Vec->x = 0x4000;
      Vec->y = 0;
    }

    return SUCCESS;
  }


/*******************************************/
/* SPVTL[a]  : Set PVector to Line         */
/* CodeRange : $06-$07                     */

  static void  Ins_SPVTL( INS_ARG )
  {
    if ( INS_SxVTL( args[1],
                    args[0],
                    CUR.opcode,
                    &CUR.GS.projVector) == FAILURE )
      return;

    CUR.GS.dualVector = CUR.GS.projVector;
    COMPUTE_Funcs();
  }


/*******************************************/
/* SFVTL[a]  : Set FVector to Line         */
/* CodeRange : $08-$09                     */

  static void  Ins_SFVTL( INS_ARG )
  {
    if ( INS_SxVTL( (Int)(args[1]),
                    (Int)(args[0]),
                    CUR.opcode,
                    &CUR.GS.freeVector) == FAILURE )
      return;

    COMPUTE_Funcs();
  }


/*******************************************/
/* SFVTPV[]  : Set FVector to PVector      */
/* CodeRange : $0E                         */

  static void  Ins_SFVTPV( INS_ARG )
  { (void)args;
    CUR.GS.freeVector = CUR.GS.projVector;
    COMPUTE_Funcs();
  }


/*******************************************/
/* SDPVTL[a] : Set Dual PVector to Line    */
/* CodeRange : $86-$87                     */

  static void  Ins_SDPVTL( INS_ARG )
  {
    Long  A, B, C;
    Long  p1, p2;   /* was Int in pas type ERROR */


    p1 = args[1];
    p2 = args[0];

    if ( BOUNDS( p2, CUR.zp1.n_points ) ||
         BOUNDS( p1, CUR.zp2.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    A = CUR.zp1.org_x[p2] - CUR.zp2.org_x[p1];
    B = CUR.zp1.org_y[p2] - CUR.zp2.org_y[p1];

    if ( (CUR.opcode & 1) != 0 )
    {
      C =  B;   /* CounterClockwise rotation */
      B =  A;
      A = -C;
    }

    if ( NORMalize( A, B, &CUR.GS.dualVector ) == FAILURE )
      return;

    A = CUR.zp1.cur_x[p2] - CUR.zp2.cur_x[p1];
    B = CUR.zp1.cur_y[p2] - CUR.zp2.cur_y[p1];

    if ( (CUR.opcode & 1) != 0 )
    {
      C =  B;   /* CounterClockwise rotation */
      B =  A;
      A = -C;
    }

    if ( NORMalize( A, B, &CUR.GS.projVector ) == FAILURE )
      return;

    COMPUTE_Funcs();
  }


/*******************************************/
/* SPVFS[]   : Set PVector From Stack      */
/* CodeRange : $0A                         */

  static void  Ins_SPVFS( INS_ARG )
  {
    Short  S;
    Long   X, Y;


    /* Only use low 16bits, then sign extend */
    S = (Short)args[1];
    Y = (Long)S;
    S = (Short)args[0];
    X = (Long)S;

    if ( NORMalize( X, Y, &CUR.GS.projVector ) == FAILURE )
      return;

    CUR.GS.dualVector = CUR.GS.projVector;

    COMPUTE_Funcs();
  }


/*******************************************/
/* SFVFS[]   : Set FVector From Stack      */
/* CodeRange : $0B                         */

  static void  Ins_SFVFS( INS_ARG )
  {
    Short  S;
    Long   X, Y;


    /* Only use low 16bits, then sign extend */
    S = (Short)args[1];
    Y = (Long)S;
    S = (Short)args[0];
    X = S;

    if ( NORMalize( X, Y, &CUR.GS.freeVector ) == FAILURE )
      return;

    COMPUTE_Funcs();
  }


/*******************************************/
/* GPV[]     : Get Projection Vector       */
/* CodeRange : $0C                         */

  static void  Ins_GPV( INS_ARG )
  {
    args[0] = CUR.GS.projVector.x;
    args[1] = CUR.GS.projVector.y;
  }


/*******************************************/
/* GFV[]     : Get Freedom Vector          */
/* CodeRange : $0D                         */

  static void  Ins_GFV( INS_ARG )
  {
    args[0] = CUR.GS.freeVector.x;
    args[1] = CUR.GS.freeVector.y;
  }


/*******************************************/
/* SRP0[]    : Set Reference Point 0       */
/* CodeRange : $10                         */

  static void  Ins_SRP0( INS_ARG )
  {
    CUR.GS.rp0 = (Int)(args[0]);
  }


/*******************************************/
/* SRP1[]    : Set Reference Point 1       */
/* CodeRange : $11                         */

  static void  Ins_SRP1( INS_ARG )
  {
    CUR.GS.rp1 = (Int)(args[0]);
  }


/*******************************************/
/* SRP2[]    : Set Reference Point 2       */
/* CodeRange : $12                         */

  static void  Ins_SRP2( INS_ARG )
  {
    CUR.GS.rp2 = (Int)(args[0]);
  }


/*******************************************/
/* SZP0[]    : Set Zone Pointer 0          */
/* CodeRange : $13                         */

  static void  Ins_SZP0( INS_ARG )
  {
    switch ( args[0] )
    {
    case 0:
      CUR.zp0 = CUR.twilight;
      break;

    case 1:
      CUR.zp0 = CUR.pts;
      break;

    default:
      CUR.error = TT_Err_Invalid_Reference;
      return;
      break;
    }

    CUR.GS.gep0 = (Int)(args[0]);
  }


/*******************************************/
/* SZP1[]    : Set Zone Pointer 1          */
/* CodeRange : $14                         */

  static void  Ins_SZP1( INS_ARG )
  {
    switch ( args[0] )
    {
    case 0:
      CUR.zp1 = CUR.twilight;
      break;

    case 1:
      CUR.zp1 = CUR.pts;
      break;

    default:
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.GS.gep1 = (Int)(args[0]);
  }


/*******************************************/
/* SZP2[]    : Set Zone Pointer 2          */
/* CodeRange : $15                         */

  static void  Ins_SZP2( INS_ARG )
  {
    switch ( args[0] )
    {
    case 0:
      CUR.zp2 = CUR.twilight;
      break;

    case 1:
      CUR.zp2 = CUR.pts;
      break;

    default:
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.GS.gep2 = (Int)(args[0]);
  }


/*******************************************/
/* SZPS[]    : Set Zone Pointers           */
/* CodeRange : $16                         */

  static void  Ins_SZPS( INS_ARG )
  {
    switch ( args[0] )
    {
    case 0:
      CUR.zp0 = CUR.twilight;
      break;

    case 1:
      CUR.zp0 = CUR.pts;
      break;

    default:
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.zp1 = CUR.zp0;
    CUR.zp2 = CUR.zp0;

    CUR.GS.gep0 = (Int)(args[0]);
    CUR.GS.gep1 = (Int)(args[0]);
    CUR.GS.gep2 = (Int)(args[0]);
  }


/*******************************************/
/* RTHG[]    : Round To Half Grid          */
/* CodeRange : $19                         */

  static void  Ins_RTHG( INS_ARG )
  { (void)args;
    CUR.GS.round_state = TT_Round_To_Half_Grid;

    CUR.func_round = (TRound_Function)Round_To_Half_Grid;
  }


/*******************************************/
/* RTG[]     : Round To Grid               */
/* CodeRange : $18                         */

  static void  Ins_RTG( INS_ARG )
  { (void)args;
    CUR.GS.round_state = TT_Round_To_Grid;

    CUR.func_round = (TRound_Function)Round_To_Grid;
  }


/*******************************************/
/* RTDG[]    : Round To Double Grid        */
/* CodeRange : $3D                         */

  static void  Ins_RTDG( INS_ARG )
  { (void)args;
    CUR.GS.round_state = TT_Round_To_Double_Grid;

    CUR.func_round = (TRound_Function)Round_To_Double_Grid;
  }


/*******************************************/
/* RUTG[]    : Round Up To Grid            */
/* CodeRange : $7C                         */

  static void  Ins_RUTG( INS_ARG )
  { (void)args;
    CUR.GS.round_state = TT_Round_Up_To_Grid;

    CUR.func_round = (TRound_Function)Round_Up_To_Grid;
  }


/*******************************************/
/* RDTG[]    : Round Down To Grid          */
/* CodeRange : $7D                         */

  static void  Ins_RDTG( INS_ARG )
  { (void)args;
    CUR.GS.round_state = TT_Round_Down_To_Grid;

    CUR.func_round = (TRound_Function)Round_Down_To_Grid;
  }


/*******************************************/
/* ROFF[]    : Round OFF                   */
/* CodeRange : $7A                         */

  static void  Ins_ROFF( INS_ARG )
  { (void)args;
    CUR.GS.round_state = TT_Round_Off;

    CUR.func_round = (TRound_Function)Round_None;
  }


/*******************************************/
/* SROUND[]  : Super ROUND                 */
/* CodeRange : $76                         */

  static void  Ins_SROUND( INS_ARG )
  {
    SET_SuperRound( 0x4000L, args[0] );
    CUR.GS.round_state = TT_Round_Super;

    CUR.func_round = (TRound_Function)Round_Super;
  }


/*******************************************/
/* S45ROUND[]: Super ROUND 45 degrees      */
/* CodeRange : $77                         */

  static void  Ins_S45ROUND( INS_ARG )
  {
    SET_SuperRound( 0x2D41L, args[0] );
    CUR.GS.round_state = TT_Round_Super_45;

    CUR.func_round = (TRound_Function)Round_Super_45;
  }


/*******************************************/
/* SLOOP[]   : Set LOOP variable           */
/* CodeRange : $17                         */

  static void  Ins_SLOOP( INS_ARG )
  {
    CUR.GS.loop = args[0];
  }


/*******************************************/
/* SMD[]     : Set Minimum Distance        */
/* CodeRange : $1A                         */

  static void  Ins_SMD( INS_ARG )
  {
    CUR.GS.minimum_distance = args[0];
  }


/*******************************************/
/* INSTCTRL[]: INSTruction ConTRol         */
/* CodeRange : $8e                         */

  static void  Ins_INSTCTRL( INS_ARG )
  {
    Long  K, L;


    K = args[1];
    L = args[0];

    if ( K < 0 || K > 3 )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.GS.instruct_control = (Int)((CUR.GS.instruct_control & (~K)) | (L & K));
  }


/*******************************************/
/* SCANCTRL[]: SCAN ConTRol                */
/* CodeRange : $85                         */

  static void  Ins_SCANCTRL( INS_ARG )
  {
    Int  A;


    /* Get Threshold */
    A = (Int)(args[0] & 0xFF);

    if ( A == 0xFF )
    {
      CUR.GS.scan_control = TRUE;
      return;
    }
    else if ( A == 0 )
    {
      CUR.GS.scan_control = FALSE;
      return;
    }

    A *= 64;

    if ( (args[0] & 0x100) != 0 && CUR.metrics.pointSize <= A )
      CUR.GS.scan_control = TRUE;

    if ( (args[0] & 0x200) != 0 && CUR.metrics.rotated )
      CUR.GS.scan_control = TRUE;

    if ( (args[0] & 0x400) != 0 && CUR.metrics.stretched )
      CUR.GS.scan_control = TRUE;

    if ( (args[0] & 0x800) != 0 && CUR.metrics.pointSize > A )
      CUR.GS.scan_control = FALSE;

    if ( (args[0] & 0x1000) != 0 && CUR.metrics.rotated )
      CUR.GS.scan_control = FALSE;

    if ( (args[0] & 0x2000) != 0 && CUR.metrics.stretched )
      CUR.GS.scan_control = FALSE;
}


/*******************************************/
/* SCANTYPE[]: SCAN TYPE                   */
/* CodeRange : $8D                         */

  static void  Ins_SCANTYPE( INS_ARG )
  {
    /* For compatibility with future enhancements, */
    /* we must ignore new modes                    */

    if ( args[0] >= 0 && args[0] <= 5 )
    {
      if ( args[0] == 3 )
        args[0] = 2;

      CUR.GS.scan_type = (Int)args[0];
    }
  }


/**********************************************/
/* SCVTCI[]  : Set Control Value Table Cut In */
/* CodeRange : $1D                            */

  static void  Ins_SCVTCI( INS_ARG )
  {
    CUR.GS.control_value_cutin = (TT_F26Dot6)args[0];
  }


/**********************************************/
/* SSWCI[]   : Set Single Width Cut In        */
/* CodeRange : $1E                            */

  static void  Ins_SSWCI( INS_ARG )
  {
    CUR.GS.single_width_cutin = (TT_F26Dot6)args[0];
  }


/**********************************************/
/* SSW[]     : Set Single Width               */
/* CodeRange : $1F                            */

  static void  Ins_SSW( INS_ARG )
  {
    /* XXX : Undocumented or bug in the Windows engine ?  */
    /*                                                    */
    /* It seems that the value that is read here is       */
    /* expressed in 16.16 format, rather than in          */
    /* font units..                                       */
    /*                                                    */
    CUR.GS.single_width_value = (TT_F26Dot6)(args[0] >> 10);
  }


/**********************************************/
/* FLIPON[]  : Set Auto_flip to On            */
/* CodeRange : $4D                            */

  static void  Ins_FLIPON( INS_ARG )
  { (void)args;
    CUR.GS.auto_flip = TRUE;
  }


/**********************************************/
/* FLIPOFF[] : Set Auto_flip to Off           */
/* CodeRange : $4E                            */

  static void  Ins_FLIPOFF( INS_ARG )
  { (void)args;
    CUR.GS.auto_flip = FALSE;
  }


/**********************************************/
/* SANGW[]   : Set Angle Weight               */
/* CodeRange : $7E                            */

  static void  Ins_SANGW( INS_ARG )
  { (void)exc; (void)args;
    /* instruction not supported anymore */
  }


/**********************************************/
/* SDB[]     : Set Delta Base                 */
/* CodeRange : $5E                            */

  static void  Ins_SDB( INS_ARG )
  {
    CUR.GS.delta_base = (Int)args[0];
  }


/**********************************************/
/* SDS[]     : Set Delta Shift                */
/* CodeRange : $5F                            */

  static void  Ins_SDS( INS_ARG )
  {
    CUR.GS.delta_shift = (Int)args[0];
  }


/**********************************************/
/* GC[a]     : Get Coordinate projected onto  */
/* CodeRange : $46-$47                        */

/* BULLSHIT: Measures from the original glyph must to be taken */
/*           along the dual projection vector!                 */

  static void  Ins_GC( INS_ARG )
  {
    Long  L;


    L = args[0];

    if ( BOUNDS( L, CUR.zp2.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    switch ( CUR.opcode & 1 )
    {
    case 0:
      L = CUR_Func_project( CUR.zp2.cur_x[L],
                            CUR.zp2.cur_y[L] );
      break;

    case 1:
      L = CUR_Func_dualproj( CUR.zp2.org_x[L],
                             CUR.zp2.org_y[L] );
      break;
    }

    args[0] = L;
  }


/**********************************************/
/* SCFS[]    : Set Coordinate From Stack      */
/* CodeRange : $48                            */
/*                                            */
/* Formula:                                   */
/*                                            */
/*   OA := OA + ( value - OA.p )/( f.p ) * f  */
/*                                            */

  static void  Ins_SCFS( INS_ARG )
  {
    Long  K;
    Int   L;


    L = (Int)args[0];

    if ( BOUNDS( args[0], CUR.zp2.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    K = CUR_Func_project( CUR.zp2.cur_x[L],
                          CUR.zp2.cur_y[L] );

    CUR_Func_move( &CUR.zp2, L, args[1] - K );

    /* not part of the specs, but here for safety */

    if ( CUR.GS.gep2 == 0 )
    {
      CUR.zp2.org_x[L] = CUR.zp2.cur_x[L];
      CUR.zp2.org_y[L] = CUR.zp2.cur_y[L];
    }
  }


/**********************************************/
/* MD[a]     : Measure Distance               */
/* CodeRange : $49-$4A                        */

/* BULLSHIT: Measure taken in the original glyph must be along */
/*           the dual projection vector.                       */

/* Second BULLSHIT: Flag attributes are inverted!                */
/*                  0 => measure distance in original outline    */
/*                  1 => measure distance in grid-fitted outline */

  static void  Ins_MD( INS_ARG )
  {
    Long       K, L;
    TT_F26Dot6 D;

    K = args[1];
    L = args[0];

    if( BOUNDS( args[0], CUR.zp2.n_points ) ||
        BOUNDS( args[1], CUR.zp1.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( CUR.opcode & 1 )
      D = CUR_Func_project( CUR.zp2.cur_x[L] - CUR.zp1.cur_x[K],
                            CUR.zp2.cur_y[L] - CUR.zp1.cur_y[K] );
    else
      D = CUR_Func_dualproj( CUR.zp2.org_x[L] - CUR.zp1.org_x[K],
                             CUR.zp2.org_y[L] - CUR.zp1.org_y[K] );

    args[0] = D;
  }


/**********************************************/
/* MPPEM[]   : Measure Pixel Per EM           */
/* CodeRange : $4B                            */

  static void  Ins_MPPEM( INS_ARG )
  {
    args[0] = CURRENT_Ppem();
    DBG_PRINT1(" %d", args[0]);
  }


/**********************************************/
/* MPS[]     : Measure PointSize              */
/* CodeRange : $4C                            */

  static void  Ins_MPS( INS_ARG )
  {
    args[0] = CUR.metrics.pointSize;
  }



/****************************************************************/
/*                                                              */
/* MANAGING OUTLINES                                            */
/*                                                              */
/*  Instructions appear in the specs' order.                    */
/*                                                              */
/****************************************************************/

/**********************************************/
/* FLIPPT[]  : FLIP PoinT                     */
/* CodeRange : $80                            */

  static void  Ins_FLIPPT( INS_ARG )
  {
    Long  point;
    (void)args;

    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Too_Few_Arguments;
      return;
    }

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.pts.n_points ) )
      {
        CUR.error = TT_Err_Invalid_Reference;
       return;
      }

      CUR.pts.touch[point] ^= TT_Flag_On_Curve;

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


/**********************************************/
/* FLIPRGON[]: FLIP RanGe ON                  */
/* CodeRange : $81                            */

  static void  Ins_FLIPRGON( INS_ARG )
  {
    Long  I, K, L;


    K = args[1];
    L = args[0];

    if ( BOUNDS( K, CUR.pts.n_points ) ||
         BOUNDS( L, CUR.pts.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    for ( I = L; I <= K; I++ )
      CUR.pts.touch[I] |= TT_Flag_On_Curve;
  }


/**********************************************/
/* FLIPRGOFF : FLIP RanGe OFF                 */
/* CodeRange : $82                            */

  static void  Ins_FLIPRGOFF( INS_ARG )
  {
    Long  I, K, L;


    K = args[1];
    L = args[0];

    if ( BOUNDS( K, CUR.pts.n_points ) ||
         BOUNDS( L, CUR.pts.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    for ( I = L; I <= K; I++ )
      CUR.pts.touch[I] &= ~TT_Flag_On_Curve;
}


  static Bool  Compute_Point_Displacement( EXEC_OPS
                                           PCoordinates  x,
                                           PCoordinates  y,
                                           PGlyph_Zone   zone,
                                           Int*          refp )
  {
    TGlyph_Zone  zp;
    Int          p;
    TT_F26Dot6   d;


    if ( CUR.opcode & 1 )
    {
      zp = CUR.zp0;
      p  = CUR.GS.rp1;
    }
    else
    {
      zp = CUR.zp1;
      p  = CUR.GS.rp2;
    }

    if ( BOUNDS( p, zp.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Displacement;
      return FAILURE;
    }

    *zone = zp;
    *refp = p;

    d = CUR_Func_project( zp.cur_x[p] - zp.org_x[p],
                          zp.cur_y[p] - zp.org_y[p] );

    *x = MulDiv_Round(d, (Long)CUR.GS.freeVector.x * 0x10000L, CUR.F_dot_P );
    *y = MulDiv_Round(d, (Long)CUR.GS.freeVector.y * 0x10000L, CUR.F_dot_P );

    return SUCCESS;
  }


  static void  Move_Zp2_Point( EXEC_OPS
                               Long        point,
                               TT_F26Dot6  dx,
                               TT_F26Dot6  dy,
                               Bool        touch )
  {
    if ( CUR.GS.freeVector.x != 0 )
    {
      CUR.zp2.cur_x[point] += dx;
      if ( touch )
        CUR.zp2.touch[point] |= TT_Flag_Touched_X;
    }

    if ( CUR.GS.freeVector.y != 0 )
    {
      CUR.zp2.cur_y[point] += dy;
      if ( touch )
        CUR.zp2.touch[point] |= TT_Flag_Touched_Y;
    }
  }


/**********************************************/
/* SHP[a]    : SHift Point by the last point  */
/* CodeRange : $32-33                         */

  static void  Ins_SHP( INS_ARG )
  {
    TGlyph_Zone zp;
    Int         refp;

    TT_F26Dot6  dx,
                dy;
    Long        point;
    (void)args;

    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( COMPUTE_Point_Displacement( &dx, &dy, &zp, &refp ) )
      return;

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;
      point = CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.zp2.n_points ) )
      {
        CUR.error = TT_Err_Invalid_Reference;
        return;
      }

      /* undocumented: SHP touches the points */
      MOVE_Zp2_Point( point, dx, dy, TRUE );

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


/**********************************************/
/* SHC[a]    : SHift Contour                  */
/* CodeRange : $34-35                         */

  static void  Ins_SHC( INS_ARG )
  {
    TGlyph_Zone zp;
    Int         refp;
    TT_F26Dot6  dx,
                dy;

    Long        contour, i;
    Int         first_point, last_point;


    contour = args[0];

    if ( BOUNDS( args[0], CUR.pts.n_contours ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( COMPUTE_Point_Displacement( &dx, &dy, &zp, &refp ) )
      return;

    if ( contour == 0 )
      first_point = 0;
    else
      first_point = CUR.pts.contours[contour-1] + 1;

    last_point = CUR.pts.contours[contour];

    /* undocumented: SHC doesn't touch the points */
    for ( i = first_point; i <= last_point; i++ )
    {
      if ( zp.cur_x != CUR.zp2.cur_x || refp != i )
        MOVE_Zp2_Point( i, dx, dy, FALSE );
    }
  }


/**********************************************/
/* SHZ[a]    : SHift Zone                     */
/* CodeRange : $36-37                         */

  static void  Ins_SHZ( INS_ARG )
  {
    TGlyph_Zone zp;
    Int         refp;
    TT_F26Dot6  dx,
                dy;

    Int  last_point;
    Long i;


    if ( BOUNDS( args[0], 2 ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( COMPUTE_Point_Displacement( &dx, &dy, &zp, &refp ) )
      return;

    last_point = zp.n_points - 1;

    /* undocumented: SHZ doesn't touch the points */
    for ( i = 0; i <= last_point; i++ )
    {
      if ( zp.cur_x != CUR.zp2.cur_x || refp != i )
        MOVE_Zp2_Point( i, dx, dy, FALSE );
    }
  }


/**********************************************/
/* SHPIX[]   : SHift points by a PIXel amount */
/* CodeRange : $38                            */

  static void  Ins_SHPIX( INS_ARG )
  {
    TT_F26Dot6  dx, dy;
    Long        point;


    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    dx = MulDiv_Round( args[0],
                       (Long)CUR.GS.freeVector.x,
                       0x4000 );
    dy = MulDiv_Round( args[0],
                       (Long)CUR.GS.freeVector.y,
                       0x4000 );

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.zp2.n_points ) )
      {
        CUR.error = TT_Err_Invalid_Reference;
        return;
      }

      MOVE_Zp2_Point( point, dx, dy, TRUE );

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


/**********************************************/
/* MSIRP[a]  : Move Stack Indirect Relative   */
/* CodeRange : $3A-$3B                        */

  static void  Ins_MSIRP( INS_ARG )
  {
    Int         point;
    TT_F26Dot6  distance;


    point = (Int)args[0];

    if ( BOUNDS( args[0], CUR.zp1.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* XXX: Undocumented behaviour */

    if ( CUR.GS.gep0 == 0 )   /* if in twilight zone */
    {
      CUR.zp1.org_x[point] = CUR.zp0.org_x[CUR.GS.rp0];
      CUR.zp1.org_y[point] = CUR.zp0.org_y[CUR.GS.rp0];
      CUR.zp1.cur_x[point] = CUR.zp1.org_x[point];
      CUR.zp1.cur_y[point] = CUR.zp1.org_y[point];
    }

    distance = CUR_Func_project( CUR.zp1.cur_x[point] -
                                   CUR.zp0.cur_x[CUR.GS.rp0],
                                 CUR.zp1.cur_y[point] -
                                   CUR.zp0.cur_y[CUR.GS.rp0] );

    CUR_Func_move( &CUR.zp1, point, args[1] - distance );

    CUR.GS.rp1 = CUR.GS.rp0;
    CUR.GS.rp2 = point;

    if ( (CUR.opcode & 1) != 0 )
      CUR.GS.rp0 = point;
  }


/**********************************************/
/* MDAP[a]   : Move Direct Absolute Point     */
/* CodeRange : $2E-$2F                        */

  static void  Ins_MDAP( INS_ARG )
  {
    Int         point;
    TT_F26Dot6  cur_dist,
                distance;


    point = (Int)args[0];

    if ( BOUNDS( args[0], CUR.zp0.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* XXX: Is there some undocumented feature while in the */
    /*      twilight zone? ?                                */

    if ( (CUR.opcode & 1) != 0 )
    {
      cur_dist = CUR_Func_project( CUR.zp0.cur_x[point],
                                   CUR.zp0.cur_y[point] );
      distance = CUR_Func_round( cur_dist,
                                 CUR.metrics.compensations[0] ) - cur_dist;
    }
    else
      distance = 0;

    CUR_Func_move( &CUR.zp0, point, distance );

    CUR.GS.rp0 = point;
    CUR.GS.rp1 = point;
  }


/**********************************************/
/* MIAP[a]   : Move Indirect Absolute Point   */
/* CodeRange : $3E-$3F                        */

  static void  Ins_MIAP( INS_ARG )
  {
    Int         cvtEntry, point;
    TT_F26Dot6  distance,
                org_dist;


    cvtEntry = (Int)args[1];
    point    = (Int)args[0];

    if ( BOUNDS( args[0], CUR.zp0.n_points ) ||
         BOUNDS( args[1], CUR.cvtSize )      )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* Undocumented:                                     */
    /*                                                   */
    /* The behaviour of an MIAP instruction is quite     */
    /* different when used in the twilight zone.         */
    /*                                                   */
    /* First, no control value cutin test is performed   */
    /* as it would fail anyway.  Second, the original    */
    /* point, i.e. (org_x,org_y) of zp0.point, is set    */
    /* to the absolute, unrounded distance found in      */
    /* the CVT.                                          */
    /*                                                   */
    /* This is used in the CVT programs of the Microsoft */
    /* fonts Arial, Times, etc., in order to re-adjust   */
    /* some key font heights.  It allows the use of the  */
    /* IP instruction in the twilight zone, which        */
    /* otherwise would be "illegal" according to the     */
    /* specs :)                                          */
    /*                                                   */
    /* We implement it with a special sequence for the   */
    /* twilight zone. This is a bad hack, but it seems   */
    /* to work.                                          */

    distance = CUR_Func_read_cvt( cvtEntry );

    DBG_PRINT3(" cvtEntry=%d point=%d distance=%d", cvtEntry, point, distance);

    if ( CUR.GS.gep0 == 0 )   /* If in twilight zone */
    {
      CUR.zp0.org_x[point] = MulDiv_Round( CUR.GS.freeVector.x,
                                           distance, 0x4000L );
      CUR.zp0.cur_x[point] = CUR.zp0.org_x[point];

      CUR.zp0.org_y[point] = MulDiv_Round( CUR.GS.freeVector.y,
                                           distance, 0x4000L );
      CUR.zp0.cur_y[point] = CUR.zp0.org_y[point];
    }

    org_dist = CUR_Func_project( CUR.zp0.cur_x[point],
                                 CUR.zp0.cur_y[point] );

    if ( (CUR.opcode & 1) != 0 )   /* rounding and control cutin flag */
    {
      if ( ABS(distance - org_dist) > CUR.GS.control_value_cutin )
        distance = org_dist;

      distance = CUR_Func_round( distance, CUR.metrics.compensations[0] );
    }

    CUR_Func_move( &CUR.zp0, point, distance - org_dist );

    CUR.GS.rp0 = point;
    CUR.GS.rp1 = point;
  }


/**********************************************/
/* MDRP[abcde] : Move Direct Relative Point   */
/* CodeRange   : $C0-$DF                      */

  static void  Ins_MDRP( INS_ARG )
  {
    Int         point;
    TT_F26Dot6  distance,
                org_dist;


    point = (Int)args[0];

    if ( BOUNDS( args[0], CUR.zp1.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* XXX: Is there some undocumented feature while in the */
    /*      twilight zone?                                  */

    org_dist = CUR_Func_dualproj( CUR.zp1.org_x[point] -
                                    CUR.zp0.org_x[CUR.GS.rp0],
                                  CUR.zp1.org_y[point] -
                                    CUR.zp0.org_y[CUR.GS.rp0] );

    /* single width cutin test */

    if ( ABS(org_dist) < CUR.GS.single_width_cutin )
    {
      if ( org_dist >= 0 )
        org_dist = CUR.GS.single_width_value;
      else
        org_dist = -CUR.GS.single_width_value;
    }

    /* round flag */

    if ( (CUR.opcode & 4) != 0 )
      distance = CUR_Func_round( org_dist,
                                 CUR.metrics.compensations[CUR.opcode & 3] );
    else
      distance = Round_None( EXEC_ARGS
                             org_dist,
                             CUR.metrics.compensations[CUR.opcode & 3]  );

    /* minimum distance flag */

    if ( (CUR.opcode & 8) != 0 )
    {
      if ( org_dist >= 0 )
      {
        if ( distance < CUR.GS.minimum_distance )
          distance = CUR.GS.minimum_distance;
      }
      else
      {
        if ( distance > -CUR.GS.minimum_distance )
          distance = -CUR.GS.minimum_distance;
      }
    }

    /* now move the point */

    org_dist = CUR_Func_project( CUR.zp1.cur_x[point] -
                                   CUR.zp0.cur_x[CUR.GS.rp0],
                                 CUR.zp1.cur_y[point] -
                                   CUR.zp0.cur_y[CUR.GS.rp0] );

    CUR_Func_move( &CUR.zp1, point, distance - org_dist );

    CUR.GS.rp1 = CUR.GS.rp0;
    CUR.GS.rp2 = point;

    if ( (CUR.opcode & 16) != 0 )
      CUR.GS.rp0 = point;
  }


/**********************************************/
/* MIRP[abcde] : Move Indirect Relative Point */
/* CodeRange   : $E0-$FF                      */

  static void  Ins_MIRP( INS_ARG )
  {
    Int         point,
                cvtEntry;

    TT_F26Dot6  cvt_dist,
                distance,
                cur_dist,
                org_dist;


    point    = (Int)args[0];
    cvtEntry = (Int)args[1];

    /* XXX: UNDOCUMENTED! cvt[-1] = 0 always */

    if ( BOUNDS( args[0],   CUR.zp1.n_points ) ||
         BOUNDS( args[1]+1, CUR.cvtSize+1 )    )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( args[1] < 0 )
      cvt_dist = 0;
    else
      cvt_dist = CUR_Func_read_cvt( cvtEntry );

    /* single width test */

    if ( ABS( cvt_dist ) < CUR.GS.single_width_cutin )
    {
      if ( cvt_dist >= 0 )
        cvt_dist =  CUR.GS.single_width_value;
      else
        cvt_dist = -CUR.GS.single_width_value;
    }

    /* XXX : Undocumented - twilight zone */

    if ( CUR.GS.gep1 == 0 )
    {
      CUR.zp1.org_x[point] = CUR.zp0.org_x[CUR.GS.rp0] +
                             MulDiv_Round( cvt_dist,
                                           CUR.GS.freeVector.x,
                                           0x4000 );

      CUR.zp1.org_y[point] = CUR.zp0.org_y[CUR.GS.rp0] +
                             MulDiv_Round( cvt_dist,
                                           CUR.GS.freeVector.y,
                                           0x4000 );

      CUR.zp1.cur_x[point] = CUR.zp1.org_x[point];
      CUR.zp1.cur_y[point] = CUR.zp1.org_y[point];
    }

    org_dist = CUR_Func_dualproj( CUR.zp1.org_x[point] -
                                    CUR.zp0.org_x[CUR.GS.rp0],
                                  CUR.zp1.org_y[point] -
                                    CUR.zp0.org_y[CUR.GS.rp0] );

    cur_dist = CUR_Func_project( CUR.zp1.cur_x[point] -
                                   CUR.zp0.cur_x[CUR.GS.rp0],
                                 CUR.zp1.cur_y[point] -
                                   CUR.zp0.cur_y[CUR.GS.rp0] );

    /* auto-flip test */

    if ( CUR.GS.auto_flip )
    {
      if ( (org_dist ^ cvt_dist) < 0 )
        cvt_dist = -cvt_dist;
    }

    /* control value cutin and round */

    if ( (CUR.opcode & 4) != 0 )
    {
      /* XXX: Undocumented : only perform cut-in test when both points */
      /*      refer to the same zone.                                  */

      if ( CUR.GS.gep0 == CUR.GS.gep1 )
        if ( ABS( cvt_dist - org_dist ) >= CUR.GS.control_value_cutin )
          cvt_dist = org_dist;

      distance = CUR_Func_round( cvt_dist,
                                 CUR.metrics.compensations[CUR.opcode & 3] );
    }
    else
      distance = Round_None( EXEC_ARGS
                             cvt_dist,
                             CUR.metrics.compensations[CUR.opcode & 3] );

    /* minimum distance test */

    if ( (CUR.opcode & 8) != 0 )
    {
      if ( org_dist >= 0 )
      {
        if ( distance < CUR.GS.minimum_distance )
          distance = CUR.GS.minimum_distance;
      }
      else
      {
        if ( distance > -CUR.GS.minimum_distance )
          distance = -CUR.GS.minimum_distance;
      }
    }

    CUR_Func_move( &CUR.zp1, point, distance - cur_dist );

    CUR.GS.rp1 = CUR.GS.rp0;

    if ( (CUR.opcode & 16) != 0 )
      CUR.GS.rp0 = point;

    /* UNDOCUMENTED! */

    CUR.GS.rp2 = point;
  }


/**********************************************/
/* ALIGNRP[]   : ALIGN Relative Point         */
/* CodeRange   : $3C                          */

  static void  Ins_ALIGNRP( INS_ARG )
  {
    Int         point;
    TT_F26Dot6  distance;
    (void)args;

    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (Int)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.zp1.n_points ) )
      {
        CUR.error = TT_Err_Invalid_Reference;
        return;
      }

      distance = CUR_Func_project( CUR.zp1.cur_x[point] -
                                     CUR.zp0.cur_x[CUR.GS.rp0],
                                   CUR.zp1.cur_y[point] -
                                     CUR.zp0.cur_y[CUR.GS.rp0] );

      CUR_Func_move( &CUR.zp1, point, -distance );
      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


/**********************************************/
/* AA[]        : Adjust Angle                 */
/* CodeRange   : $7F                          */

  static void  Ins_AA( INS_ARG )
  { (void)exc; (void)args;
    /* Intentional - no longer supported */
  }


/**********************************************/
/* ISECT[]     : moves point to InterSECTion  */
/* CodeRange   : $0F                          */

  static void  Ins_ISECT( INS_ARG )
  {
    Long  point,           /* are these Ints or Longs? */
          a0, a1,
          b0, b1;

    TT_F26Dot6  discriminant;

    TT_F26Dot6  dx,  dy,
                dax, day,
                dbx, dby;

    TT_F26Dot6  val;

    TT_Vector   R;


    point = args[0];

    a0 = args[1];
    a1 = args[2];
    b0 = args[3];
    b1 = args[4];

    if ( BOUNDS( b0, CUR.zp0.n_points ) ||
         BOUNDS( b1, CUR.zp0.n_points ) ||
         BOUNDS( a0, CUR.zp1.n_points ) ||
         BOUNDS( a1, CUR.zp1.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    dbx = CUR.zp0.cur_x[b1] - CUR.zp0.cur_x[b0];
    dby = CUR.zp0.cur_y[b1] - CUR.zp0.cur_y[b0];

    dax = CUR.zp1.cur_x[a1] - CUR.zp1.cur_x[a0];
    day = CUR.zp1.cur_y[a1] - CUR.zp1.cur_y[a0];

    dx = CUR.zp0.cur_x[b0] - CUR.zp1.cur_x[a0];
    dy = CUR.zp0.cur_y[b0] - CUR.zp1.cur_y[a0];

    CUR.zp2.touch[point] |= TT_Flag_Touched_Both;

    discriminant = MulDiv_Round( dax, -dby, 0x40L ) +
                   MulDiv_Round( day, dbx, 0x40L );

    if ( ABS( discriminant ) >= 0x40 )
    {
      val = MulDiv_Round( dx, -dby, 0x40L ) + MulDiv_Round( dy, dbx, 0x40L );

      R.x = MulDiv_Round( val, dax, discriminant );
      R.y = MulDiv_Round( val, day, discriminant );

      CUR.zp2.cur_x[point] = CUR.zp1.cur_x[a0] + R.x;
      CUR.zp2.cur_y[point] = CUR.zp1.cur_y[a0] + R.y;
    }
    else
    {
      /* else, take the middle of the middles of A and B */

      CUR.zp2.cur_x[point] = ( CUR.zp1.cur_x[a0] +
                               CUR.zp1.cur_x[a1] +
                               CUR.zp0.cur_x[b0] +
                               CUR.zp1.cur_x[b1] ) / 4;
      CUR.zp2.cur_y[point] = ( CUR.zp1.cur_y[a0] +
                               CUR.zp1.cur_y[a1] +
                               CUR.zp0.cur_y[b0] +
                               CUR.zp1.cur_y[b1] ) / 4;
    }
  }


/**********************************************/
/* ALIGNPTS[]  : ALIGN PoinTS                 */
/* CodeRange   : $27                          */

  static void  Ins_ALIGNPTS( INS_ARG )
  {
    Int         p1, p2;
    TT_F26Dot6  distance;


    p1 = (Int)args[0];
   p2 = (Int)args[1];

    if ( BOUNDS( args[0], CUR.zp1.n_points ) ||
         BOUNDS( args[1], CUR.zp0.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    distance = CUR_Func_project( CUR.zp0.cur_x[p2] -
                                   CUR.zp1.cur_x[p1],
                                 CUR.zp0.cur_y[p2] -
                                   CUR.zp1.cur_x[p1] ) / 2;

    CUR_Func_move( &CUR.zp1, p1, distance );

    CUR_Func_move( &CUR.zp0, p2, -distance );
  }


/**********************************************/
/* IP[]        : Interpolate Point            */
/* CodeRange   : $39                          */

  static void  Ins_IP( INS_ARG )
  {
    TT_F26Dot6  org_a, org_b, org_x,
                cur_a, cur_b, cur_x,
                distance;
    Int         point;
    (void)args;

    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    org_a = CUR_Func_dualproj( CUR.zp0.org_x[CUR.GS.rp1],
                               CUR.zp0.org_y[CUR.GS.rp1] );

    org_b = CUR_Func_dualproj( CUR.zp1.org_x[CUR.GS.rp2],
                               CUR.zp1.org_y[CUR.GS.rp2] );

    cur_a = CUR_Func_project( CUR.zp0.cur_x[CUR.GS.rp1],
                              CUR.zp0.cur_y[CUR.GS.rp1] );

    cur_b = CUR_Func_project( CUR.zp1.cur_x[CUR.GS.rp2],
                              CUR.zp1.cur_y[CUR.GS.rp2] );

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (Int)CUR.stack[CUR.args];
      if ( BOUNDS( point, CUR.zp2.n_points ) )
      {
        CUR.error = TT_Err_Invalid_Reference;
        return;
      }

      org_x = CUR_Func_dualproj( CUR.zp2.org_x[point],
                                 CUR.zp2.org_y[point] );

      cur_x = CUR_Func_project( CUR.zp2.cur_x[point],
                                CUR.zp2.cur_y[point] );

      if ( ( org_a <= org_b && org_x <= org_a ) ||
           ( org_a >  org_b && org_x >= org_a ) )

        distance = ( cur_a - org_a ) + ( org_x - cur_x );

      else if ( ( org_a <= org_b  &&  org_x >= org_b ) ||
                ( org_a >  org_b  &&  org_x <  org_b ) )

        distance = ( cur_b - org_b ) + ( org_x - cur_x );

      else
         /* note: it seems that rounding this value isn't a good */
         /*       idea (cf. width of capital 'S' in Times)       */

         distance = MulDiv_Round( cur_b - cur_a,
                                  org_x - org_a,
                                  org_b - org_a ) + ( cur_a - cur_x );

      CUR_Func_move( &CUR.zp2, point, distance );

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


/**********************************************/
/* UTP[a]      : UnTouch Point                */
/* CodeRange   : $29                          */

  static void  Ins_UTP( INS_ARG )
  {
    Byte  mask;

    if ( BOUNDS( args[0], CUR.zp0.n_points ) )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    mask = 0xFF;

    if ( CUR.GS.freeVector.x != 0 )
      mask &= ~TT_Flag_Touched_X;

    if ( CUR.GS.freeVector.y != 0 )
      mask &= ~TT_Flag_Touched_Y;

    CUR.zp0.touch[args[0]] &= mask;
  }


  /* Local variables for Ins_IUP: */
  struct LOC_Ins_IUP
  {
    PCoordinates  orgs;   /* original and current coordinate */
    PCoordinates  curs;   /* arrays                          */
  };


  static void  Shift( Int  p1,
                      Int  p2,
                      Int  p,
                      struct LOC_Ins_IUP*  LINK )
  {
    Int         i;
    TT_F26Dot6  x;


    x = LINK->curs[p] - LINK->orgs[p];

    for ( i = p1; i < p; i++ )
      LINK->curs[i] += x;

    for ( i = p + 1; i <= p2; i++ )
      LINK->curs[i] += x;
  }


  static void  Interp( Int  p1, Int  p2,
                       Int  ref1, Int  ref2,
                       struct LOC_Ins_IUP*  LINK )
  {
    Long        i;
    TT_F26Dot6  x, x1, x2, d1, d2;


    if ( p1 > p2 )
      return;

    x1 = LINK->orgs[ref1];
    d1 = LINK->curs[ref1] - LINK->orgs[ref1];
    x2 = LINK->orgs[ref2];
    d2 = LINK->curs[ref2] - LINK->orgs[ref2];

    if ( x1 == x2 )
    {
      for ( i = p1; i <= p2; i++ )
      {
        x = LINK->orgs[i];

        if ( x <= x1 )
          x += d1;
        else
          x += d2;

        LINK->curs[i] = x;
      }
      return;
    }

    if ( x1 < x2 )
    {
      for ( i = p1; i <= p2; i++ )
      {
        x = LINK->orgs[i];

        if ( x <= x1 )
          x += d1;
        else
        {
          if ( x >= x2 )
            x += d2;
          else
            x = LINK->curs[ref1] +
                  MulDiv_Round( x - x1,
                                LINK->curs[ref2] - LINK->curs[ref1],
                                x2 - x1 );
        }
        LINK->curs[i] = x;
      }
      return;
    }

    /* x2 < x1 */

    for ( i = p1; i <= p2; i++ )
    {
      x = LINK->orgs[i];
      if ( x <= x2 )
        x += d2;
      else
      {
        if ( x >= x1 )
          x += d1;
        else
          x = LINK->curs[ref1] +
                MulDiv_Round( x - x1,
                              LINK->curs[ref2] - LINK->curs[ref1],
                              x2 - x1 );
      }
      LINK->curs[i] = x;
    }
  }


/**********************************************/
/* IUP[a]      : Interpolate Untouched Points */
/* CodeRange   : $30-$31                      */

  static void  Ins_IUP( INS_ARG )
  {
    struct LOC_Ins_IUP  V;
    unsigned char       mask;

    Long first_point;   /* first point of contour        */
    Long end_point;     /* end point (last+1) of contour */

    Long first_touched; /* first touched point in contour   */
    Long cur_touched;   /* current touched point in contour */

    Long point;         /* current point   */
    Long contour;       /* current contour */
    (void)args;

    if ( CUR.opcode & 1 )
    {
      mask   = TT_Flag_Touched_X;
      V.orgs = CUR.pts.org_x;
      V.curs = CUR.pts.cur_x;
    }
    else
    {
      mask   = TT_Flag_Touched_Y;
      V.orgs = CUR.pts.org_y;
      V.curs = CUR.pts.cur_y;
    }

    contour = 0;
    point   = 0;

    do
    {
      end_point   = CUR.pts.contours[contour];
      first_point = point;

      while ( point <= end_point && (CUR.pts.touch[point] & mask) == 0 )
        point++;

      if ( point <= end_point )
      {
        first_touched = point;
        cur_touched   = point;

        point++;

        while ( point <= end_point )
        {
          if ( (CUR.pts.touch[point] & mask) != 0 )
          {
            Interp( (Int)(cur_touched + 1),
                    (Int)(point - 1),
                    (Int)cur_touched,
                    (Int)point,
                    &V );
            cur_touched = point;
          }

          point++;
        }

        if ( cur_touched == first_touched )
          Shift( (Int)first_point, (Int)end_point, (Int)cur_touched, &V );
        else
        {
          Interp((Int)(cur_touched + 1),
                 (Int)(end_point),
                 (Int)(cur_touched),
                 (Int)(first_touched),
                 &V );

          Interp((Int)(first_point),
                 (Int)(first_touched - 1),
                 (Int)(cur_touched),
                 (Int)(first_touched),
                 &V );
        }
      }
      contour++;
    } while ( contour < CUR.pts.n_contours );
  }


/**********************************************/
/* DELTAPn[]   : DELTA Exceptions P1, P2, P3  */
/* CodeRange   : $5D,$71,$72                  */

  static void  Ins_DELTAP( INS_ARG )
  {
    Int   k;
    Long  A, B, C, nump;


    nump = args[0];

    for ( k = 1; k <= nump; k++ )
    {
      if ( CUR.args < 2 )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        return;
      }

      CUR.args -= 2;

      A = CUR.stack[CUR.args + 1];
      B = CUR.stack[CUR.args];
#if 0
      if ( BOUNDS( A, CUR.zp0.n_points ) )
#else
      /* igorm changed : allow phantom points (Altona.Page_3.2002-09-27.pdf). */
      if ( BOUNDS( A, CUR.zp0.n_points + 2 ) )
#endif
      {
        CUR.error = TT_Err_Invalid_Reference;
        return;
      }

      C = (B & 0xF0) >> 4;

      switch ( CUR.opcode )
      {
      case 0x5d:
        break;

      case 0x71:
        C += 16;
        break;

      case 0x72:
        C += 32;
        break;
      }

      C += CUR.GS.delta_base;

      if ( CURRENT_Ppem() == C )
      {
        B = (B & 0xF) - 8;
        if ( B >= 0 )
          B++;
        B = B * 64 / (1L << CUR.GS.delta_shift);

        CUR_Func_move( &CUR.zp0, (Int)A, (Int)B );
      }
    }

    CUR.new_top = CUR.args;
  }


/**********************************************/
/* DELTACn[]   : DELTA Exceptions C1, C2, C3  */
/* CodeRange   : $73,$74,$75                  */

  static void  Ins_DELTAC( INS_ARG )
  {
    Long  nump, k;
    Long  A, B, C;


    nump = args[0];

    for ( k = 1; k <= nump; k++ )
    {
      if ( CUR.args < 2 )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        return;
      }

      CUR.args -= 2;

      A = CUR.stack[CUR.args + 1];
      B = CUR.stack[CUR.args];

      if ( A >= CUR.cvtSize )
      {
        CUR.error = TT_Err_Invalid_Reference;
        return;
      }

      C = ((unsigned long)(B & 0xF0)) >> 4;

      switch ( CUR.opcode )
      {
      case 0x73:
        break;

      case 0x74:
        C += 16;
        break;

      case 0x75:
        C += 32;
        break;
      }

      C += CUR.GS.delta_base;

      if ( CURRENT_Ppem() == C )
      {
        B = (B & 0xF) - 8;
        if ( B >= 0 )
          B++;
        B = B * 64 / (1L << CUR.GS.delta_shift);

        CUR_Func_move_cvt( A, B );
      }
    }

    CUR.new_top = CUR.args;
  }



/****************************************************************/
/*                                                              */
/* MISC. INSTRUCTIONS                                           */
/*                                                              */
/****************************************************************/

/***********************************************************/
/* DEBUG[]     : DEBUG. Unsupported                        */
/* CodeRange   : $4F                                       */

/* NOTE : The original instruction pops a value from the stack */

  static void  Ins_DEBUG( INS_ARG )
  { (void)args;
    CUR.error = TT_Err_Debug_OpCode;
  }


/**********************************************/
/* GETINFO[]   : GET INFOrmation              */
/* CodeRange   : $88                          */

  static void  Ins_GETINFO( INS_ARG )
  {
    Long  K;


    K = 0;

    /* We return then Windows 3.1 version number */
    /* for the font scaler                       */
    if ( (args[0] & 1) != 0 )
      K = 3;

    /* Has the glyph been rotated ? */
    if ( CUR.metrics.rotated )
      K |= 0x80;

    /* Has the glyph been stretched ? */
    if ( CUR.metrics.stretched )
      K |= 0x100;

    args[0] = K;
  }


  static void  Ins_UNKNOWN( INS_ARG )
  { /* Rewritten by igorm. */
    Byte i;
    TDefRecord*  def;
    PCallRecord  call;

#   if 0     /* The condition below appears always false 
		due to limited range of data type
		- skip it to quiet a compiler warning. */
    if (CUR.opcode > sizeof(CUR.IDefPtr) / sizeof(CUR.IDefPtr[0])) {
	CUR.error = TT_Err_Invalid_Opcode;
	return;
    }
#   endif
    i = CUR.IDefPtr[(Byte)CUR.opcode];

    if (i >= CUR.numIDefs) 
      {
	CUR.error = TT_Err_Invalid_Opcode;
	return;
      }
    def   = &CUR.IDefs[i];


    if ( CUR.callTop >= CUR.callSize )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    call = CUR.callStack + CUR.callTop++;

    call->Caller_Range = CUR.curRange;
    call->Caller_IP    = CUR.IP+1;
    call->Cur_Count    = 1;
    call->Cur_Restart  = def->Start;

    INS_Goto_CodeRange( def->Range, def->Start );

    CUR.step_ins = FALSE;
    return;
  }


  static struct { const char *sName; TInstruction_Function p; }  Instruct_Dispatch[256] =
  {
    /* Opcodes are gathered in groups of 16. */
    /* Please keep the spaces as they are.   */

     {"  SVTCA  y  ",  Ins_SVTCA  }
    ,{"  SVTCA  x  ",  Ins_SVTCA  }
    ,{"  SPvTCA y  ",  Ins_SPVTCA }
    ,{"  SPvTCA x  ",  Ins_SPVTCA }
    ,{"  SFvTCA y  ",  Ins_SFVTCA }
    ,{"  SFvTCA x  ",  Ins_SFVTCA }
    ,{"  SPvTL //  ",  Ins_SPVTL  }
    ,{"  SPvTL +   ",  Ins_SPVTL  }
    ,{"  SFvTL //  ",  Ins_SFVTL  }
    ,{"  SFvTL +   ",  Ins_SFVTL  }
    ,{"  SPvFS     ",  Ins_SPVFS  }
    ,{"  SFvFS     ",  Ins_SFVFS  }
    ,{"  GPV       ",  Ins_GPV    }
    ,{"  GFV       ",  Ins_GFV    }
    ,{"  SFvTPv    ",  Ins_SFVTPV }
    ,{"  ISECT     ",  Ins_ISECT  }

    ,{"  SRP0      ",  Ins_SRP0   }
    ,{"  SRP1      ",  Ins_SRP1   }
    ,{"  SRP2      ",  Ins_SRP2   }
    ,{"  SZP0      ",  Ins_SZP0   }
    ,{"  SZP1      ",  Ins_SZP1   }
    ,{"  SZP2      ",  Ins_SZP2   }
    ,{"  SZPS      ",  Ins_SZPS   }
    ,{"  SLOOP     ",  Ins_SLOOP  }
    ,{"  RTG       ",  Ins_RTG    }
    ,{"  RTHG      ",  Ins_RTHG   }
    ,{"  SMD       ",  Ins_SMD    }
    ,{"  ELSE      ",  Ins_ELSE   }
    ,{"  JMPR      ",  Ins_JMPR   }
    ,{"  SCvTCi    ",  Ins_SCVTCI }
    ,{"  SSwCi     ",  Ins_SSWCI  }
    ,{"  SSW       ",  Ins_SSW    }

    ,{"  DUP       ",  Ins_DUP    }
    ,{"  POP       ",  Ins_POP    }
    ,{"  CLEAR     ",  Ins_CLEAR  }
    ,{"  SWAP      ",  Ins_SWAP   }
    ,{"  DEPTH     ",  Ins_DEPTH  }
    ,{"  CINDEX    ",  Ins_CINDEX }
    ,{"  MINDEX    ",  Ins_MINDEX }
    ,{"  AlignPTS  ",  Ins_ALIGNPTS}
    ,{"  INS_$28   ",  Ins_UNKNOWN }
    ,{"  UTP       ",  Ins_UTP     }
    ,{"  LOOPCALL  ",  Ins_LOOPCALL}
    ,{"  CALL      ",  Ins_CALL    }
    ,{"  FDEF      ",  Ins_FDEF    }
    ,{"  ENDF      ",  Ins_ENDF    }
    ,{"  MDAP[0]   ",  Ins_MDAP    }
    ,{"  MDAP[1]   ",  Ins_MDAP    }

    ,{"  IUP[0]    ",  Ins_IUP       }
    ,{"  IUP[1]    ",  Ins_IUP       }
    ,{"  SHP[0]    ",  Ins_SHP       }
    ,{"  SHP[1]    ",  Ins_SHP       }
    ,{"  SHC[0]    ",  Ins_SHC       }
    ,{"  SHC[1]    ",  Ins_SHC       }
    ,{"  SHZ[0]    ",  Ins_SHZ       }
    ,{"  SHZ[1]    ",  Ins_SHZ       }
    ,{"  SHPIX     ",  Ins_SHPIX     }
    ,{"  IP        ",  Ins_IP        }
    ,{"  MSIRP[0]  ",  Ins_MSIRP     }
    ,{"  MSIRP[1]  ",  Ins_MSIRP     }
    ,{"  AlignRP   ",  Ins_ALIGNRP   }
    ,{"  RTDG      ",  Ins_RTDG      }
    ,{"  MIAP[0]   ",  Ins_MIAP      }
    ,{"  MIAP[1]   ",  Ins_MIAP      }

    ,{"  NPushB    ",  Ins_NPUSHB     }
    ,{"  NPushW    ",  Ins_NPUSHW     }
    ,{"  WS        ",  Ins_WS         }
    ,{"  RS        ",  Ins_RS         }
    ,{"  WCvtP     ",  Ins_WCVTP      }
    ,{"  RCvt      ",  Ins_RCVT       }
    ,{"  GC[0]     ",  Ins_GC         }
    ,{"  GC[1]     ",  Ins_GC         }
    ,{"  SCFS      ",  Ins_SCFS       }
    ,{"  MD[0]     ",  Ins_MD         }
    ,{"  MD[1]     ",  Ins_MD         }
    ,{"  MPPEM     ",  Ins_MPPEM      }
    ,{"  MPS       ",  Ins_MPS        }
    ,{"  FlipON    ",  Ins_FLIPON     }
    ,{"  FlipOFF   ",  Ins_FLIPOFF    }
    ,{"  DEBUG     ",  Ins_DEBUG      }

    ,{"  LT        ",  Ins_LT         }
    ,{"  LTEQ      ",  Ins_LTEQ       }
    ,{"  GT        ",  Ins_GT         }
    ,{"  GTEQ      ",  Ins_GTEQ       }
    ,{"  EQ        ",  Ins_EQ         }
    ,{"  NEQ       ",  Ins_NEQ        }
    ,{"  ODD       ",  Ins_ODD        }
    ,{"  EVEN      ",  Ins_EVEN       }
    ,{"  IF        ",  Ins_IF         }
    ,{"  EIF       ",  Ins_EIF        }
    ,{"  AND       ",  Ins_AND        }
    ,{"  OR        ",  Ins_OR         }
    ,{"  NOT       ",  Ins_NOT        }
    ,{"  DeltaP1   ",  Ins_DELTAP     }
    ,{"  SDB       ",  Ins_SDB        }
    ,{"  SDS       ",  Ins_SDS        }

    ,{"  ADD       ",  Ins_ADD       }
    ,{"  SUB       ",  Ins_SUB       }
    ,{"  DIV       ",  Ins_DIV       }
    ,{"  MUL       ",  Ins_MUL       }
    ,{"  ABS       ",  Ins_ABS       }
    ,{"  NEG       ",  Ins_NEG       }
    ,{"  FLOOR     ",  Ins_FLOOR     }
    ,{"  CEILING   ",  Ins_CEILING   }
    ,{"  ROUND[0]  ",  Ins_ROUND     }
    ,{"  ROUND[1]  ",  Ins_ROUND     }
    ,{"  ROUND[2]  ",  Ins_ROUND     }
    ,{"  ROUND[3]  ",  Ins_ROUND     }
    ,{"  NROUND[0] ",  Ins_NROUND    }
    ,{"  NROUND[1] ",  Ins_NROUND    }
    ,{"  NROUND[2] ",  Ins_NROUND    }
    ,{"  NROUND[3] ",  Ins_NROUND    }

    ,{"  WCvtF     ",  Ins_WCVTF      }
    ,{"  DeltaP2   ",  Ins_DELTAP     }
    ,{"  DeltaP3   ",  Ins_DELTAP     }
    ,{"  DeltaCn[0] ", Ins_DELTAC     }
    ,{"  DeltaCn[1] ", Ins_DELTAC     }
    ,{"  DeltaCn[2] ", Ins_DELTAC     }
    ,{"  SROUND    ",  Ins_SROUND     }
    ,{"  S45Round  ",  Ins_S45ROUND   }
    ,{"  JROT      ",  Ins_JROT       }
    ,{"  JROF      ",  Ins_JROF       }
    ,{"  ROFF      ",  Ins_ROFF       }
    ,{"  INS_$7B   ",  Ins_UNKNOWN    }
    ,{"  RUTG      ",  Ins_RUTG       }
    ,{"  RDTG      ",  Ins_RDTG       }
    ,{"  SANGW     ",  Ins_SANGW      }
    ,{"  AA        ",  Ins_AA         }

    ,{"  FlipPT    ",  Ins_FLIPPT     }
    ,{"  FlipRgON  ",  Ins_FLIPRGON   }
    ,{"  FlipRgOFF ",  Ins_FLIPRGOFF  }
    ,{"  INS_$83   ",  Ins_UNKNOWN    }
    ,{"  INS_$84   ",  Ins_UNKNOWN    }
    ,{"  ScanCTRL  ",  Ins_SCANCTRL   }
    ,{"  SDPVTL[0] ",  Ins_SDPVTL     }
    ,{"  SDPVTL[1] ",  Ins_SDPVTL     }
    ,{"  GetINFO   ",  Ins_GETINFO    }
    ,{"  IDEF      ",  Ins_IDEF       }
    ,{"  ROLL      ",  Ins_ROLL       }
    ,{"  MAX       ",  Ins_MAX        }
    ,{"  MIN       ",  Ins_MIN        }
    ,{"  ScanTYPE  ",  Ins_SCANTYPE   }
    ,{"  InstCTRL  ",  Ins_INSTCTRL   }
    ,{"  INS_$8F   ",  Ins_UNKNOWN    }

    ,{"  INS_$90  ",   Ins_UNKNOWN   }
    ,{"  INS_$91  ",   Ins_UNKNOWN   }
    ,{"  INS_$92  ",   Ins_UNKNOWN   }
    ,{"  INS_$93  ",   Ins_UNKNOWN   }
    ,{"  INS_$94  ",   Ins_UNKNOWN   }
    ,{"  INS_$95  ",   Ins_UNKNOWN   }
    ,{"  INS_$96  ",   Ins_UNKNOWN   }
    ,{"  INS_$97  ",   Ins_UNKNOWN   }
    ,{"  INS_$98  ",   Ins_UNKNOWN   }
    ,{"  INS_$99  ",   Ins_UNKNOWN   }
    ,{"  INS_$9A  ",   Ins_UNKNOWN   }
    ,{"  INS_$9B  ",   Ins_UNKNOWN   }
    ,{"  INS_$9C  ",   Ins_UNKNOWN   }
    ,{"  INS_$9D  ",   Ins_UNKNOWN   }
    ,{"  INS_$9E  ",   Ins_UNKNOWN   }
    ,{"  INS_$9F  ",   Ins_UNKNOWN   }

    ,{"  INS_$A0  ",   Ins_UNKNOWN   }
    ,{"  INS_$A1  ",   Ins_UNKNOWN   }
    ,{"  INS_$A2  ",   Ins_UNKNOWN   }
    ,{"  INS_$A3  ",   Ins_UNKNOWN   }
    ,{"  INS_$A4  ",   Ins_UNKNOWN   }
    ,{"  INS_$A5  ",   Ins_UNKNOWN   }
    ,{"  INS_$A6  ",   Ins_UNKNOWN   }
    ,{"  INS_$A7  ",   Ins_UNKNOWN   }
    ,{"  INS_$A8  ",   Ins_UNKNOWN   }
    ,{"  INS_$A9  ",   Ins_UNKNOWN   }
    ,{"  INS_$AA  ",   Ins_UNKNOWN   }
    ,{"  INS_$AB  ",   Ins_UNKNOWN   }
    ,{"  INS_$AC  ",   Ins_UNKNOWN   }
    ,{"  INS_$AD  ",   Ins_UNKNOWN   }
    ,{"  INS_$AE  ",   Ins_UNKNOWN   }
    ,{"  INS_$AF  ",   Ins_UNKNOWN   }

    ,{"  PushB[0]  ",  Ins_PUSHB     }
    ,{"  PushB[1]  ",  Ins_PUSHB     }
    ,{"  PushB[2]  ",  Ins_PUSHB     }
    ,{"  PushB[3]  ",  Ins_PUSHB     }
    ,{"  PushB[4]  ",  Ins_PUSHB     }
    ,{"  PushB[5]  ",  Ins_PUSHB     }
    ,{"  PushB[6]  ",  Ins_PUSHB     }
    ,{"  PushB[7]  ",  Ins_PUSHB     }
    ,{"  PushW[0]  ",  Ins_PUSHW     }
    ,{"  PushW[1]  ",  Ins_PUSHW     }
    ,{"  PushW[2]  ",  Ins_PUSHW     }
    ,{"  PushW[3]  ",  Ins_PUSHW     }
    ,{"  PushW[4]  ",  Ins_PUSHW     }
    ,{"  PushW[5]  ",  Ins_PUSHW     }
    ,{"  PushW[6]  ",  Ins_PUSHW     }
    ,{"  PushW[7]  ",  Ins_PUSHW     }

    ,{"  MDRP[00]  ",  Ins_MDRP       }
    ,{"  MDRP[01]  ",  Ins_MDRP       }
    ,{"  MDRP[02]  ",  Ins_MDRP       }
    ,{"  MDRP[03]  ",  Ins_MDRP       }
    ,{"  MDRP[04]  ",  Ins_MDRP       }
    ,{"  MDRP[05]  ",  Ins_MDRP       }
    ,{"  MDRP[06]  ",  Ins_MDRP       }
    ,{"  MDRP[07]  ",  Ins_MDRP       }
    ,{"  MDRP[08]  ",  Ins_MDRP       }
    ,{"  MDRP[09]  ",  Ins_MDRP       }
    ,{"  MDRP[10]  ",  Ins_MDRP       }
    ,{"  MDRP[11]  ",  Ins_MDRP       }
    ,{"  MDRP[12]  ",  Ins_MDRP       }
    ,{"  MDRP[13]  ",  Ins_MDRP       }
    ,{"  MDRP[14]  ",  Ins_MDRP       }
    ,{"  MDRP[15]  ",  Ins_MDRP       }

    ,{"  MDRP[16]  ",  Ins_MDRP       }
    ,{"  MDRP[17]  ",  Ins_MDRP       }
    ,{"  MDRP[18]  ",  Ins_MDRP       }
    ,{"  MDRP[19]  ",  Ins_MDRP       }
    ,{"  MDRP[20]  ",  Ins_MDRP       }
    ,{"  MDRP[21]  ",  Ins_MDRP       }
    ,{"  MDRP[22]  ",  Ins_MDRP       }
    ,{"  MDRP[23]  ",  Ins_MDRP       }
    ,{"  MDRP[24]  ",  Ins_MDRP       }
    ,{"  MDRP[25]  ",  Ins_MDRP       }
    ,{"  MDRP[26]  ",  Ins_MDRP       }
    ,{"  MDRP[27]  ",  Ins_MDRP       }
    ,{"  MDRP[28]  ",  Ins_MDRP       }
    ,{"  MDRP[29]  ",  Ins_MDRP       }
    ,{"  MDRP[30]  ",  Ins_MDRP       }
    ,{"  MDRP[31]  ",  Ins_MDRP       }

    ,{"  MIRP[00]  ",  Ins_MIRP       }
    ,{"  MIRP[01]  ",  Ins_MIRP       }
    ,{"  MIRP[02]  ",  Ins_MIRP       }
    ,{"  MIRP[03]  ",  Ins_MIRP       }
    ,{"  MIRP[04]  ",  Ins_MIRP       }
    ,{"  MIRP[05]  ",  Ins_MIRP       }
    ,{"  MIRP[06]  ",  Ins_MIRP       }
    ,{"  MIRP[07]  ",  Ins_MIRP       }
    ,{"  MIRP[08]  ",  Ins_MIRP       }
    ,{"  MIRP[09]  ",  Ins_MIRP       }
    ,{"  MIRP[10]  ",  Ins_MIRP       }
    ,{"  MIRP[11]  ",  Ins_MIRP       }
    ,{"  MIRP[12]  ",  Ins_MIRP       }
    ,{"  MIRP[13]  ",  Ins_MIRP       }
    ,{"  MIRP[14]  ",  Ins_MIRP       }
    ,{"  MIRP[15]  ",  Ins_MIRP       }

    ,{"  MIRP[16]  ",  Ins_MIRP       }
    ,{"  MIRP[17]  ",  Ins_MIRP       }
    ,{"  MIRP[18]  ",  Ins_MIRP       }
    ,{"  MIRP[19]  ",  Ins_MIRP       }
    ,{"  MIRP[20]  ",  Ins_MIRP       }
    ,{"  MIRP[21]  ",  Ins_MIRP       }
    ,{"  MIRP[22]  ",  Ins_MIRP       }
    ,{"  MIRP[23]  ",  Ins_MIRP       }
    ,{"  MIRP[24]  ",  Ins_MIRP       }
    ,{"  MIRP[25]  ",  Ins_MIRP       }
    ,{"  MIRP[26]  ",  Ins_MIRP       }
    ,{"  MIRP[27]  ",  Ins_MIRP       }
    ,{"  MIRP[28]  ",  Ins_MIRP       }
    ,{"  MIRP[29]  ",  Ins_MIRP       }
    ,{"  MIRP[30]  ",  Ins_MIRP       }
    ,{"  MIRP[31]  ",  Ins_MIRP       }
  };



/****************************************************************/
/*                                                              */
/*                    RUN                                       */
/*                                                              */
/*  This function executes a run of opcodes.  It will exit      */
/*  in the following cases:                                     */
/*                                                              */
/*   - Errors (in which case it returns FALSE)                  */
/*                                                              */
/*   - Reaching the end of the main code range (returns TRUE).  */
/*     Reaching the end of a code range within a function       */
/*     call is an error.                                        */
/*                                                              */
/*   - After executing one single opcode, if the flag           */
/*     'Instruction_Trap' is set to TRUE (returns TRUE).        */
/*                                                              */
/*  On exit whith TRUE, test IP < CodeSize to know wether it    */
/*  comes from a instruction trap or a normal termination.      */
/*                                                              */
/*                                                              */
/*     Note:  The documented DEBUG opcode pops a value from     */
/*            the stack.  This behaviour is unsupported, here   */
/*            a DEBUG opcode is always an error.                */
/*                                                              */
/*                                                              */
/* THIS IS THE INTERPRETER'S MAIN LOOP                          */
/*                                                              */
/*  Instructions appear in the specs' order.                    */
/*                                                              */
/****************************************************************/

  TT_Error  RunIns( PExecution_Context  exc )
  {
    TT_Error     Result;
    Int          A;
    PDefRecord   WITH;
    PCallRecord  WITH1;
    bool bFirst;
    bool dbg_prt = (DBG_PRT_FUN != NULL);
#   ifdef DEBUG
	ttfMemory *mem = exc->current_face->font->tti->ttf_memory;
	F26Dot6 *save_ox, *save_oy, *save_cx, *save_cy;

	DBG_PRINT("\n%% *** Entering RunIns ***");
#   endif

    /* set CVT functions */
    CUR.metrics.ratio = 0;
    if ( CUR.metrics.x_ppem != CUR.metrics.y_ppem )
    {
      /* non-square pixels, use the stretched routines */
      CUR.func_read_cvt  = Read_CVT_Stretched;
      CUR.func_write_cvt = Write_CVT_Stretched;
      CUR.func_move_cvt  = Move_CVT_Stretched;
    }
    else
    {
      /* square pixels, use normal routines */
      CUR.func_read_cvt  = Read_CVT;
      CUR.func_write_cvt = Write_CVT;
      CUR.func_move_cvt  = Move_CVT;
    }

    COMPUTE_Funcs();
    Compute_Round( EXEC_ARGS (Byte)exc->GS.round_state );

#   ifdef DEBUG
      if (dbg_prt && CUR.pts.n_points) {
        save_ox = mem->alloc_bytes(mem, CUR.pts.n_points * sizeof(*save_ox), "RunIns");
        save_oy = mem->alloc_bytes(mem, CUR.pts.n_points * sizeof(*save_oy), "RunIns");
        save_cx = mem->alloc_bytes(mem, CUR.pts.n_points * sizeof(*save_cx), "RunIns");
        save_cy = mem->alloc_bytes(mem, CUR.pts.n_points * sizeof(*save_cy), "RunIns");
        if (!save_ox || !save_oy || !save_cx || !save_cy)
	  return TT_Err_Out_Of_Memory;
      } else
	save_ox = save_oy = save_cx = save_cy = NULL;
#   endif

    Result = setjmp(exc->trap);
    if (Result) {
	CUR.error = Result;
	goto _LExit;
    }
    bFirst = true;

    do
    {
      CALC_Length();

      /* First, let's check for empty stack and overflow */

      CUR.args = CUR.top - Pop_Push_Count[CUR.opcode * 2];

      /* `args' is the top of the stack once arguments have been popped. */
      /* One can also interpret it as the index of the last argument.    */

      if ( CUR.args < 0 )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        goto _LErrorLabel;
      }

      CUR.new_top = CUR.args + Pop_Push_Count[CUR.opcode * 2 + 1];

      /* `new_top' is the new top of the stack, after the instruction's */
      /* execution.  `top' will be set to `new_top' after the 'switch'  */
      /* statement.                                                     */

      if ( CUR.new_top > CUR.stackSize )
      {
        CUR.error = TT_Err_Stack_Overflow;
        goto _LErrorLabel;
      }

      CUR.step_ins = TRUE;
      CUR.error    = TT_Err_Ok;

#     ifdef DEBUG
        DBG_PRINT3("\n%%n=%5d IP=%5d OP=%s            ", nInstrCount, CUR.IP, Instruct_Dispatch[CUR.opcode].sName);
	/*
        { for(int i=0;i<CUR.top;i++)
            DBG_PRINT1("% %d",CUR.stack[i]);
        }
	*/
	if (save_ox != NULL) {
          memcpy(save_ox, CUR.pts.org_x, sizeof(CUR.pts.org_x[0]) * CUR.pts.n_points);
          memcpy(save_oy, CUR.pts.org_y, sizeof(CUR.pts.org_y[0]) * CUR.pts.n_points);
          memcpy(save_cx, CUR.pts.cur_x, sizeof(CUR.pts.cur_x[0]) * CUR.pts.n_points);
          memcpy(save_cy, CUR.pts.cur_y, sizeof(CUR.pts.cur_y[0]) * CUR.pts.n_points);
	}
#     endif

      Instruct_Dispatch[CUR.opcode].p( EXEC_ARGS &CUR.stack[CUR.args] );

#     ifdef DEBUG
      if (save_ox != NULL) { 
	F26Dot6 *pp[4], *qq[4];
        const char *ss[] = {"org.x", "org.y", "cur.x", "cur.y"};
        int l = 0, i, j;

        pp[0] = save_ox, 
        pp[1] = save_oy, 
        pp[2] = save_cx, 
        pp[3] = save_cy;
        qq[0] = CUR.pts.org_x;
        qq[1] = CUR.pts.org_y;
        qq[2] = CUR.pts.cur_x;
        qq[3] = CUR.pts.cur_y;

        for(i = 0; i < 4; i++)
          for(j = 0;j < CUR.pts.n_points; j++)
            { F26Dot6 *ppi = pp[i], *qqi = qq[i];
              if(ppi[j] != qqi[j] || bFirst)
	      {
                DBG_PRINT4("%%  %s[%d]%d:=%d", ss[i], j, pp[i][j], qq[i][j]);
                if(++l > 3)
                  { l=0;
                    DBG_PRINT("\n");
                  }
              }	       
            }
        nInstrCount++;
        bFirst=FALSE;
      }
#     endif

      DBG_PAINT

      if ( CUR.error != TT_Err_Ok )
      {
        switch ( CUR.error )
        {
        case TT_Err_Invalid_Opcode: /* looking for redefined instructions */
          A = 0;

          while ( A < CUR.numIDefs )
          {
            WITH = &CUR.IDefs[A];

            if ( WITH->Active && CUR.opcode == WITH->Opc )
            {
              if ( CUR.callTop >= CUR.callSize )
              {
                CUR.error = TT_Err_Invalid_Reference;
                goto _LErrorLabel;
              }

              WITH1 = &CUR.callStack[CUR.callTop];

              WITH1->Caller_Range = CUR.curRange;
              WITH1->Caller_IP    = CUR.IP + 1;
              WITH1->Cur_Count    = 1;
              WITH1->Cur_Restart  = WITH->Start;

              if ( INS_Goto_CodeRange( WITH->Range, WITH->Start ) == FAILURE )
                goto _LErrorLabel;

              goto _LSuiteLabel;
            }
            else
            {
              A++;
              continue;
            }
          }

          CUR.error = TT_Err_Invalid_Opcode;
          goto _LErrorLabel;
          break;

        default:
          CUR.error = CUR.error;
          goto _LErrorLabel;
          break;
        }
      }

      CUR.top = CUR.new_top;

      if ( CUR.step_ins )
        CUR.IP += CUR.length;

  _LSuiteLabel:

      if ( CUR.IP >= CUR.codeSize )
      {
        if ( CUR.callTop > 0 )
        {
          CUR.error = TT_Err_Code_Overflow;
          goto _LErrorLabel;
        }
        else
          goto _LNo_Error;
      }
    } while ( !CUR.instruction_trap );

  _LNo_Error:
    Result = TT_Err_Ok;
    goto _LExit;

  _LErrorLabel:
    Result = CUR.error;
    DBG_PRINT1("%%  ERROR=%d", Result);
  _LExit:
#   ifdef DEBUG
    if (save_ox != NULL) {
      mem->free(mem, save_ox, "RunIns");
      mem->free(mem, save_oy, "RunIns");
      mem->free(mem, save_cx, "RunIns");
      mem->free(mem, save_cy, "RunIns");
    }
#   endif

    return Result;
  }
