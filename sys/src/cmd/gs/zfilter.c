/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zfilter.c */
/* Filter creation */
#include "memory_.h"
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gsstruct.h"
#include "ialloc.h"
#include "stream.h"
#include "strimpl.h"
#include "sfilter.h"
#include "srlx.h"
#include "ifilter.h"
#include "files.h"		/* for filter_open, file_d'_buffer_size */

/* <source> ASCIIHexEncode/filter <file> */
int
zAXE(os_ptr op)
{	return filter_write(op, 0, &s_AXE_template, NULL, 0);
}

/* <target> ASCIIHexDecode/filter <file> */
int
zAXD(os_ptr op)
{	return filter_read(op, 0, &s_AXD_template, NULL, 0);
}

/* <target> NullEncode/filter <file> */
int
zNullE(os_ptr op)
{	return filter_write(op, 0, &s_NullE_template, NULL, 0);
}

/* <source> <bool> PFBDecode/filter <file> */
int
zPFBD(os_ptr op)
{	stream_PFBD_state state;
	check_type(*op, t_boolean);
	state.binary_to_hex = op->value.boolval;
	return filter_read(op, 1, &s_PFBD_template, (stream_state *)&state, 0);
}

/* ------ RunLength filters ------ */

/* <target> <record_size> RunLengthEncode/filter <file> */
int
zRLE(register os_ptr op)
{	stream_RLE_state state;
	check_int_leu(*op, max_uint);
	state.record_size = op->value.intval;
	return filter_write(op, 1, &s_RLE_template, (stream_state *)&state, 0);
}

/* <source> RunLengthDecode/filter <file> */
int
zRLD(os_ptr op)
{	return filter_read(op, 0, &s_RLD_template, NULL, 0);
}

/* <source> <EODcount> <EODstring> SubFileDecode/filter <file> */
int
zSFD(os_ptr op)
{	stream_SFD_state state;
	check_type(op[-1], t_integer);
	check_read_type(*op, t_string);
	if ( op[-1].value.intval < 0 )
		return_error(e_rangecheck);
	state.count = op[-1].value.intval;
	state.eod.data = op->value.const_bytes;
	state.eod.size = r_size(op);
	return filter_read(op, 2, &s_SFD_template, (stream_state *)&state,
			   r_space(op));
}

/* ------ Utilities ------ */

/* Forward references */
private int filter_ensure_buf(P3(stream **, uint, bool));

/* Set up an input filter. */
const stream_procs s_new_read_procs =
{	s_std_noavailable, s_std_noseek, s_std_read_reset,
	s_std_read_flush, s_filter_close
};
int
filter_read(os_ptr op, int npop, const stream_template *template,
  stream_state *st, uint space)
{	uint min_size = template->min_out_size;
	uint save_space = ialloc_space(idmemory);
	register os_ptr sop = op - npop;
	stream *s;
	stream *sstrm;
	int code;

	/* Check to make sure that the underlying data */
	/* can function as a source for reading. */
	switch ( r_type(sop) )
	{
	case t_string:
		check_read(*sop);
		ialloc_set_space(idmemory, max(space, r_space(sop)));
		sstrm = file_alloc_stream("filter_read(string stream)");
		if ( sstrm == 0 )
		{	code = gs_note_error(e_VMerror);
			goto out;
		}
		sread_string(sstrm, sop->value.bytes, r_size(sop));
		sstrm->is_temp = 1;
		break;
	case t_file:
		check_read_known_file(sstrm, sop, return,
				      return_error(e_invalidfileaccess));
		ialloc_set_space(idmemory, max(space, r_space(sop)));
		goto ens;
	default:
		check_proc(*sop);
		ialloc_set_space(idmemory, max(space, r_space(sop)));
		code = sread_proc(sop, &sstrm);
		if ( code < 0 )
		  goto out;
		sstrm->is_temp = 2;
ens:		code = filter_ensure_buf(&sstrm,
					 template->min_in_size +
				          sstrm->state->template->min_out_size,
					 false);
		if ( code < 0 )
		  goto out;
		break;
	   }
	if ( min_size < 128 )
	  min_size = file_default_buffer_size;
	code = filter_open("r", min_size, (ref *)sop,
			   &s_new_read_procs, template, st, &s);
	if ( code < 0 )
	  goto out;
	s->strm = sstrm;
	pop(npop);
out:	ialloc_set_space(idmemory, save_space);
	return code;
}

/* Set up an output filter. */
const stream_procs s_new_write_procs =
{	s_std_noavailable, s_std_noseek, s_std_write_reset,
	s_std_write_flush, s_filter_close
};
int
filter_write(os_ptr op, int npop, const stream_template *template,
  stream_state *st, uint space)
{	uint min_size = template->min_in_size;
	uint save_space = ialloc_space(idmemory);
	register os_ptr sop = op - npop;
	stream *s;
	stream *sstrm;
	int code;

	/* Check to make sure that the underlying data */
	/* can function as a sink for writing. */
	switch ( r_type(sop) )
	{
	case t_string:
		check_write(*sop);
		ialloc_set_space(idmemory, max(space, r_space(sop)));
		sstrm = file_alloc_stream("filter_write(string)");
		if ( sstrm == 0 )
		{	code = gs_note_error(e_VMerror);
			goto out;
		}
		swrite_string(sstrm, sop->value.bytes, r_size(sop));
		sstrm->is_temp = 1;
		break;
	case t_file:
		check_write_known_file(sstrm, sop, return);
		ialloc_set_space(idmemory, max(space, r_space(sop)));
		goto ens;
	default:
		check_proc(*sop);
		ialloc_set_space(idmemory, max(space, r_space(sop)));
		code = swrite_proc(sop, &sstrm);
		if ( code < 0 )
		  goto out;
		sstrm->is_temp = 2;
ens:		code = filter_ensure_buf(&sstrm,
					 template->min_out_size +
				          sstrm->state->template->min_in_size,
					 true);
		if ( code < 0 )
		  goto out;
		break;
	}
	if ( min_size < 128 )
	  min_size = file_default_buffer_size;
	code = filter_open("w", min_size, (ref *)sop,
			   &s_new_write_procs, template, st, &s);
	if ( code < 0 )
		goto out;
	s->strm = sstrm;
	pop(npop);
out:	ialloc_set_space(idmemory, save_space);
	return code;
}

/* Ensure a minimum buffer size for a filter. */
/* This may require creating an intermediate stream. */
private int
filter_ensure_buf(stream **ps, uint min_size, bool writing)
{	stream *s = *ps;
	stream *bs;
	ref bsop;
	int code;

	if ( s->modes == 0 /* stream is closed */ || s->bsize >= min_size )
	  return 0;
	/* Otherwise, allocate an intermediate stream. */
	if ( s->cbuf == 0 )
	  {	/* This is a newly created procedure stream. */
		/* Just allocate a buffer for it. */
		uint len = max(min_size, 128);
		byte *buf = ialloc_bytes(len, "filter_ensure_buf");
		if ( buf == 0 )
		  return_error(e_VMerror);
		s->cbuf = buf;
		s->srptr = s->srlimit = s->swptr = buf - 1;
		s->swlimit = buf - 1 + len;
		s->bsize = s->cbsize = len;
		return 0;
	  }
	else
	  {	/* Allocate an intermediate stream. */
		if ( writing )
		  code = filter_open("w", min_size, &bsop, &s_new_write_procs,
				     &s_NullE_template, NULL, &bs);
		else
		  code = filter_open("r", min_size, &bsop, &s_new_read_procs,
				     &s_NullD_template, NULL, &bs);
		if ( code < 0 )
		  return code;
		bs->strm = s;
		bs->is_temp = 2;
		*ps = bs;
		return code;
	  }
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zfilter_op_defs) {
		op_def_begin_filter(),
	{"1ASCIIHexEncode", zAXE},
	{"1ASCIIHexDecode", zAXD},
	{"1NullEncode", zNullE},
	{"2PFBDecode", zPFBD},
	{"2RunLengthEncode", zRLE},
	{"1RunLengthDecode", zRLD},
	{"1SubFileDecode", zSFD},
END_OP_DEFS(0) }
