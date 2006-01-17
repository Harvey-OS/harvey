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

/* $Id: ttfmemd.c,v 1.7 2004/06/24 10:10:17 igor Exp $ */

/* Memory structure descriptors for the TrueType instruction interpreter. */

#include "ttmisc.h"
#include "ttfoutl.h"
#include "ttobjs.h"
#include "ttfmemd.h"
#include "gsstruct.h"

gs_public_st_ptrs5(st_TFace, TFace, "TFace", 
    st_TFace_enum_ptrs, st_TFace_reloc_ptrs, 
    r, font, fontProgram, cvtProgram, cvt);

gs_public_st_composite(st_TInstance, TInstance,
    "TInstance", TInstance_enum_ptrs, TInstance_reloc_ptrs);

private 
ENUM_PTRS_BEGIN(TInstance_enum_ptrs) return 0;
    ENUM_PTR(0, TInstance, face);
    ENUM_PTR(1, TInstance, FDefs);
    ENUM_PTR(2, TInstance, IDefs);
    ENUM_PTR(3, TInstance, codeRangeTable[0].Base);
    ENUM_PTR(4, TInstance, codeRangeTable[1].Base);
    ENUM_PTR(5, TInstance, codeRangeTable[2].Base);
    ENUM_PTR(6, TInstance, cvt);
    ENUM_PTR(7, TInstance, storage);
ENUM_PTRS_END

private RELOC_PTRS_WITH(TInstance_reloc_ptrs, TInstance *mptr)
    RELOC_PTR(TInstance, face);
    RELOC_PTR(TInstance, FDefs);
    RELOC_PTR(TInstance, IDefs);
    RELOC_PTR(TInstance, codeRangeTable[0].Base);
    RELOC_PTR(TInstance, codeRangeTable[1].Base);
    RELOC_PTR(TInstance, codeRangeTable[2].Base);
    RELOC_PTR(TInstance, cvt);
    RELOC_PTR(TInstance, storage);
    DISCARD(mptr);
RELOC_PTRS_END

gs_public_st_composite(st_TExecution_Context, TExecution_Context,
    "TExecution_Context", TExecution_Context_enum_ptrs, TExecution_Context_reloc_ptrs);

private 
ENUM_PTRS_BEGIN(TExecution_Context_enum_ptrs) return 0;
    ENUM_PTR(0, TExecution_Context, current_face);
    /* ENUM_PTR(1, TExecution_Context, code); // local, no gc invocations */
    ENUM_PTR(1, TExecution_Context, FDefs);
    ENUM_PTR(2, TExecution_Context, IDefs);
    /* ENUM_PTR(4, TExecution_Context, glyphIns); // Never used. */
    ENUM_PTR(3, TExecution_Context, callStack);
    ENUM_PTR(4, TExecution_Context, codeRangeTable[0].Base);
    ENUM_PTR(5, TExecution_Context, codeRangeTable[1].Base);
    ENUM_PTR(6, TExecution_Context, codeRangeTable[2].Base);
    ENUM_PTR(7, TExecution_Context, storage);
    ENUM_PTR(8, TExecution_Context, stack);
    /* zp0, // local, no gc invocations */
    /* zp1, // local, no gc invocations */
    /* zp2, // local, no gc invocations */
    ENUM_PTR(9, TExecution_Context, pts.org_x);
    ENUM_PTR(10, TExecution_Context, pts.org_y);
    ENUM_PTR(11, TExecution_Context, pts.cur_x);
    ENUM_PTR(12, TExecution_Context, pts.cur_y);
    ENUM_PTR(13, TExecution_Context, pts.touch);
    ENUM_PTR(14, TExecution_Context, pts.contours);
    ENUM_PTR(15, TExecution_Context, twilight.org_x);
    ENUM_PTR(16, TExecution_Context, twilight.org_y);
    ENUM_PTR(17, TExecution_Context, twilight.cur_x);
    ENUM_PTR(18, TExecution_Context, twilight.cur_y);
    ENUM_PTR(19, TExecution_Context, twilight.touch);
    ENUM_PTR(20, TExecution_Context, twilight.contours);
    ENUM_PTR(21, TExecution_Context, cvt);
ENUM_PTRS_END

private RELOC_PTRS_WITH(TExecution_Context_reloc_ptrs, TExecution_Context *mptr)
{
    RELOC_PTR(TExecution_Context, current_face);
    /* RELOC_PTR(TExecution_Context, code);  // local, no gc invocations */
    RELOC_PTR(TExecution_Context, FDefs);
    RELOC_PTR(TExecution_Context, IDefs);
    /* RELOC_PTR(TExecution_Context, glyphIns);  // Never used. */
    RELOC_PTR(TExecution_Context, callStack);
    RELOC_PTR(TExecution_Context, codeRangeTable[0].Base);
    RELOC_PTR(TExecution_Context, codeRangeTable[1].Base);
    RELOC_PTR(TExecution_Context, codeRangeTable[2].Base);
    RELOC_PTR(TExecution_Context, storage);
    RELOC_PTR(TExecution_Context, stack);
    /* zp0, // local, no gc invocations */
    /* zp1, // local, no gc invocations */
    /* zp2, // local, no gc invocations */
    RELOC_PTR(TExecution_Context, pts.org_x);
    RELOC_PTR(TExecution_Context, pts.org_y);
    RELOC_PTR(TExecution_Context, pts.cur_x);
    RELOC_PTR(TExecution_Context, pts.cur_y);
    RELOC_PTR(TExecution_Context, pts.touch);
    RELOC_PTR(TExecution_Context, pts.contours);
    RELOC_PTR(TExecution_Context, twilight.org_x);
    RELOC_PTR(TExecution_Context, twilight.org_y);
    RELOC_PTR(TExecution_Context, twilight.cur_x);
    RELOC_PTR(TExecution_Context, twilight.cur_y);
    RELOC_PTR(TExecution_Context, twilight.touch);
    RELOC_PTR(TExecution_Context, twilight.contours);
    RELOC_PTR(TExecution_Context, cvt);
    DISCARD(mptr);
}
RELOC_PTRS_END

gs_public_st_composite(st_ttfFont, ttfFont,
    "ttfFont", ttfFont_enum_ptrs, ttfFont_reloc_ptrs);

private 
ENUM_PTRS_BEGIN(ttfFont_enum_ptrs) return 0;
    ENUM_PTR(0, ttfFont, face);
    ENUM_PTR(1, ttfFont, inst);
    ENUM_PTR(2, ttfFont, exec);
    ENUM_PTR(3, ttfFont, tti);
ENUM_PTRS_END

private RELOC_PTRS_WITH(ttfFont_reloc_ptrs, ttfFont *mptr)
    RELOC_PTR(ttfFont, face);
    RELOC_PTR(ttfFont, inst);
    RELOC_PTR(ttfFont, exec);
    RELOC_PTR(ttfFont, tti);
    DISCARD(mptr);
RELOC_PTRS_END

gs_public_st_ptrs3(st_ttfInterpreter, ttfInterpreter, "ttfInterpreter", 
    st_ttfInterpreter_enum_ptrs, st_ttfInterpreter_reloc_ptrs, 
    exec, usage, ttf_memory);
