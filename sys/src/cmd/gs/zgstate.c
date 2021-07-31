/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zgstate.c */
/* Graphics state operators */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "ialloc.h"
#include "istruct.h"
#include "igstate.h"
#include "gsmatrix.h"
#include "store.h"

/* Imported operators */
extern int zpop(P1(os_ptr));

/* Forward references */
private int near num_param(P2(const_os_ptr, int (*)(P2(gs_state *, floatp))));
private int near line_param(P2(const_os_ptr, int *));

/* Structure descriptors */
private_st_int_gstate();

/* ------ Operations on the entire graphics state ------ */

/* The current graphics state */
gs_state *igs;
private gs_gc_root_t igs_root;

/* "Client" procedures */
private void *gs_istate_alloc(P1(gs_memory_t *mem));
private int gs_istate_copy(P2(void *to, const void *from));
private void gs_istate_free(P2(void *old, gs_memory_t *mem));
private const gs_state_client_procs istate_procs = {
	gs_istate_alloc,
	gs_istate_copy,
	gs_istate_free
};

/* Initialize the graphics stack. */
void
igs_init(void)
{	int_gstate *iigs;
	ref proc0;
	igs = gs_state_alloc(imemory);
	iigs = gs_alloc_struct(imemory, int_gstate, &st_int_gstate, "igs_init");
	int_gstate_map_refs(iigs, make_null);
	make_empty_array(&iigs->dash_pattern, a_all);
	ialloc_ref_array(&proc0, a_readonly + a_executable, 2,
			 "igs_init");
	make_oper(proc0.value.refs, 0, zpop);
	make_real(proc0.value.refs + 1, 0.0);
	iigs->black_generation = proc0;
	iigs->undercolor_removal = proc0;
	gs_state_set_client(igs, iigs, &istate_procs);
	gs_register_struct_root(imemory, &igs_root, (void **)&igs, "igs");
	/*
	 * gsave and grestore only work properly
	 * if there are always at least 2 entries on the stack.
	 * We count on the PostScript initialization code to do a gsave.
	 */
}

/* - gsave - */
int
zgsave(register os_ptr op)
{	return gs_gsave(igs);
}

/* - grestore - */
int
zgrestore(register os_ptr op)
{	return gs_grestore(igs);
}

/* - grestoreall - */
int
zgrestoreall(register os_ptr op)
{	return gs_grestoreall(igs);
}

/* - initgraphics - */
int
zinitgraphics(register os_ptr op)
{	return gs_initgraphics(igs);
}

/* ------ Operations on graphics state elements ------ */

/* <num> setlinewidth - */
int
zsetlinewidth(register os_ptr op)
{	return num_param(op, gs_setlinewidth);
}

/* - currentlinewidth <num> */
int
zcurrentlinewidth(register os_ptr op)
{	push(1);
	make_real(op, gs_currentlinewidth(igs));
	return 0;
}

/* <cap_int> setlinecap - */
int
zsetlinecap(register os_ptr op)
{	int param;
	int code = line_param(op, &param);
	if ( !code ) code = gs_setlinecap(igs, (gs_line_cap)param);
	return code;
}

/* - currentlinecap <cap_int> */
int
zcurrentlinecap(register os_ptr op)
{	push(1);
	make_int(op, (int)gs_currentlinecap(igs));
	return 0;
}

/* <join_int> setlinejoin - */
int
zsetlinejoin(register os_ptr op)
{	int param;
	int code = line_param(op, &param);
	if ( !code ) code = gs_setlinejoin(igs, (gs_line_join)param);
	return code;
}

/* - currentlinejoin <join_int> */
int
zcurrentlinejoin(register os_ptr op)
{	push(1);
	make_int(op, (int)gs_currentlinejoin(igs));
	return 0;
}

/* <num> setmiterlimit - */
int
zsetmiterlimit(register os_ptr op)
{	return num_param(op, gs_setmiterlimit);
}

/* - currentmiterlimit <num> */
int
zcurrentmiterlimit(register os_ptr op)
{	push(1);
	make_real(op, gs_currentmiterlimit(igs));
	return 0;
}

/* <array> <offset> setdash - */
int
zsetdash(register os_ptr op)
{	float offset;
	int code = real_param(op, &offset);
	uint n;
	gs_memory_t *mem = imemory;
	float *pattern;

	if ( code < 0 )
	  return_op_typecheck(op);
	check_array(op[-1]);
	/* Adobe interpreters apparently don't check the array for */
	/* read access, so we won't either. */
	/*check_read(op[-1]);*/
	/* Unpack the dash pattern and check it */
	n = r_size(op - 1);
	pattern =
	  (float *)gs_alloc_byte_array(mem, n, sizeof(float), "setdash");
	if ( pattern == 0 )
	  return_error(e_VMerror);
	code = num_params(op[-1].value.const_refs + (n - 1), n, pattern);
	if ( code >= 0 )
	  code = gs_setdash(igs, pattern, n, offset);
	gs_free_object(mem, pattern, "setdash"); /* gs_setdash copies this */
	if ( code >= 0 )
	{	ref_assign(&istate->dash_pattern, op - 1);
		pop(2);
	}
	return code;
}

/* - currentdash <array> <offset> */
int
zcurrentdash(register os_ptr op)
{	push(2);
	ref_assign(op - 1, &istate->dash_pattern);
	make_real(op, gs_currentdash_offset(igs));
	return 0;
}

/* <num> setflat - */
int
zsetflat(register os_ptr op)
{	return num_param(op, gs_setflat);
}

/* - currentflat <num> */
int
zcurrentflat(register os_ptr op)
{	push(1);
	make_real(op, gs_currentflat(igs));
	return 0;
}

/* ------ Extensions ------ */

/* <num> .setfilladjust - */
int
zsetfilladjust(register os_ptr op)
{	return num_param(op, gs_setfilladjust);
}

/* - .currentfilladjust <num> */
int
zcurrentfilladjust(register os_ptr op)
{	push(1);
	make_real(op, gs_currentfilladjust(igs));
	return 0;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zgstate_op_defs) {
	{"0currentdash", zcurrentdash},
	{"0.currentfilladjust", zcurrentfilladjust},
	{"0currentflat", zcurrentflat},
	{"0currentlinecap", zcurrentlinecap},
	{"0currentlinejoin", zcurrentlinejoin},
	{"0currentlinewidth", zcurrentlinewidth},
	{"0currentmiterlimit", zcurrentmiterlimit},
	{"0grestore", zgrestore},
	{"0grestoreall", zgrestoreall},
	{"0gsave", zgsave},
	{"0initgraphics", zinitgraphics},
	{"2setdash", zsetdash},
	{"1.setfilladjust", zsetfilladjust},
	{"1setflat", zsetflat},
	{"1setlinecap", zsetlinecap},
	{"1setlinejoin", zsetlinejoin},
	{"1setlinewidth", zsetlinewidth},
	{"1setmiterlimit", zsetmiterlimit},
END_OP_DEFS(0) }

/* ------ Internal routines ------ */

/* Allocate the interpreter's part of a graphics state. */
private void *
gs_istate_alloc(gs_memory_t *mem)
{	return gs_alloc_struct(mem, int_gstate, &st_int_gstate, "int_gsave");
}

/* Copy the interpreter's part of a graphics state. */
private int
gs_istate_copy(void *to, const void *from)
{	*(int_gstate *)to = *(int_gstate *)from;
	return 0;
}

/* Free the interpreter's part of a graphics state. */
private void
gs_istate_free(void *old, gs_memory_t *mem)
{	gs_free_object(mem, old, "int_grestore");
}

/* Get a numeric parameter */
private int near
num_param(const_os_ptr op, int (*pproc)(P2(gs_state *, floatp)))
{	float param;
	int code = real_param(op, &param);

	if ( code < 0 )
	  return_op_typecheck(op);
	code = (*pproc)(igs, param);
	if ( !code ) pop(1);
	return code;
}

/* Get an integer parameter 0-2. */
private int near
line_param(register const_os_ptr op, int *pparam)
{	check_int_leu(*op, 2);
	*pparam = (int)op->value.intval;
	pop(1);
	return 0;
}
