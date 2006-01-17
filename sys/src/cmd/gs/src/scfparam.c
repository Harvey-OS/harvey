/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: scfparam.c,v 1.5 2005/07/06 20:54:07 stefan Exp $ */
/* CCITTFax filter parameter setting and reading */
#include "std.h"
#include "gserror.h"
#include "gserrors.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gsparam.h"
#include "scommon.h"
#include "scf.h"		/* for cfe_max_width */
#include "scfx.h"

/* Define the CCITTFax parameters. */
private const gs_param_item_t s_CF_param_items[] =
{
#define cfp(key, type, memb) { key, type, offset_of(stream_CF_state, memb) }
    cfp("Uncompressed", gs_param_type_bool, Uncompressed),
    cfp("K", gs_param_type_int, K),
    cfp("EndOfLine", gs_param_type_bool, EndOfLine),
    cfp("EncodedByteAlign", gs_param_type_bool, EncodedByteAlign),
    cfp("Columns", gs_param_type_int, Columns),
    cfp("Rows", gs_param_type_int, Rows),
    cfp("EndOfBlock", gs_param_type_bool, EndOfBlock),
    cfp("BlackIs1", gs_param_type_bool, BlackIs1),
    cfp("DamagedRowsBeforeError", gs_param_type_int, DamagedRowsBeforeError),
    cfp("FirstBitLowOrder", gs_param_type_bool, FirstBitLowOrder),
    cfp("DecodedByteAlign", gs_param_type_int, DecodedByteAlign),
#undef cfp
    gs_param_item_end
};

/* Define a limit on the Rows parameter, was too low at 32000 */
#define cf_max_height 1000000

/* Get non-default CCITTFax filter parameters. */
stream_state_proc_get_params(s_CF_get_params, stream_CF_state);		/* check */
int
s_CF_get_params(gs_param_list * plist, const stream_CF_state * ss, bool all)
{
    stream_CF_state cfs_defaults;
    const stream_CF_state *defaults;

    if (all)
	defaults = 0;
    else {
	s_CF_set_defaults_inline(&cfs_defaults);
	defaults = &cfs_defaults;
    }
    return gs_param_write_items(plist, ss, defaults, s_CF_param_items);
}

/* Put CCITTFax filter parameters. */
stream_state_proc_put_params(s_CF_put_params, stream_CF_state);		/* check */
int
s_CF_put_params(gs_param_list * plist, stream_CF_state * ss)
{
    stream_CF_state state;
    int code;

    state = *ss;
    code = gs_param_read_items(plist, (void *)&state, s_CF_param_items);
    if (code >= 0 &&
	(state.K < -cf_max_height || state.K > cf_max_height ||
	 state.Columns < 0 || state.Columns > cfe_max_width ||
	 state.Rows < 0 || state.Rows > cf_max_height ||
	 state.DamagedRowsBeforeError < 0 ||
	 state.DamagedRowsBeforeError > cf_max_height ||
	 state.DecodedByteAlign < 1 || state.DecodedByteAlign > 16 ||
	 (state.DecodedByteAlign & (state.DecodedByteAlign - 1)) != 0)
	)
	code = gs_note_error(gs_error_rangecheck);
    if (code >= 0)
	*ss = state;
    return code;
}
