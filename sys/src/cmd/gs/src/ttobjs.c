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

/* $Id: ttobjs.c,v 1.9 2004/12/21 20:13:41 igor Exp $ */

/* Changes after FreeType: cut out the TrueType instruction interpreter. */

/*******************************************************************
 *
 *  ttobjs.c                                                     1.0
 *
 *    Objects manager.        
 *
 *  Copyright 1996-1998 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT.  By continuing to use, modify, or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 ******************************************************************/

#include "ttmisc.h"

#include "ttfoutl.h"
#include "ttobjs.h"
#include "ttcalc.h"
#include "ttload.h"
#include "ttinterp.h"

/* Add extensions definition */
#ifdef TT_EXTEND_ENGINE
#endif



/*******************************************************************
 *                                                                 *
 *                     CODERANGE FUNCTIONS                         *
 *                                                                 *
 *                                                                 *
 *******************************************************************/

/*******************************************************************
 *
 *  Function    :  Goto_CodeRange
 *
 *  Description :  Switch to a new code range (updates Code and IP).
 *
 *  Input  :  exec    target execution context
 *            range   new execution code range       
 *            IP      new IP in new code range       
 *
 *  Output :  SUCCESS on success.  FAILURE on error (no code range).
 *
 *****************************************************************/

  TT_Error  Goto_CodeRange( PExecution_Context  exec, Int  range, Int  IP )
  {
    PCodeRange  cr;


    if ( range < 1 || range > 3 )
      return TT_Err_Bad_Argument;

    cr = &exec->codeRangeTable[range - 1];

    if ( cr->Base == NULL )
      return TT_Err_Invalid_CodeRange;

    /* NOTE:  Because the last instruction of a program may be a CALL */
    /*        which will return to the first byte *after* the code    */
    /*        range, we test for IP <= Size, instead of IP < Size.    */

    if ( IP > cr->Size )
      return TT_Err_Code_Overflow;

    exec->code     = cr->Base;
    exec->codeSize = cr->Size;
    exec->IP       = IP;
    exec->curRange = range;

    return TT_Err_Ok;
  }

/*******************************************************************
 *
 *  Function    :  Unset_CodeRange
 *
 *  Description :  Unsets the code range pointer.
 *
 *  Input  :  exec    target execution context
 *
 *  Output :  
 *
 *  Note   : The pointer must be unset after used to avoid pending pointers
 *           while a garbager invokation.
 *
 *****************************************************************/

  void  Unset_CodeRange( PExecution_Context  exec )
  {
    exec->code = 0;
    exec->codeSize = 0;
  }
  
/*******************************************************************
 *
 *  Function    :  Get_CodeRange
 *
 *  Description :  Returns a pointer to a given code range.  Should
 *                 be used only by the debugger.  Returns NULL if
 *                 'range' is out of current bounds.
 *
 *  Input  :  exec    target execution context
 *            range   new execution code range       
 *
 *  Output :  Pointer to the code range record.  NULL on failure.
 *
 *****************************************************************/

  PCodeRange  Get_CodeRange( PExecution_Context  exec, Int  range )
  {
    if ( range < 1 || range > 3 )
      return (PCodeRange)NULL;
    else                /* arrays start with 1 in Pascal, and with 0 in C */
      return &exec->codeRangeTable[range - 1];
  }


/*******************************************************************
 *
 *  Function    :  Set_CodeRange
 *
 *  Description :  Sets a code range.
 *
 *  Input  :  exec    target execution context
 *            range   code range index
 *            base    new code base
 *            length  sange size in bytes
 *
 *  Output :  SUCCESS on success.  FAILURE on error.
 *
 *****************************************************************/

  TT_Error  Set_CodeRange( PExecution_Context  exec,
                           Int                 range,
                           void*               base,
                           Int                 length )
  {
    if ( range < 1 || range > 3 )
      return TT_Err_Bad_Argument;

    exec->codeRangeTable[range - 1].Base = (unsigned char*)base;
    exec->codeRangeTable[range - 1].Size = length;

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Clear_CodeRange
 *
 *  Description :  Clears a code range.
 *
 *  Input  :  exec    target execution context
 *            range   code range index
 *
 *  Output :  SUCCESS on success.  FAILURE on error.
 *
 *  Note   : Does not set the Error variable.
 *
 *****************************************************************/

  TT_Error Clear_CodeRange( PExecution_Context  exec, Int  range )
  {
    if ( range < 1 || range > 3 )
      return TT_Err_Bad_Argument;

    exec->codeRangeTable[range - 1].Base = (Byte*)NULL;
    exec->codeRangeTable[range - 1].Size = 0;

    return TT_Err_Ok;
  }



/*******************************************************************
 *                                                                 *
 *                EXECUTION CONTEXT ROUTINES                       *
 *                                                                 *
 *                                                                 *
 *******************************************************************/


#define FREE(ptr) { mem->free(mem, ptr, "ttobjs.c"); ptr = NULL; }
#define ALLOC_ARRAY(ptr, old_count, count, type) \
	(old_count >= count ? 0 : \
	  !(free_aux(mem, ptr),   \
	    ptr = mem->alloc_bytes(mem, (count) * sizeof(type), "ttobjs.c"))) 
#define SETMAX(a, b) a = (a > b ? a : b)

static int free_aux(ttfMemory *mem, void *ptr)
{
    mem->free(mem, ptr, "ttobjs.c");
    return 0;
}

/*******************************************************************
 *
 *  Function    :  Context_Destroy
 *
 *****************************************************************/

 TT_Error  Context_Destroy( void*  _context )
 {
   PExecution_Context  exec = (PExecution_Context)_context;
   ttfMemory *mem;

   if ( !exec )
     return TT_Err_Ok;
   if ( !exec->current_face ) {
     /* This may happen while closing a high level device, when allocator runs out of memory. 
        A test case is 01_001.pdf with pdfwrite and a small vmthreshold.
     */
     return TT_Err_Out_Of_Memory;
   }
   if (--exec->lock)
     return TT_Err_Ok; /* Still in use */

   mem = exec->current_face->font->tti->ttf_memory;

   /* points zone */
   FREE( exec->pts.cur_y );
   FREE( exec->pts.cur_x );
   FREE( exec->pts.org_y );
   FREE( exec->pts.org_x );
   FREE( exec->pts.touch );
   FREE( exec->pts.contours );
   exec->pts.n_points   = 0;
   exec->pts.n_contours = 0;

   /* twilight zone */
   FREE( exec->twilight.touch );
   FREE( exec->twilight.cur_y );
   FREE( exec->twilight.cur_x );
   FREE( exec->twilight.org_y );
   FREE( exec->twilight.org_x );
   FREE( exec->twilight.contours );
   exec->twilight.n_points   = 0;
   exec->twilight.n_contours = 0;

   /* free stack */
   FREE( exec->stack );
   exec->stackSize = 0;

   /* free call stack */
   FREE( exec->callStack );
   exec->callSize = 0;
   exec->callTop  = 0;

   /* free glyph code range */
   exec->glyphSize = 0;
   exec->maxGlyphSize = 0;

   exec->current_face    = (PFace)NULL;

   return TT_Err_Ok;
 }

/*******************************************************************
 *
 *  Function    :  Context_Create
 *
 *****************************************************************/

 TT_Error  Context_Create( void*  _context, void*  _face )
 {
   /* Note : The function name is a kind of misleading due to our improvement.
    * Now it adjusts (enhances) the context for the specified face.
    * We keep the old Free Type's name for easier localization of our changes.
    * The context must be initialized with zeros before the first call.
    * (igorm).
    */
   PExecution_Context  exec = (PExecution_Context)_context;

   PFace        face = (PFace)_face;
   ttfMemory   *mem = face->font->tti->ttf_memory;
   TMaxProfile *maxp = &face->maxProfile;
   Int          n_points, n_twilight;
   Int          callSize, stackSize;

   callSize  = 32;

   /* reserve a little extra for broken fonts like courbs or timesbs */
   stackSize = maxp->maxStackElements + 32;

   n_points        = face->maxPoints + 2;
   n_twilight      = maxp->maxTwilightPoints;

   if ( ALLOC_ARRAY( exec->callStack, exec->callSize, callSize, TCallRecord ) ||
        /* reserve interpreter call stack */

        ALLOC_ARRAY( exec->stack, exec->stackSize, stackSize, Long )           ||
        /* reserve interpreter stack */

        ALLOC_ARRAY( exec->pts.org_x, exec->n_points, n_points, TT_F26Dot6 )        ||
        ALLOC_ARRAY( exec->pts.org_y, exec->n_points, n_points, TT_F26Dot6 )        ||
        ALLOC_ARRAY( exec->pts.cur_x, exec->n_points, n_points, TT_F26Dot6 )        ||
        ALLOC_ARRAY( exec->pts.cur_y, exec->n_points, n_points, TT_F26Dot6 )        ||
        ALLOC_ARRAY( exec->pts.touch, exec->n_points, n_points, Byte )                          ||
        /* reserve points zone */

        ALLOC_ARRAY( exec->twilight.org_x, exec->twilight.n_points, n_twilight, TT_F26Dot6 ) ||
        ALLOC_ARRAY( exec->twilight.org_y, exec->twilight.n_points, n_twilight, TT_F26Dot6 ) ||
        ALLOC_ARRAY( exec->twilight.cur_x, exec->twilight.n_points, n_twilight, TT_F26Dot6 ) ||
        ALLOC_ARRAY( exec->twilight.cur_y, exec->twilight.n_points, n_twilight, TT_F26Dot6 ) ||
        ALLOC_ARRAY( exec->twilight.touch, exec->twilight.n_points, n_twilight, Byte )                   ||
        /* reserve twilight zone */

        ALLOC_ARRAY( exec->pts.contours, exec->n_contours, face->maxContours, UShort ) 
        /* reserve contours array */
      )
     goto Fail_Memory;

     SETMAX(exec->callSize, callSize);
     SETMAX(exec->stackSize, stackSize);
     SETMAX(exec->twilight.n_points, n_twilight);
     SETMAX(exec->maxGlyphSize, maxp->maxSizeOfInstructions);
     SETMAX(exec->n_contours, face->maxContours);
     SETMAX(exec->n_points, n_points);
     exec->lock++;

     return TT_Err_Ok;

  Fail_Memory:
    /* Context_Destroy( exec ); Don't release buffers because the context is shared. */
    return TT_Err_Out_Of_Memory;
 }


/*******************************************************************
 *
 *  Function    :  Context_Load
 *
 *****************************************************************/

 TT_Error Context_Load( PExecution_Context  exec,
                        PInstance           ins )
 {
   Int  i;


   exec->current_face = ins->face;

   exec->numFDefs = ins->numFDefs;
   exec->numIDefs = ins->numIDefs;
   exec->FDefs    = ins->FDefs;
   exec->IDefs    = ins->IDefs;
   exec->countIDefs = ins->countIDefs;
   memcpy(exec->IDefPtr, ins->IDefPtr, sizeof(exec->IDefPtr));

   exec->metrics  = ins->metrics;

   for ( i = 0; i < MAX_CODE_RANGES; i++ )
     exec->codeRangeTable[i] = ins->codeRangeTable[i];

   exec->pts.n_points   = 0;
   exec->pts.n_contours = 0;

   exec->instruction_trap = FALSE;

   /* set default graphics state */
   exec->GS = ins->GS;

   exec->cvtSize = ins->cvtSize;
   exec->cvt     = ins->cvt;

   exec->storeSize = ins->storeSize;
   exec->storage   = ins->storage;

   return TT_Err_Ok;
 }


/*******************************************************************
 *
 *  Function    :  Context_Save
 *
 *****************************************************************/

 TT_Error  Context_Save( PExecution_Context  exec,
                         PInstance           ins )
 {
   Int  i;

   for ( i = 0; i < MAX_CODE_RANGES; i++ ) {
     ins->codeRangeTable[i] = exec->codeRangeTable[i];
     exec->codeRangeTable[i].Base = 0;
     exec->codeRangeTable[i].Size = 0;
   }
   exec->numFDefs = 0;
   exec->numIDefs = 0;
   memcpy(ins->IDefPtr, exec->IDefPtr, sizeof(ins->IDefPtr));
   ins->countIDefs = exec->countIDefs;
   exec->countIDefs = 0;
   exec->FDefs    = 0;
   exec->IDefs    = 0;
   exec->cvtSize = 0;
   exec->cvt     = 0;
   exec->storeSize = 0;
   exec->storage   = 0;
   exec->current_face = 0;

   return TT_Err_Ok;
 }


/*******************************************************************
 *
 *  Function    :  Context_Run
 *
 *****************************************************************/

 TT_Error  Context_Run( PExecution_Context  exec,
                        Bool                debug )
 {
   TT_Error  error;


   if ( ( error = Goto_CodeRange( exec, TT_CodeRange_Glyph, 0 ) ) )
     return error;

   exec->zp0 = exec->pts;
   exec->zp1 = exec->pts;
   exec->zp2 = exec->pts;

   exec->GS.gep0 = 1;
   exec->GS.gep1 = 1;
   exec->GS.gep2 = 1;

   exec->GS.projVector.x = 0x4000;
   exec->GS.projVector.y = 0x0000;

   exec->GS.freeVector = exec->GS.projVector;
   exec->GS.dualVector = exec->GS.projVector;

   exec->GS.round_state = 1;
   exec->GS.loop        = 1;

   /* some glyphs leave something on the stack. so we clean it */
   /* before a new execution.                                  */
   exec->top     = 0;
   exec->callTop = 0;

   if ( !debug ) {
     error = RunIns( exec );
     Unset_CodeRange(exec);    
     return error;
   } else
     return TT_Err_Ok;
 }


  const TGraphicsState  Default_GraphicsState =
  {
    0, 0, 0, 
    { 0x4000, 0 },
    { 0x4000, 0 },
    { 0x4000, 0 },
    1, 64, 1,
    TRUE, 68, 0, 0, 9, 3,
    0, FALSE, 2, 1, 1, 1
  };



/*******************************************************************
 *                                                                 *
 *                     INSTANCE  FUNCTIONS                         *
 *                                                                 *
 *                                                                 *
 *******************************************************************/

/*******************************************************************
 *
 *  Function    : Instance_Destroy
 *
 *  Description :
 *
 *  Input  :  _instance   the instance object to destroy
 *
 *  Output :  error code.
 *
 ******************************************************************/

  TT_Error  Instance_Destroy( void* _instance )
  {
    PInstance  ins = (PInstance)_instance;
    ttfMemory *mem;

    if ( !_instance )
      return TT_Err_Ok;
    if ( !ins->face ) {
      /* This may happen while closing a high level device, when allocator runs out of memory. 
         A test case is 01_001.pdf with pdfwrite and a small vmthreshold.
      */
      return TT_Err_Out_Of_Memory;
    }
    mem = ins->face->font->tti->ttf_memory;

    FREE( ins->cvt );
    ins->cvtSize = 0;

    FREE( ins->FDefs );
    FREE( ins->IDefs );
    FREE( ins->storage );
    ins->numFDefs = 0;
    ins->numIDefs = 0;

    ins->face = (PFace)NULL;
    ins->valid = FALSE;

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    : Instance_Create
 *
 *  Description :
 *
 *  Input  :  _instance    instance record to initialize
 *            _face        parent face object
 *
 *  Output :  Error code.  All partially built subtables are
 *            released on error.
 *
 ******************************************************************/

  TT_Error  Instance_Create( void*  _instance,
                             void*  _face )
  {
    PInstance ins  = (PInstance)_instance;
    PFace     face = (PFace)_face;
    ttfMemory *mem = face->font->tti->ttf_memory;
    PMaxProfile  maxp = &face->maxProfile;
    Int       i;

    ins->FDefs=NULL;
    ins->IDefs=NULL;
    ins->cvt=NULL;
    ins->storage=NULL;

    ins->face = face;
    ins->valid = FALSE;

    ins->numFDefs = maxp->maxFunctionDefs;
    ins->numIDefs = maxp->maxInstructionDefs;
    ins->countIDefs = 0;
    if (maxp->maxInstructionDefs > 255)
	goto Fail_Memory;
    memset(ins->IDefPtr, (Byte)ins->numIDefs, sizeof(ins->IDefPtr));
    if (ins->numFDefs < 50)
	ins->numFDefs = 50; /* Bug 687858 */
    ins->cvtSize  = face->cvtSize;

    ins->metrics.pointSize    = 10 * 64;     /* default pointsize  = 10pts */

    ins->metrics.x_resolution = 96;          /* default resolution = 96dpi */
    ins->metrics.y_resolution = 96;

    ins->metrics.x_ppem = 0;
    ins->metrics.y_ppem = 0;

    ins->metrics.rotated   = FALSE;
    ins->metrics.stretched = FALSE;

    ins->storeSize = maxp->maxStorage;

    for ( i = 0; i < 4; i++ )
      ins->metrics.compensations[i] = 0;     /* Default compensations */

    if ( ALLOC_ARRAY( ins->FDefs, 0, ins->numFDefs, TDefRecord )  ||
         ALLOC_ARRAY( ins->IDefs, 0, ins->numIDefs, TDefRecord )  ||
         ALLOC_ARRAY( ins->cvt, 0, ins->cvtSize, Long )           ||
         ALLOC_ARRAY( ins->storage, 0, ins->storeSize, Long )     )
      goto Fail_Memory;

    memset (ins->FDefs, 0, ins->numFDefs * sizeof(TDefRecord));
    memset (ins->IDefs, 0, ins->numIDefs * sizeof(TDefRecord));

    ins->GS = Default_GraphicsState;

    return TT_Err_Ok;

  Fail_Memory:
    Instance_Destroy( ins );
    return TT_Err_Out_Of_Memory;
  }

/*******************************************************************
 *
 *  Function    : Instance_Init
 *
 *  Description : Initialize a fresh new instance.
 *                Executes the font program if any is found.
 *
 *  Input  :  _instance   the instance object to destroy
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Instance_Init( PInstance  ins )
  {
    PExecution_Context  exec;

    TT_Error  error;
    PFace     face = ins->face;

    exec = ins->face->font->exec;
    /* debugging instances have their own context */

    ins->GS = Default_GraphicsState;

    Context_Load( exec, ins );

    exec->callTop   = 0;
    exec->top       = 0;

    exec->period    = 64;
    exec->phase     = 0;
    exec->threshold = 0;

    exec->metrics.x_ppem    = 0;
    exec->metrics.y_ppem    = 0;
    exec->metrics.pointSize = 0;
    exec->metrics.x_scale1  = 0;
    exec->metrics.x_scale2  = 1;
    exec->metrics.y_scale1  = 0;
    exec->metrics.y_scale2  = 1;

    exec->metrics.ppem      = 0;
    exec->metrics.scale1    = 0;
    exec->metrics.scale2    = 1;
    exec->metrics.ratio     = 1 << 16;

    exec->instruction_trap = FALSE;

    exec->cvtSize = ins->cvtSize;
    exec->cvt     = ins->cvt;

    exec->F_dot_P = 0x10000;

    /* allow font program execution */
    Set_CodeRange( exec,
                   TT_CodeRange_Font,
                   face->fontProgram,
                   face->fontPgmSize );

    /* disable CVT and glyph programs coderange */
    Clear_CodeRange( exec, TT_CodeRange_Cvt );
    Clear_CodeRange( exec, TT_CodeRange_Glyph );

    if ( face->fontPgmSize > 0 )
    {
      error = Goto_CodeRange( exec, TT_CodeRange_Font, 0 );
      if ( error )
        goto Fin;

      error = RunIns( exec );
      Unset_CodeRange(exec);    
    }
    else
      error = TT_Err_Ok;

  Fin:
    Context_Save( exec, ins );

    ins->valid = FALSE;

    return error;
  }


/*******************************************************************
 *
 *  Function    : Instance_Reset
 *
 *  Description : Resets an instance to a new pointsize/transform.
 *                Executes the cvt program if any is found.
 *
 *  Input  :  _instance   the instance object to destroy
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Instance_Reset( PInstance  ins,
                            Bool       debug )
  {
    TT_Error  error;
    Int       i;
    PFace     face = ins->face;
    PExecution_Context exec = ins->face->font->exec;

    if ( !ins )
      return TT_Err_Invalid_Instance_Handle;

    if ( ins->valid )
      return TT_Err_Ok;

    if ( ins->metrics.x_ppem < 1 ||
         ins->metrics.y_ppem < 1 )
      return TT_Err_Invalid_PPem;

    /* compute new transformation */
    if ( ins->metrics.x_ppem >= ins->metrics.y_ppem )
    {
      ins->metrics.scale1  = ins->metrics.x_scale1;
      ins->metrics.scale2  = ins->metrics.x_scale2;
      ins->metrics.ppem    = ins->metrics.x_ppem;
      ins->metrics.x_ratio = 1 << 16;
      ins->metrics.y_ratio = MulDiv_Round( ins->metrics.y_ppem,
                                           0x10000,
                                           ins->metrics.x_ppem );
    }
    else
    {
      ins->metrics.scale1  = ins->metrics.y_scale1;
      ins->metrics.scale2  = ins->metrics.y_scale2;
      ins->metrics.ppem    = ins->metrics.y_ppem;
      ins->metrics.x_ratio = MulDiv_Round( ins->metrics.x_ppem,
                                           0x10000,
                                           ins->metrics.y_ppem );
      ins->metrics.y_ratio = 1 << 16;
    }

    /* Scale the cvt values to the new ppem.          */
    /* We use by default the y ppem to scale the CVT. */

    for ( i = 0; i < ins->cvtSize; i++ )
      ins->cvt[i] = MulDiv_Round( face->cvt[i],
                                  ins->metrics.scale1,
                                  ins->metrics.scale2 );

    ins->GS = Default_GraphicsState;

    /* get execution context and run prep program */

    Context_Load( exec, ins );

    Set_CodeRange( exec,
                   TT_CodeRange_Cvt,
                   face->cvtProgram,
                   face->cvtPgmSize );

    Clear_CodeRange( exec, TT_CodeRange_Glyph );

    for ( i = 0; i < exec->storeSize; i++ )
      exec->storage[i] = 0;

    exec->instruction_trap = FALSE;

    exec->top     = 0;
    exec->callTop = 0;

    /* All twilight points are originally zero */

    for ( i = 0; i < exec->twilight.n_points; i++ )
    {
      exec->twilight.org_x[i] = 0;
      exec->twilight.org_y[i] = 0;
      exec->twilight.cur_x[i] = 0;
      exec->twilight.cur_y[i] = 0;
    }

    if ( face->cvtPgmSize > 0 )
    {
      error = Goto_CodeRange( exec, TT_CodeRange_Cvt, 0 );
      if (error)
        goto Fin;

      error = RunIns( exec );
      Unset_CodeRange(exec);    
    }
    else
      error = TT_Err_Ok;

    ins->GS = exec->GS;
    /* save default graphics state */

  Fin:
    Context_Save( exec, ins );

    if ( !error )
      ins->valid = TRUE;

    return error;
  }


/*******************************************************************
 *                                                                 *
 *                         FACE  FUNCTIONS                         *
 *                                                                 *
 *                                                                 *
 *******************************************************************/

/*******************************************************************
 *
 *  Function    :  Face_Destroy
 *
 *  Description :  The face object destructor.
 *
 *  Input  :  _face   typeless pointer to the face object to destroy
 *
 *  Output :  Error code.                       
 *
 ******************************************************************/

  TT_Error  Face_Destroy( PFace face )
  {
    ttfMemory *mem = face->font->tti->ttf_memory;
    if ( !face )
      return TT_Err_Ok;

    /* freeing the CVT */
    FREE( face->cvt );
    face->cvtSize = 0;

    /* freeing the programs */
    FREE( face->fontProgram );
    FREE( face->cvtProgram );
    face->fontPgmSize = 0;
    face->cvtPgmSize  = 0;

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Face_Create
 *
 *  Description :  The face object constructor.
 *
 *  Input  :  _face    face record to build
 *            _input   input stream where to load font data
 *
 *  Output :  Error code.
 *
 *  NOTE : The input stream is kept in the face object.  The
 *         caller shouldn't destroy it after calling Face_Create().
 *
 ******************************************************************/

#define LOAD_( table ) \
          ( error = Load_TrueType_##table (face) )


  TT_Error  Face_Create( PFace  face)
  {
    TT_Error      error;

    /* Load tables */

    if ( /*LOAD_(Header)                      ||*/
         LOAD_(MaxProfile)                  ||
         /*LOAD_(Locations)                   ||*/
         /*LOAD_(CMap)                        ||*/
         LOAD_(CVT)                         ||
         /*LOAD_(Horizontal_Header)           ||*/
         LOAD_(Programs)                    
         /*LOAD_(HMTX)                        ||*/
         /*LOAD_(Gasp)                        ||*/
         /*LOAD_(Names)                       ||*/
         /*LOAD_(OS2)                         ||*/
         /*LOAD_(PostScript)                  ||*/
         /*LOAD_(Hdmx)                        */
        )
      goto Fail;

    return TT_Err_Ok;

  Fail :
    Face_Destroy( face );
    return error;
  }




/*******************************************************************
 *
 *  Function    :  Scale_X
 *
 *  Description :  scale an horizontal distance from font
 *                 units to 26.6 pixels
 *
 *  Input  :  metrics  pointer to metrics
 *            x        value to scale
 *
 *  Output :  scaled value
 *
 ******************************************************************/

 TT_Pos  Scale_X( PIns_Metrics  metrics, TT_Pos  x )
 {
   return MulDiv_Round( x, metrics->x_scale1, metrics->x_scale2 );
 }

/*******************************************************************
 *
 *  Function    :  Scale_Y
 *
 *  Description :  scale a vertical distance from font
 *                 units to 26.6 pixels
 *
 *  Input  :  metrics  pointer to metrics
 *            y        value to scale
 *
 *  Output :  scaled value
 *
 ******************************************************************/

 TT_Pos  Scale_Y( PIns_Metrics  metrics, TT_Pos  y )
 {
   return MulDiv_Round( y, metrics->y_scale1, metrics->y_scale2 );
 }

