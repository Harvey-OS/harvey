/* Copyright (C) 1994, 1995 Aladdin Enterprises.  All rights reserved.
  
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

/* zfproc.c */
/* Procedure-based filter stream support */
#include "ghost.h"
#include "errors.h"
#include "oper.h"		/* for ifilter.h */
#include "estack.h"
#include "gsstruct.h"
#include "ialloc.h"
#include "istruct.h"		/* for gs_reloc_refs */
#include "stream.h"
#include "strimpl.h"
#include "ifilter.h"
#include "files.h"
#include "store.h"

extern int zpop(P1(os_ptr));

/* ---------------- Generic ---------------- */

/* GC procedures */
#define pptr ((stream_proc_state *)vptr)
private CLEAR_MARKS_PROC(sproc_clear_marks) {
	r_clear_attrs(&pptr->proc, l_mark);
	r_clear_attrs(&pptr->data, l_mark);
}
private ENUM_PTRS_BEGIN(sproc_enum_ptrs) return 0;
	case 0:
		*pep = &pptr->proc;
		return ptr_ref_type;
	case 1:
		*pep = &pptr->data;
		return ptr_ref_type;
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(sproc_reloc_ptrs) ;
	gs_reloc_refs((ref_packed *)&pptr->proc,
		      (ref_packed *)(&pptr->proc + 1), gcst);
	r_clear_attrs(&pptr->proc, l_mark);
	gs_reloc_refs((ref_packed *)&pptr->data,
		      (ref_packed *)(&pptr->data + 1), gcst);
	r_clear_attrs(&pptr->data, l_mark);
RELOC_PTRS_END
#undef pptr
/* Structure type for procedure-based streams. */
private_st_stream_proc_state();

/* Allocate and open a procedure-based filter. */
/* The caller must have checked that *sop is a procedure. */
private int
s_proc_init(ref *sop, stream **psstrm, uint mode, const stream_template *temp)
{	stream *sstrm;
	stream_proc_state *state;
	static const stream_procs procs =
	   {	s_std_noavailable, s_std_noseek, s_std_read_reset,
		s_std_read_flush, s_std_null, NULL
	   };
	sstrm = file_alloc_stream("s_proc_init(stream)");
	state = (stream_proc_state *)s_alloc_state(imemory, &st_sproc_state,
						   "s_proc_init(state)");
	if ( sstrm == 0 || state == 0 )
	{	ifree_object(state, "s_proc_init(state)");
		/*ifree_object(sstrm, "s_proc_init(stream)");*/	/* just leave it on the file list */
		return_error(e_VMerror);
	}
	s_std_init(sstrm, NULL, 0, &procs, mode);
	sstrm->procs.process = temp->process;
	state->template = temp;
	state->memory = imemory;
	state->eof = 0;
	state->proc = *sop;
	make_empty_string(&state->data, a_all);
	state->index = 0;
	sstrm->state = (stream_state *)state;
	*psstrm = sstrm;
	return 0;
}

/* Handle an interrupt during a stream operation. */
/* This is logically unrelated to procedure streams, */
/* but it is also associated with the interpreter stream machinery. */
private int
s_handle_intc(const ref *pstate, int (*cont)(P1(os_ptr)))
{	int npush;
	if ( pstate )
	{	check_estack(3);
		ref_assign(esp + 2, pstate);
		npush = 3;
	}
	else
	{	check_estack(2);
		npush = 2;
	}
#if 0				/* **************** */
	{ int code = gs_interpret_error(e_interrupt, (ref *)(esp + npush));
	  if ( code < 0 )
	    return code;
	}
#else				/* **************** */
	npush--;
#endif				/* **************** */
	make_op_estack(esp + 1, cont);
	esp += npush;
	return o_push_estack;
}


/* ---------------- Read streams ---------------- */

/* Forward references */
private stream_proc_process(s_proc_read_process);
private int s_proc_read_continue(P1(os_ptr));

/* Stream templates */
private const stream_template s_proc_read_template =
{	&st_sproc_state, NULL, s_proc_read_process, 1, 1, NULL
};

/* Allocate and open a procedure-based read stream. */
/* The caller must have checked that *sop is a procedure. */
int
sread_proc(ref *sop, stream **psstrm)
{	return s_proc_init(sop, psstrm, s_mode_read, &s_proc_read_template);
}

#ifdef Plan9
/* need <string.h> for memcpy */
#include <string.h>
#endif

/* Handle an input request. */
#define ss ((stream_proc_state *)st)
private int
s_proc_read_process(stream_state *st, stream_cursor_read *ignore_pr,
  stream_cursor_write *pw, bool last)
{	/* Move data from the string returned by the procedure */
	/* into the stream buffer, or ask for a callback. */
	uint count = r_size(&ss->data) - ss->index;
	if ( count > 0 )
	  {	uint wcount = pw->limit - pw->ptr;
		if ( wcount < count )
		  count = wcount;
		memcpy(pw->ptr + 1, ss->data.value.bytes + ss->index, count);
		pw->ptr += count;
		ss->index += count;
		return 1;
	  }
	return (ss->eof ? EOFC : CALLC);
}
#undef ss

/* Handle an exception (INTC or CALLC) from a read stream */
/* whose buffer is empty. */
int
s_handle_read_exception(int status, const ref *fop, const ref *pstate,
  int (*cont)(P1(os_ptr)))
{	int npush;
	stream *ps;
	switch ( status )
	{
	case INTC: return s_handle_intc(pstate, cont);
	case CALLC: break;
	default: return_error(e_ioerror);
	}
	/* Find the stream whose buffer needs refilling. */
	for ( ps = fptr(fop); ps->strm != 0; )
	  ps = ps->strm;
#define psst ((stream_proc_state *)ps->state)
	if ( pstate )
	{	check_estack(5);
		esp[2] = *pstate;
		npush = 5;
	}
	else
	{	check_estack(4);
		npush = 4;
	}
	make_op_estack(esp + 1, cont);
	esp += npush;
	make_op_estack(esp - 2, s_proc_read_continue);
	esp[-1] = *fop;
	r_clear_attrs(esp - 1, a_executable);
	*esp = psst->proc;
#undef psst
	return o_push_estack;
}
/* Continue a read operation after returning from a procedure callout. */
/* osp[0] contains the file (pushed on the e-stack by handle_read_status); */
/* osp[-1] contains the new data string (pushed by the procedure). */
/* The top of the e-stack contains the real continuation. */
private int
s_proc_read_continue(os_ptr op)
{	os_ptr opbuf = op - 1;
	stream *ps;
	stream_proc_state *ss;

	check_file(ps, op);
	check_read_type(*opbuf, t_string);
	while ( (ps->end_status = 0, ps->strm) != 0 )
	  ps = ps->strm;
	ss = (stream_proc_state *)ps->state;
	ss->data = *opbuf;
	ss->index = 0;
	if ( r_size(opbuf) == 0 )
	  ss->eof = true;
	pop(2);
	return 0;
}

/* ---------------- Write streams ---------------- */

/* Forward references */
private stream_proc_process(s_proc_write_process);
private int s_proc_write_continue(P1(os_ptr));

/* Stream templates */
private const stream_template s_proc_write_template =
{	&st_sproc_state, NULL, s_proc_write_process, 1, 1, NULL
};

/* Allocate and open a procedure-based write stream. */
/* The caller must have checked that *sop is a procedure. */
int
swrite_proc(ref *sop, stream **psstrm)
{	return s_proc_init(sop, psstrm, s_mode_write, &s_proc_write_template);
}

/* Handle an output request. */
#define ss ((stream_proc_state *)st)
private int
s_proc_write_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *ignore_pw, bool last)
{	/* Move data from the stream buffer to the string */
	/* returned by the procedure, or ask for a callback. */
	uint rcount = pr->limit - pr->ptr;
	if ( rcount > 0 )
	  {	uint wcount = r_size(&ss->data) - ss->index;
		uint count = min(rcount, wcount);
		memcpy(ss->data.value.bytes + ss->index, pr->ptr + 1, count);
		pr->ptr += count;
		ss->index += count;
		if ( rcount > wcount )
		  return CALLC;
		else if ( last )
		  {	ss->eof = true;
			return CALLC;
		  }
		else
		  return 0;
	  }
	return ((ss->eof = last) ? EOFC : 0);
}
#undef ss

/* Handle an exception (INTC or CALLC) from a write stream */
/* whose buffer is full. */
int
s_handle_write_exception(int status, const ref *fop, const ref *pstate,
  int (*cont)(P1(os_ptr)))
{	int npush;
	stream *ps;
	switch ( status )
	{
	case INTC: return s_handle_intc(pstate, cont);
	case CALLC: break;
	default: return_error(e_ioerror);
	}
	/* Find the stream whose buffer needs emptying. */
	for ( ps = fptr(fop); ps->strm != 0; )
	  ps = ps->strm;
#define psst ((stream_proc_state *)ps->state)
	if ( psst->eof )
	{	/* This is the final call from closing the stream. */
		/* Don't run the continuation. */
		check_estack(5);
		esp += 5;
		make_op_estack(esp - 4, zpop);	/* pop the file */
		make_op_estack(esp - 3, zpop);	/* pop the string returned */
						/* by the procedure */
		make_false(esp - 1);
	}
	else
	{	if ( pstate )
		{	check_estack(7);
			ref_assign(esp + 2, pstate);
			npush = 7;
		}
		else
		{	check_estack(6);
			npush = 6;
		}
		make_op_estack(esp + 1, cont);
		esp += npush;
		make_op_estack(esp - 4, s_proc_write_continue);
		esp[-3] = *fop;
		r_clear_attrs(esp - 3, a_executable);
		make_true(esp - 1);
	}
	esp[-2] = psst->proc;
	*esp = psst->data;
	r_set_size(esp, psst->index);
#undef psst
	return o_push_estack;
}
/* Continue a write operation after returning from a procedure callout. */
/* osp[0] contains the file (pushed on the e-stack by handle_write_status); */
/* osp[-1] contains the new buffer string (pushed by the procedure). */
/* The top of the e-stack contains the real continuation. */
private int
s_proc_write_continue(os_ptr op)
{	os_ptr opbuf = op - 1;
	stream *ps;
	stream_proc_state *ss;

	check_file(ps, op);
	check_write_type(*opbuf, t_string);
	while ( (ps->end_status = 0, ps->strm) != 0 )
	  ps = ps->strm;
	ss = (stream_proc_state *)ps->state;
	ss->data = *opbuf;
	ss->index = 0;
	pop(2);
	return 0;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zfproc_op_defs) {
		/* Internal operators */
	{"2%s_proc_read_continue", s_proc_read_continue},
	{"2%s_proc_write_continue", s_proc_write_continue},
END_OP_DEFS(0) }
