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

/* zfbcp.c */
/* (T)BCP filter creation */
#include "memory_.h"
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gsstruct.h"
#include "ialloc.h"
#include "stream.h"
#include "strimpl.h"
#include "sfilter.h"
#include "ifilter.h"

/* Define null handlers for the BCP out-of-band signals. */
private int
no_bcp_signal_interrupt(stream_state *st)
{	return 0;
}
private int
no_bcp_request_status(stream_state *st)
{	return 0;
}

/* <source> BCPEncode/filter <file> */
int
zBCPE(os_ptr op)
{	return filter_write(op, 0, &s_BCPE_template, NULL, 0);
}

/* <target> BCPDecode/filter <file> */
int
zBCPD(os_ptr op)
{	stream_BCPD_state state;
	state.signal_interrupt = no_bcp_signal_interrupt;
	state.request_status = no_bcp_request_status;
	return filter_read(op, 0, &s_BCPD_template, (stream_state *)&state, 0);
}

/* <source> TBCPEncode/filter <file> */
int
zTBCPE(os_ptr op)
{	return filter_write(op, 0, &s_TBCPE_template, NULL, 0);
}

/* <target> TBCPDecode/filter <file> */
int
zTBCPD(os_ptr op)
{	stream_BCPD_state state;
	state.signal_interrupt = no_bcp_signal_interrupt;
	state.request_status = no_bcp_request_status;
	return filter_read(op, 0, &s_TBCPD_template, (stream_state *)&state, 0);
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zfbcp_op_defs) {
		op_def_begin_filter(),
	{"1BCPEncode", zBCPE},
	{"1BCPDecode", zBCPD},
	{"1TBCPEncode", zTBCPE},
	{"1TBCPDecode", zTBCPD},
END_OP_DEFS(0) }
