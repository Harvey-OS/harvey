/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* sdctc.c */
/* Code common to DCT encoding and decoding streams */
#include "stdio_.h"
#include "jpeglib.h"
#include "strimpl.h"
#include "sdct.h"

public_st_DCT_state();
/* GC procedures */
private ENUM_PTRS_BEGIN(dct_enum_ptrs) return 0;
	ENUM_CONST_STRING_PTR(0, stream_DCT_state, Markers);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(dct_reloc_ptrs) {
	RELOC_CONST_STRING_PTR(stream_DCT_state, Markers);
} RELOC_PTRS_END
