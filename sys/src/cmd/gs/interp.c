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

/* interp.c */
/* Ghostscript language interpreter */
#include "memory_.h"
#include "ghost.h"
#include "gsstruct.h"			/* for iastruct.h */
#include "stream.h"
#include "errors.h"
#include "estack.h"
#include "ialloc.h"
#include "iastruct.h"
#include "iname.h"
#include "interp.h"
#include "ipacked.h"
#include "ostack.h"		/* must precede iscan.h */
#include "strimpl.h"		/* for sfilter.h */
#include "sfilter.h"		/* for iscan.h */
#include "iscan.h"
#include "idict.h"
#include "isave.h"
#include "istack.h"
#include "iutil.h"		/* for array_get */
#include "ivmspace.h"
#include "dstack.h"
#include "files.h"		/* for file_check_read */
#include "oper.h"
#include "store.h"

#define PACKED_SPECIAL_OPS 1

/* Imported operator procedures */
extern int zop_add(P1(os_ptr));
extern int zop_def(P1(os_ptr));
extern int zop_sub(P1(os_ptr));
/* Other imported procedures */
extern int in_stopped(P0());
extern int ztokenexec_continue(P1(os_ptr));

/* 
 * The procedure to call if an operator requests rescheduling.
 * This causes an error unless the context machinery has been installed.
 */
private int
no_reschedule(void)
{	return_error(e_invalidcontext);
}
int (*gs_interp_reschedule_proc)(P0()) = no_reschedule;
/*
 * The procedure to call for time-slicing.
 * This is a no-op unless the context machinery has been installed.
 */
int
no_time_slice_proc(void)
{	return 0;
}
int (*gs_interp_time_slice_proc)(P0()) = no_time_slice_proc;
/*
 * The number of interpreter "ticks" between calls on the time_slice_proc.
 * Currently, the clock ticks before each operator, and at each
 * procedure return.
 */
int gs_interp_time_slice_ticks = 0x7fff;

/* Forward references */
private int estack_underflow(P1(os_ptr));
private int interp(P2(ref *, ref *));
private int interp_exit(P1(os_ptr));
private void set_gc_signal(P2(int *, int));
private int copy_stack(P2(const ref_stack *, ref *));

/* Stack sizes */

/*
 * The minimum value of block_ostack is the larger of:
 *	- the maximum number of parameters to an operator
 *	(currently setcolorscreen, with 12 parameters);
 *	- the maximum number of values pushed by an operator
 *	(currently setcolortransfer, which calls zcolor_remap_one 4 times
 *	and therefore pushes 16 values).
 */
#define max_ostack 800
#define min_block_ostack 16
const int gs_interp_max_op_num_args = min_block_ostack;	/* for iinit.c */

/*
 * The minimum value of block_estack is the largest size of a contiguous
 * block surrounding an e_stack mark, currently ???.  At least, that's
 * what the minimum value would be if we supported multi-block estacks,
 * which we currently don't.
 */
#define max_estack 250
#define min_block_estack max_estack

/*
 * The minimum value of block_dstack is the number of permanent entries
 * on the dictionary stack, currently 3.
 */
#define max_dstack 20
#define min_block_dstack 3
uint min_dstack_size;		/* set by iinit.c */

/* Interpreter state variables */
ref ref_systemdict;		/* set by iinit.c */
ref ref_language_level;		/* 1 or 2, set by iinit.c */

/* See estack.h for a description of the execution stack. */

/* The logic for managing icount and iref below assumes that */
/* there are no control operators which pop and then push */
/* information on the execution stack. */

/* Stacks */
/*
 * We must create the stacks as legitimate 'objects' for the memory
 * manager, so that the garbage collector can work with them.
 */
#define os_guard_under 10
#define os_guard_over 10
#define os_refs_size(body_size)\
  (stack_block_refs + os_guard_under + (body_size) + os_guard_over)
ref_stack o_stack;
#define es_guard_under 1
#define es_guard_over 10
#define es_refs_size(body_size)\
  (stack_block_refs + es_guard_under + (body_size) + es_guard_over)
ref_stack e_stack;
#define ds_refs_size(body_size)\
  (stack_block_refs + (body_size))
ref_stack d_stack;

/* Allocate the top blocks statically iff we are dealing with */
/* a segmented architecture where this is important. */
#if stacks_are_segmented
private struct stk_ {
	chunk_head_t head;
	obj_header_t prefix;
	ref bodies[os_refs_size(max_ostack) +
		   es_refs_size(max_estack) +
		   ds_refs_size(max_dstack)];
	ref padding;		/* for GC */
} static_stacks;
#endif
extern_st(st_ref_stack);
ref ref_static_stacks;		/* exported for GC */
ref ref_ref_stacks[3];		/* exported for GC */

/* Stack pointers */
ref *esfile;				/* cache pointer to currentfile */
/* Cached dstack values are in idict.c. */

/* Extended types.  The interpreter may replace the type of operators */
/* in procedures with these, to speed up the interpretation loop. */
/****** NOTE: If you add or change entires in this list, */
/****** you must change the three dispatches in the interpreter loop. */
#define tx_op t_next_index
extern int zadd(P1(os_ptr));
extern int zdef(P1(os_ptr));
extern int zdup(P1(os_ptr));
extern int zexch(P1(os_ptr));
extern int zif(P1(os_ptr));
extern int zifelse(P1(os_ptr));
extern int zindex(P1(os_ptr));
extern int zpop(P1(os_ptr));
extern int zroll(P1(os_ptr));
extern int zsub(P1(os_ptr));
private const op_proc_p special_ops[] = {
	zadd, zdef, zdup, zexch, zif, zifelse, zindex, zpop, zroll, zsub
};
typedef enum {
	tx_op_add = tx_op,
	tx_op_def,
	tx_op_dup,
	tx_op_exch,
	tx_op_if,
	tx_op_ifelse,
	tx_op_index,
	tx_op_pop,
	tx_op_roll,
	tx_op_sub,
	tx_next_op
} special_op_types;
#define num_special_ops ((int)tx_next_op - tx_op)
const int gs_interp_num_special_ops = num_special_ops; /* for iinit.c */
const int tx_next_index = tx_next_op;

/* A null procedure. */
#define make_null_proc(pref)\
  make_empty_const_array(pref, a_executable + a_readonly)
static ref null_proc;

/* Initialize the interpreter */
void
gs_interp_init(void)
{	/* Initialize the stacks. */
	gs_ref_memory_t *smem = iimemory_system;
	ref stk;
	ref euop;
	ref *next_body;
	uint refs_size_ostack, refs_size_estack, refs_size_dstack;

	if ( gs_debug_c('+') )
	  refs_size_ostack = os_refs_size(min_block_ostack),
	  refs_size_estack = es_refs_size(min_block_estack),
	  refs_size_dstack = ds_refs_size(min_block_dstack);
	else
	  refs_size_ostack = os_refs_size(max_ostack),
	  refs_size_estack = es_refs_size(max_estack),
	  refs_size_dstack = ds_refs_size(max_dstack);
#if stacks_are_segmented
	next_body = static_stacks.bodies;
	make_array(&ref_static_stacks, avm_system,
		   countof(static_stacks.bodies), next_body);
	refset_null(next_body, countof(static_stacks.bodies));
#else
	make_empty_array(&ref_static_stacks, 0);
	  {	ref sdata;
		gs_alloc_ref_array(smem, &sdata, 0,
				   refs_size_ostack + refs_size_estack +
				     refs_size_dstack, "interp_init");
		next_body = sdata.value.refs;
	  }
#endif
#define alloc_init_stack(stk, i, n)\
  make_array(&stk, avm_system, n, next_body),\
  next_body += (n),\
  make_struct(&ref_ref_stacks[i], avm_system,\
	      gs_alloc_struct((gs_memory_t *)smem, ref_stack, &st_ref_stack,\
			      "alloc_init_stack"))

	alloc_init_stack(stk, 0, refs_size_ostack);
	ref_stack_init(&o_stack, &stk, os_guard_under, os_guard_over, NULL,
		       smem);
	o_stack.underflow_error = e_stackunderflow;
	o_stack.overflow_error = e_stackoverflow;
	ref_stack_set_max_count(&o_stack, max_ostack);

	alloc_init_stack(stk, 1, refs_size_estack);
	make_oper(&euop, 0, estack_underflow);
	ref_stack_init(&e_stack, &stk, es_guard_under, es_guard_over, &euop,
		       smem);
	e_stack.underflow_error = e_ExecStackUnderflow;
	e_stack.overflow_error = e_execstackoverflow;
	esfile_clear_cache();
	ref_stack_set_max_count(&e_stack, max_estack);

	alloc_init_stack(stk, 2, refs_size_dstack);
	ref_stack_init(&d_stack, &stk, 0, 0, NULL, smem);
	d_stack.underflow_error = e_dictstackunderflow;
	d_stack.overflow_error = e_dictstackoverflow;
	ref_stack_set_max_count(&d_stack, max_dstack);
}
void
gs_interp_reset(void)
{	/* Reset the stacks. */
	ref_stack_clear(&o_stack);
	ref_stack_clear(&e_stack);
	esp++;
	make_oper(esp, 0, interp_exit);
	ref_stack_pop_to(&d_stack, min_dstack_size);
	dict_set_top();
}
/* Report an e-stack block underflow.  The bottom guard slots of */
/* e-stack blocks contain a pointer to this procedure. */
private int
estack_underflow(os_ptr op)
{	return e_ExecStackUnderflow;
}

/*
 * Create an operator during initialization.
 * If operator is hard-coded into the interpreter,
 * assign it a special type and index.
 */
void
gs_interp_make_oper(ref *opref, op_proc_p proc, int idx)
{     register int i = num_special_ops;
      while ( --i >= 0 && proc != special_ops[i] )
	;
      if ( i >= 0 )
        make_tasv(opref, tx_op + i, a_executable, i + 1, opproc, proc);
      else
        make_tasv(opref, t_operator, a_executable, idx, opproc, proc);
}

/*
 * Invoke the interpreter.  If execution completes normally, return 0.
 * If an error occurs, the action depends on user_errors as follows:
 *    user_errors < 0: always return an error code.
 *    user_errors >= 0: let the PostScript machinery handle all errors.
 *	(This will eventually result in a fatal error if no 'stopped'
 *	is active.)
 * In case of a quit or a fatal error, also store the exit code.
 */
private int gs_call_interp(P4(ref *, int, int *, ref *));
int
gs_interpret(ref *pref, int user_errors, int *pexit_code, ref *perror_object)
{	gs_gc_root_t error_root;
	int code;
	gs_register_ref_root(imemory_system, &error_root,
			     (void **)&perror_object, "gs_interpret");
	/* Initialize the error object in case of GC. */
	make_null(perror_object);
	code = gs_call_interp(pref, user_errors, pexit_code, perror_object);
	gs_unregister_root(imemory_system, &error_root, "gs_interpret");
	return code;
}
private int
gs_call_interp(ref *pref, int user_errors, int *pexit_code, ref *perror_object)
{	ref *epref = pref;
	ref doref;
	ref *perrordict;
	ref error_name;
	int code, ccode;
	ref saref;
	int gc_signal;

	*pexit_code = 0;
	ialloc_reset_requested(idmemory);
again:	o_stack.requested = e_stack.requested = d_stack.requested = 0;
	code = interp(epref, perror_object);
	/* Prevent a dangling reference to the GC signal. */
	set_gc_signal(&gc_signal, 0);
	if ( esp < esbot )			/* popped guard entry */
	  esp = esbot;
	switch ( code )
	{
	case e_Fatal:
		*pexit_code = 255;
		return code;
	case e_Quit:
		*perror_object = osp[-1];
		*pexit_code = code = osp->value.intval;
		osp -= 2;
		if ( code == 0 )
		  return e_Quit;
		else if ( code > 0 )
		  return e_Fatal;
		else
		  return code;
	case e_InterpreterExit:
		return 0;
	case e_ExecStackUnderflow:
		/****** WRONG -- must keep mark blocks intact ******/
		ref_stack_pop_block(&e_stack);
		doref = *perror_object;
		epref = &doref;
		goto again;
	case e_VMreclaim:
		/* Do the GC and continue. */
		code = (*idmemory->reclaim)(idmemory,
					    (osp->value.intval == 2 ? 1 : 0));
		/****** What if code < 0? ******/
		make_oper(&doref, 0, zpop);
		epref = &doref;
		goto again;
	}
	/* Adjust osp in case of operand stack underflow */
	if ( osp < osbot - 1 )
		osp = osbot - 1;
	/* We have to handle stack over/underflow specially, because */
	/* we might be able to recover by adding or removing a block. */
	switch ( code )
	  {
	case e_dictstackoverflow:
		if ( ref_stack_extend(&d_stack, d_stack.requested) >= 0 )
		  {	dict_set_top();
			doref = *perror_object;
			epref = &doref;
			goto again;
		  }
		if ( osp >= ostop )
		  {	if ( (ccode = ref_stack_extend(&o_stack, 1)) < 0 )
			  return ccode;
		  }
		ccode = copy_stack(&d_stack, &saref);
		if ( ccode < 0 )
		  return ccode;
		ref_stack_pop_to(&d_stack, min_dstack_size);
		dict_set_top();
		*++osp = saref;
		break;
	case e_dictstackunderflow:
		if ( ref_stack_pop_block(&d_stack) >= 0 )
		  {	dict_set_top();
			doref = *perror_object;
			epref = &doref;
			goto again;
		  }
		break;
	case e_execstackoverflow:
		/* We don't have to handle this specially: */
		/* The only places that could generate it */
		/* use check_estack, which does a ref_stack_extend, */
		/* so if we get this error, it's a real one. */
		if ( osp >= ostop )
		  {	if ( (ccode = ref_stack_extend(&o_stack, 1)) < 0 )
			  return ccode;
		  }
		ccode = copy_stack(&e_stack, &saref);
		if ( ccode < 0 )
		  return ccode;
		{	uint count = ref_stack_count(&e_stack);
			long limit = ref_stack_max_count(&e_stack) - 10;
			if ( count > limit )
			  pop_estack(count - limit);
		}
		*++osp = saref;
		break;
	case e_stackoverflow:
		if ( ref_stack_extend(&o_stack, o_stack.requested) >= 0 )
		  {	/* We can't just re-execute the object, because */
			/* it might be a procedure being pushed as a */
			/* literal.  We check for this case specially. */
			doref = *perror_object;
			if ( r_is_proc(&doref) )
			  {	*++osp = doref;
				make_null_proc(&doref);
			  }
			epref = &doref;
			goto again;
		  }
		ccode = copy_stack(&o_stack, &saref);
		if ( ccode < 0 ) return ccode;
		ref_stack_clear(&o_stack);
		*++osp = saref;
		break;
	case e_stackunderflow:
		if ( ref_stack_pop_block(&o_stack) >= 0 )
		  {	doref = *perror_object;
			epref = &doref;
			goto again;
		  }
		break;
	  }
	if ( user_errors < 0 )
	  return code;
	if ( (ccode = gs_errorname(code, &error_name)) < 0 )
	  return ccode;		/* something is very wrong! */
	if ( dict_find_string(systemdict, "errordict", &perrordict) <= 0 ||
	     dict_find(perrordict, &error_name, &epref) <= 0
	   )
	  return_error(e_undefined); /* error name not in errordict??? */
	doref = *epref;
	epref = &doref;
	/* Push the error object on the operand stack if appropriate. */
	if ( !error_is_interrupt(code) )
	  *++osp = *perror_object;
	goto again;
}	
private int
interp_exit(os_ptr op)
{	return e_InterpreterExit;
}

/* Set the GC signal for global and local VM. */
private void
set_gc_signal(int *psignal, int value)
{	gs_memory_gc_status_t stat;
	gs_memory_gc_status(iimemory_local, &stat);
	stat.psignal = psignal;
	stat.signal_value = value;
	gs_memory_set_gc_status(iimemory_local, &stat);
	gs_memory_gc_status(iimemory_global, &stat);
	stat.psignal = psignal;
	stat.signal_value = value;
	gs_memory_set_gc_status(iimemory_global, &stat);
}


/* Copy the contents of an overflowed stack into a (local) array. */
private int
copy_stack(const ref_stack *pstack, ref *arr)
{	uint size = ref_stack_count(pstack);
	uint save_space = ialloc_space(idmemory);
	int code;
	ialloc_set_space(idmemory, avm_local);
	code = ialloc_ref_array(arr, a_all, size, "copy_stack");
	if ( code >= 0 )
	  code = ref_stack_store(pstack, arr, size, 0, 1, true, "copy_stack");
	ialloc_set_space(idmemory, save_space);
	return code;
}

/* Get the name corresponding to an error number. */
int
gs_errorname(int code, ref *perror_name)
{	ref *perrordict, *pErrorNames;
	if ( dict_find_string(systemdict, "errordict", &perrordict) <= 0 ||
	     dict_find_string(systemdict, "ErrorNames", &pErrorNames) <= 0
	   )
	  return_error(e_undefined); /* errordict or ErrorNames not found?! */
	return array_get(pErrorNames, (long)(-code - 1), perror_name);
}

/* Store an error string in $error.errorinfo. */
/* This routine is here because of the proximity to the error handler. */
int
gs_errorinfo_put_string(const char *str)
{	ref rstr;
	ref *pderror;
	int code = string_to_ref(str, &rstr, iimemory, "gs_errorinfo_put_string");
	if ( code < 0 )
		return code;
	if ( dict_find_string(systemdict, "$error", &pderror) <= 0 ||
	     !r_has_type(pderror, t_dictionary) ||
	     dict_put_string(pderror, "errorinfo", &rstr) < 0
	   )
		return_error(e_Fatal);
	return 0;
}

/* Main interpreter. */
/* If execution terminates normally, return e_InterpreterExit. */
/* If an error occurs, leave the current object in *perror_object */
/* and return a (negative) error code. */
private int
interp(ref *pref /* object to interpret */, ref *perror_object)
{	register ref *iref = pref;
	register int icount = 0;	/* # of consecutive tokens at iref */
	register os_ptr iosp = osp;	/* private copy of osp */
	register es_ptr iesp = esp;	/* private copy of esp */
	int code;
	ref token;		/* token read from file or string, */
				/* must be declared in this scope */
	register ref *pvalue;
	os_ptr whichp;
	/*
	 * We have to make the error information into a struct;
	 * otherwise, the Watcom compiler will assign it to registers
	 * strictly on the basis of textual frequency.
	 * We also have to use ref_assign_inline everywhere, and
	 * avoid direct assignments of refs, so that esi and edi
	 * will remain available on Intel processors.
	 */
	struct interp_error_s {
		int code;
		int line;
		ref *obj;
		ref full;
	} ierror;
	/*
	 * Get a pointer to the name table so that we can use the
	 * inline version of name_index_ref.
	 */
	const name_table *int_nt = the_name_table();
#define set_error(ecode)\
  { ierror.code = ecode; ierror.line = __LINE__; }
#define return_with_error(ecode, objp)\
  { set_error(ecode); ierror.obj = objp; goto rwe; }
#define return_with_error_iref(ecode)\
  { set_error(ecode); goto rwei; }
#define return_with_code_iref()\
  { ierror.line = __LINE__; goto rweci; }
#define return_with_error_code_op(nargs)\
  return_with_code_iref()
	int ticks_left = gs_interp_time_slice_ticks;

	/*
	 * If we exceed the VMThreshold, set ticks_left to -1
	 * to alert the interpreter that we need to garbage collect.
	 */
	set_gc_signal(&ticks_left, -100);

	esfile_clear_cache();
	/*
	 * From here on, if icount > 0, iref and icount correspond
	 * to the top entry on the execution stack: icount is the count
	 * of sequential entries remaining AFTER the current one.
	 */
#define add1_short(pref) (ref *)((ushort *)(pref) + 1)
#define add1_either(pref) (r_is_packed(pref) ? add1_short(pref) : (pref) + 1)
#define store_state(ep)\
  ( icount > 0 ? (ep->value.refs = iref + 1, r_set_size(ep, icount)) : 0 )
#define store_state_short(ep)\
  ( icount > 0 ? (ep->value.refs = add1_short(iref), r_set_size(ep, icount)) : 0 )
#define store_state_either(ep)\
  ( icount > 0 ? (ep->value.refs = add1_either(iref), r_set_size(ep, icount)) : 0 )
#define next()\
  if ( --icount > 0 ) { iref++; goto top; } else goto out
#define next_short()\
  if ( --icount <= 0 ) { if ( icount < 0 ) goto up; iesp--; }\
  iref = add1_short(iref); goto top
#define next_either()\
  if ( --icount <= 0 ) { if ( icount < 0 ) goto up; iesp--; }\
  iref = add1_either(iref); goto top

#if !PACKED_SPECIAL_OPS
#  undef next_either
#  define next_either() next()
#  undef store_state_either
#  define store_state_either(ep) store_state(ep)
#endif

	/* We want to recognize executable arrays here, */
	/* so we push the argument on the estack and enter */
	/* the loop at the bottom. */
	if ( iesp >= estop ) return_with_error(e_execstackoverflow, pref);
	++iesp;
	ref_assign_inline(iesp, pref);
	goto bot;
top:	/*
	 * This is the top of the interpreter loop.
	 * iref points to the ref being interpreted.
	 * Note that this might be an element of a packed array,
	 * not a real ref: we carefully arranged the first 16 bits of
	 * a ref and of a packed array element so they could be distinguished
	 * from each other.  (See ghost.h and packed.h for more detail.)
	 */
#ifdef DEBUG
	/* Do a little validation on the top o-stack entry. */
	if ( iosp >= osbot && r_type(iosp) == t__invalid )
	  {	lprintf("Invalid value on o-stack!\n");
		return_with_error_iref(e_Fatal);
	  }
if ( gs_debug['I'] ||
     (gs_debug['i'] &&
      (r_is_packed(iref) ?
       r_packed_is_name((ref_packed *)iref) :
       r_has_type(iref, t_name)))
   )
   {	void debug_print_ref(P1(ref *));
	os_ptr save_osp = osp;		/* avoid side-effects */
	es_ptr save_esp = esp;
	int edepth;
	char depth[10];
	osp = iosp;
	esp = iesp;
	edepth = ref_stack_count(&e_stack);
	sprintf(depth, "%2d", edepth);
	dputs(depth);
	for ( edepth -= strlen(depth); edepth >= 5; edepth -= 5 )
	  dputc('*');		/* indent */
	for ( ; edepth > 0; --edepth )
	  dputc('.');
	dprintf3("0x%lx(%d)<%d>: ",
		 (ulong)iref, icount, ref_stack_count(&o_stack));
	debug_print_ref(iref);
	if ( iosp >= osbot )
	   {	dputs(" // ");
		debug_print_ref(iosp);
	   }
	dputc('\n');
	osp = save_osp;
	esp = save_esp;
	fflush(dstderr);
   }
#endif
/* Objects that have attributes (arrays, dictionaries, files, and strings) */
/* use lit and exec; other objects use plain and plain_exec. */
#define lit(t) type_xe_value(t, a_execute)
#define exec(t) type_xe_value(t, a_execute + a_executable)
#define nox(t) type_xe_value(t, 0)
#define nox_exec(t) type_xe_value(t, a_executable)
#define plain(t) type_xe_value(t, 0)
#define plain_exec(t) type_xe_value(t, a_executable)
	/*
	 * We have to populate enough cases of the switch statement to force
	 * some compilers to use a dispatch rather than a testing loop.
	 * What a nuisance!
	 */
	switch ( r_type_xe(iref) )
	   {
	/* Access errors. */
#define cases_invalid()\
  case plain(t__invalid): case plain_exec(t__invalid)
	cases_invalid():
		return_with_error_iref(e_Fatal);
#define cases_nox()\
  case nox_exec(t_array): case nox_exec(t_dictionary):\
  case nox_exec(t_file): case nox_exec(t_string):\
  case nox_exec(t_mixedarray): case nox_exec(t_shortarray)
	cases_nox():
		return_with_error_iref(e_invalidaccess);
	/*
	 * Literal objects.  We have to enumerate all the types.
	 * In fact, we have to include some extra plain_exec entries
	 * just to populate the switch.  We break them up into groups
	 * to avoid overflowing some preprocessors.
	 */
#define cases_lit_1()\
  case lit(t_array): case nox(t_array):\
  case plain(t_boolean): case plain_exec(t_boolean):\
  case lit(t_dictionary): case nox(t_dictionary)
#define cases_lit_2()\
  case lit(t_file): case nox(t_file):\
  case plain(t_fontID): case plain_exec(t_fontID):\
  case plain(t_integer): case plain_exec(t_integer):\
  case plain(t_mark): case plain_exec(t_mark)
#define cases_lit_3()\
  case plain(t_name):\
  case plain(t_null):\
  case plain(t_oparray):\
  case plain(t_operator)
#define cases_lit_4()\
  case plain(t_real): case plain_exec(t_real):\
  case plain(t_save): case plain_exec(t_save):\
  case lit(t_string): case nox(t_string)
#define cases_lit_5()\
  case lit(t_mixedarray): case nox(t_mixedarray):\
  case lit(t_shortarray): case nox(t_shortarray):\
  case plain(t_device): case plain_exec(t_device):\
  case plain(t_struct): case plain_exec(t_struct):\
  case plain(t_astruct): case plain_exec(t_astruct)
	/* Executable arrays are treated as literals in direct execution. */
#define cases_lit_array()\
  case exec(t_array): case exec(t_mixedarray): case exec(t_shortarray)
	cases_lit_1():
	cases_lit_2():
	cases_lit_3():
	cases_lit_4():
	cases_lit_5():
	cases_lit_array():
		break;
	/* Special operators. */
	case plain_exec(tx_op_add):
x_add:		if ( (code = zop_add(iosp)) < 0 )
		  return_with_error_code_op(2);
		iosp--;
		next_either();
	case plain_exec(tx_op_def):
x_def:		if ( (code = zop_def(iosp)) < 0 )
		  return_with_error_code_op(2);
		iosp -= 2;
		next_either();
	case plain_exec(tx_op_dup):
x_dup:		if ( iosp < osbot )
		  return_with_error_iref(e_stackunderflow);
		iosp++;
		ref_assign_inline(iosp, iosp - 1);
		next_either();
	case plain_exec(tx_op_exch):
x_exch:		if ( iosp <= osbot )
		  return_with_error_iref(e_stackunderflow);
		ref_assign_inline(&token, iosp);
		ref_assign_inline(iosp, iosp - 1);
		ref_assign_inline(iosp - 1, &token);
		next_either();
	case plain_exec(tx_op_if):
x_if:		if ( !r_has_type(iosp - 1, t_boolean) )
		  return_with_error_iref((iosp <= osbot ?
					  e_stackunderflow : e_typecheck));
		if ( !iosp[-1].value.boolval )
		  { iosp -= 2;
		    next_either();
		  }
		if ( iesp >= estop )
		  return_with_error_iref(e_execstackoverflow);
		store_state_either(iesp);
		whichp = iosp;
		iosp -= 2;
		goto ifup;
	case plain_exec(tx_op_ifelse):
x_ifelse:	if ( !r_has_type(iosp - 2, t_boolean) )
		  return_with_error_iref((iosp < osbot + 2 ?
					  e_stackunderflow : e_typecheck));
		if ( iesp >= estop )
		  return_with_error_iref(e_execstackoverflow);
		store_state_either(iesp);
		whichp = (iosp[-2].value.boolval ? iosp - 1 : iosp);
		iosp -= 3;
		/* Open code "up" for the array case(s) */
ifup:		if ( !r_is_proc(whichp) )
		   {	/* This is an unusual enough case that we go ahead */
			/* and clear the currentfile cache without */
			/* checking whether we have an exec(t_file), */
			/* just to avoid another case test. */
			esfile_clear_cache();
			ref_assign_inline(iesp + 1, whichp);
			iref = iesp + 1;
			icount = 0;
			goto top;
		   }
		if ( (icount = r_size(whichp) - 1) <= 0 )
		   {	if ( icount < 0 ) goto up;	/* 0-element proc */
			iref = whichp->value.refs;	/* 1-element proc */
			if ( --ticks_left > 0 )
			  goto top;
		   }
		++iesp;
		/* Do a ref_assign, but also set iref. */
		iesp->tas = whichp->tas;
		iref = iesp->value.refs = whichp->value.refs;
		if ( --ticks_left > 0 )
		  goto top;
		goto slice;
	case plain_exec(tx_op_index):
x_index:	osp = iosp;	/* zindex references o_stack */
		if ( (code = zindex(iosp)) < 0 )
		  return_with_error_code_op(1);
		next_either();
	case plain_exec(tx_op_pop):
x_pop:		if ( iosp < osbot )
		  return_with_error_iref(e_stackunderflow);
		iosp--;
		next_either();
	case plain_exec(tx_op_roll):
x_roll:		osp = iosp;	/* zroll references o_stack */
		if ( (code = zroll(iosp)) < 0 )
		  return_with_error_code_op(2);
		iosp -= 2;
		next_either();
	case plain_exec(tx_op_sub):
x_sub:		if ( (code = zop_sub(iosp)) < 0 )
		  return_with_error_code_op(2);
		iosp--;
		next_either();
	/* Executable types. */
	case plain_exec(t_null):
		goto bot;
	case plain_exec(t_oparray):
		/* Replace with the definition and go again. */
		pvalue =
		  &op_array_table.value.refs[op_index(iref) - op_def_count];
prst:		/* Prepare to call the procedure (array) in *pvalue. */
		store_state(iesp);
pr:		/* Call the array in *pvalue.  State has been stored. */
		if ( (icount = r_size(pvalue) - 1) <= 0 )
		   {	if ( icount < 0 ) goto up;	/* 0-element proc */
			iref = pvalue->value.refs;	/* 1-element proc */
			if ( --ticks_left > 0 )
			  goto top;
		   }
		if ( iesp >= estop )
		  return_with_error_iref(e_execstackoverflow);
		++iesp;
		/* Do a ref_assign, but also set iref. */
		iesp->tas = pvalue->tas;
		iref = iesp->value.refs = pvalue->value.refs;
		if ( --ticks_left > 0 )
		  goto top;
		goto slice;
	case plain_exec(t_operator):
		if ( --ticks_left <= 0 )
		  {	/* The following doesn't work, */
			/* and I can't figure out why. */
			/****** goto sst; ******/
		  }
		esp = iesp;		/* save for operator */
		osp = iosp;		/* ditto */
		/* Operator routines take osp as an argument. */
		/* This is just a convenience, since they adjust */
		/* osp themselves to reflect the results. */
		/* Operators that (net) push information on the */
		/* operand stack must check for overflow: */
		/* this normally happens automatically through */
		/* the push macro (in oper.h). */
		/* Operators that do not typecheck their operands, */
		/* or take a variable number of arguments, */
		/* must check explicitly for stack underflow. */
		/* (See oper.h for more detail.) */
		/* Note that each case must set iosp = osp: */
		/* this is so we can switch on code without having to */
		/* store it and reload it (for dumb compilers). */
		switch ( code = (*real_opproc(iref))(iosp) )
		   {
		case 0:			/* normal case */
		case 1:			/* alternative success case */
			iosp = osp;
			next();
		case o_push_estack:	/* store the state and go to up */
			store_state(iesp);
opush:			iosp = osp;
			iesp = esp;
			if ( --ticks_left > 0 )
			  goto up;
			goto slice;
		case o_pop_estack:	/* just go to up */
opop:			iosp = osp;
			if ( esp == iesp ) goto bot;
			iesp = esp;
			goto up;
		case o_reschedule:
			store_state(iesp);
			goto res;
		case e_InsertProc:
			store_state(iesp);
oeinsert:		ref_assign_inline(iesp + 1, iref);
			/* esp = iesp + 2; *esp = the procedure */
			iesp = esp;
			goto up;
		   }
		iosp = osp;
		iesp = esp;
		return_with_code_iref();
	case plain_exec(t_name):
		pvalue = iref->value.pname->pvalue;
		if ( !pv_valid(pvalue) )
		{	uint nidx = name_index(iref);
			uint htemp;
			if ( (pvalue = dict_find_name_by_index_inline(nidx, htemp)) == 0 )
			  return_with_error_iref(e_undefined);
		}
		/* Dispatch on the type of the value. */
		/* Again, we have to over-populate the switch. */
		switch ( r_type_xe(pvalue) )
		   {
		cases_invalid():
			return_with_error_iref(e_Fatal);
		cases_nox():	/* access errors */
			return_with_error_iref(e_invalidaccess);
		cases_lit_1():
		cases_lit_2():
		cases_lit_3():
		cases_lit_4():
		cases_lit_5():
			/* Just push the value */
			if ( iosp >= ostop )
			  return_with_error(e_stackoverflow, pvalue);
			++iosp;
			ref_assign_inline(iosp, pvalue);
			next();
		case exec(t_array):
		case exec(t_mixedarray):
		case exec(t_shortarray):
			/* This is an executable procedure, execute it. */
			goto prst;
		case plain_exec(tx_op_add): goto x_add;
		case plain_exec(tx_op_def): goto x_def;
		case plain_exec(tx_op_dup): goto x_dup;
		case plain_exec(tx_op_exch): goto x_exch;
		case plain_exec(tx_op_if): goto x_if;
		case plain_exec(tx_op_ifelse): goto x_ifelse;
		case plain_exec(tx_op_index): goto x_index;
		case plain_exec(tx_op_pop): goto x_pop;
		case plain_exec(tx_op_roll): goto x_roll;
		case plain_exec(tx_op_sub): goto x_sub;
		case plain_exec(t_null):
			goto bot;
		case plain_exec(t_oparray):
			pvalue =
			  &op_array_table.value.refs[op_index(pvalue) -
						     op_def_count];
			goto prst;
		case plain_exec(t_operator):
		   {	/* Shortcut for operators. */
			/* See above for the logic. */
			if ( --ticks_left <= 0 )
			  {	/* The following doesn't work, */
				/* and I can't figure out why. */
				/****** goto sst; ******/
			  }
			esp = iesp;
			osp = iosp;
			switch ( code = (*real_opproc(pvalue))(iosp) )
			   {
			case 0:			/* normal case */
			case 1:			/* alternative success case */
				iosp = osp;
				next();
			case o_push_estack:
				store_state(iesp);
				goto opush;
			case o_pop_estack:
				goto opop;
			case o_reschedule:
				store_state(iesp);
				goto res;
			case e_InsertProc:
				store_state(iesp);
				goto oeinsert;
			   }
			iosp = osp;
			iesp = esp;
			return_with_error(code, pvalue);
		   }
		case plain_exec(t_name):
		case exec(t_file):
		case exec(t_string):
		default:
			/* Not a procedure, reinterpret it. */
			store_state(iesp);
			icount = 0;
			iref = pvalue;
			goto top;
		   }
	case exec(t_file):
	   {	/* Executable file.  Read the next token and interpret it. */
	   	stream *s;
		scanner_state sstate;
		check_read_known_file(s, iref, return_with_error_iref,
				      s = invalid_file_entry);
rt:		if ( iosp >= ostop )	/* check early */
		  return_with_error_iref(e_stackoverflow);
		osp = iosp;		/* scan_token uses ostack */
		scanner_state_init(&sstate, false);
again:		code = scan_token(s, &token, &sstate);
		iosp = osp;		/* ditto */
		switch ( code )
		   {
		case 0:			/* read a token */
			/* It's worth checking for literals, which make up */
			/* the majority of input tokens, before storing the */
			/* state on the e-stack.  Note that because of //, */
			/* the token may have *any* type and attributes. */
			/* Note also that executable arrays aren't executed */
			/* at the top level -- they're treated as literals. */
			if ( !r_has_attr(&token, a_executable) ||
			     r_is_array(&token)
			   )
			  {	/* If scan_token used the o-stack, */
				/* we know we can do a push now; if not, */
				/* the pre-check is still valid. */
				iosp++;
				ref_assign_inline(iosp, &token);
				goto rt;
			  }
			store_state(iesp);
			/* Push the file on the e-stack */
			if ( iesp >= estop )
			  return_with_error_iref(e_execstackoverflow);
			esfile_set_cache(++iesp);
			ref_assign_inline(iesp, iref);
			iref = &token;
			icount = 0;
			goto top;
		case scan_EOF:		/* end of file */
			esfile_clear_cache();
			goto bot;
		case scan_BOS:
			/* Binary object sequences */
			/* ARE executed at the top level. */
			store_state(iesp);
			/* Push the file on the e-stack */
			if ( iesp >= estop )
			  return_with_error_iref(e_execstackoverflow);
			esfile_set_cache(++iesp);
			ref_assign_inline(iesp, iref);
			pvalue = &token;
			goto pr;
		case scan_Refill:
			store_state(iesp);
			/* iref may point into the exec stack; */
			/* save its referent now. */
			ref_assign_inline(&token, iref);
			/* Push the file on the e-stack */
			if ( iesp >= estop )
			  return_with_error_iref(e_execstackoverflow);
			++iesp;
			ref_assign_inline(iesp, &token);
			esp = iesp;
			osp = iosp;
			code = scan_handle_refill(&token, &sstate, true,
						  ztokenexec_continue);
			iosp = osp;
			iesp = esp;
			switch ( code )
			  {
			  case 0:
				iesp--;		/* don't push the file */
				goto again;	/* stacks are unchanged */
			  case o_push_estack:
				/* See above for why it is safe to push */
				/* the file onto the o-stack */
				/* (for ztokenexec_continue). */
				esfile_clear_cache();
				iosp++;
				ref_assign_inline(iosp, &token);
				if ( --ticks_left > 0 )
					goto up;
				goto slice;
			  }
			/* must be an error */
			iesp--;		/* don't push the file */
		default:		/* error */
			return_with_code_iref();
		   }
	   }
	case exec(t_string):
	   {	/* Executable string.  Read a token and interpret it. */
		stream ss;
		scanner_state sstate;
		scanner_state_init(&sstate, true);
		sread_string(&ss, iref->value.bytes, r_size(iref));
		osp = iosp;		/* scan_token uses ostack */
		code = scan_token(&ss, &token, &sstate);
		iosp = osp;		/* ditto */
		switch ( code )
		  {
		case 0:			/* read a token */
		case scan_BOS:		/* binary object sequence */
		    store_state(iesp);
		    /* Push the updated string back on the e-stack */
		    if ( iesp >= estop )
		      return_with_error_iref(e_execstackoverflow);
		    ++iesp;
		    iesp->tas.type_attrs = iref->tas.type_attrs;
		    iesp->value.const_bytes = sbufptr(&ss);
		    r_set_size(iesp, sbufavailable(&ss));
		    if ( code == 0 )
		    {	iref = &token;
			icount = 0;
			goto top;
		    }
		    /* Handle BOS specially */
		    pvalue = &token;
		    goto pr;
		case scan_EOF:		/* end of string */
		    goto bot;
		case scan_Refill:	/* error */
		    code = gs_note_error(e_syntaxerror);
		default:		/* error */
		    return_with_code_iref();
		  }
	   }
	/* Handle packed arrays here by re-dispatching. */
	/* This also picks up some anomalous cases of non-packed arrays. */
	default:
	  {	uint index;
		switch ( *(ushort *)iref >> r_packed_type_shift )
		{
		case pt_full_ref:
		case pt_full_ref+1:
			if ( iosp >= ostop )
			  return_with_error_iref(e_stackoverflow);
			/* We know this can't be an executable object */
			/* requiring special handling, so we just push it. */
			++iosp;
			/* We know that refs are properly aligned: */
			/* see packed.h for details. */
			ref_assign_inline(iosp, iref);
			next();
		case pt_executable_operator:
			index = *(ushort *)iref & packed_value_mask;
			if ( --ticks_left <= 0 )
			  {	/* The following doesn't work, */
				/* and I can't figure out why. */
				/****** goto sst_short; ******/
			  }
			if ( !op_index_is_operator(index) )
			{	store_state_short(iesp);
				/* Call the operator procedure. */
				pvalue =
				  &op_array_table.value.refs[index -
							     op_def_count];
				goto pr;
			}
			/* See the main plain_exec(t_operator) case */
			/* for details of what happens here. */
#if PACKED_SPECIAL_OPS
			/* We arranged in iinit.c that the special ops */
			/* have operator indices starting at 1. */
#  define case_xop(xop) case xop - tx_op + 1
			switch ( index )
			  {
			  case_xop(tx_op_add): goto x_add;
			  case_xop(tx_op_def): goto x_def;
			  case_xop(tx_op_dup): goto x_dup;
			  case_xop(tx_op_exch): goto x_exch;
			  case_xop(tx_op_if): goto x_if;
			  case_xop(tx_op_ifelse): goto x_ifelse;
			  case_xop(tx_op_index): goto x_index;
			  case_xop(tx_op_pop): goto x_pop;
			  case_xop(tx_op_roll): goto x_roll;
			  case_xop(tx_op_sub): goto x_sub;
			  case 0:	/* for dumb compilers */
			  default:
			    ;
			  }
#  undef case_xop
#endif
			esp = iesp;
			osp = iosp;
			switch ( code = (*op_index_proc(index))(iosp) )
			   {
			case 0:
			case 1:
				iosp = osp;
				next_short();
			case o_push_estack:
				store_state_short(iesp);
				goto opush;
			case o_pop_estack:
				iosp = osp;
				if ( esp == iesp )
				   {	next_short();
				   }
				iesp = esp;
				goto up;
			case o_reschedule:
				store_state_short(iesp);
				goto res;
			case e_InsertProc:
				store_state_short(iesp);
				packed_get((const ref_packed *)iref, iesp + 1);
				/* esp = iesp + 2; *esp = the procedure */
				iesp = esp;
				goto up;
			   }
			iosp = osp;
			iesp = esp;
			return_with_code_iref();
		case pt_integer:
			if ( iosp >= ostop )
			  return_with_error_iref(e_stackoverflow);
			++iosp;
			make_int(iosp, (*(short *)iref & packed_int_mask) +
					packed_min_intval);
			next_short();
		case pt_literal_name:
		  {	uint nidx = *(ushort *)iref & packed_value_mask;
			if ( iosp >= ostop )
			  return_with_error_iref(e_stackoverflow);
			++iosp;
			name_index_ref_inline(int_nt, nidx, iosp);
			next_short();
		  }
		case pt_executable_name:
		  {	uint nidx = (uint)*(ushort *)iref & packed_value_mask;
			pvalue = name_index_ptr_inline(int_nt, nidx)->pvalue;
			if ( !pv_valid(pvalue) )
			   {	uint htemp;
				if ( (pvalue = dict_find_name_by_index_inline(nidx, htemp)) == 0 )
				  {	name_index_ref(nidx, &token);
					return_with_error(e_undefined, &token);
				  }
			   }
			if ( r_has_masked_attrs(pvalue, a_execute, a_execute + a_executable) )
			  {	/* Literal, push it. */
				if ( iosp >= ostop )
				  return_with_error_iref(e_stackoverflow);
				++iosp;
				ref_assign_inline(iosp, pvalue);
				next_short();
			  }
			if ( r_is_proc(pvalue) )
			   {	/* This is an executable procedure, */
				/* execute it. */
				store_state_short(iesp);
				goto pr;
			   }
			/* Not a literal or procedure, reinterpret it. */
			store_state_short(iesp);
			icount = 0;
			iref = pvalue;
			goto top;
		  }
		/* default can't happen here */
		}
	  }
	}
	/* Literal type, just push it. */
	if ( iosp >= ostop )
	  return_with_error_iref(e_stackoverflow);
	++iosp;
	ref_assign_inline(iosp, iref);
bot:	next();
out:	/* At most 1 more token in the current procedure. */
	/* (We already decremented icount.) */
	if ( !icount )
	   {	/* Pop the execution stack for tail recursion. */
		iesp--;
		iref++;
		goto top;
	   }
up:	if ( --ticks_left < 0 )
	  goto slice;
	/* See if there is anything left on the execution stack. */
	if ( !r_is_proc(iesp) )
	   {	iref = iesp--;
		icount = 0;
		goto top;
	   }
	iref = iesp->value.refs;		/* next element of array */
	icount = r_size(iesp) - 1;
	if ( icount <= 0 )		/* <= 1 more elements */
	   {	iesp--;			/* pop, or tail recursion */
		if ( icount < 0 ) goto up;
	   }
	goto top;
res:	/* Some operator has asked for context rescheduling. */
	/* We've done a store_state. */
	code = (*gs_interp_reschedule_proc)();
sched:	/* We've just called a scheduling procedure. */
	/* The interpreter state is in memory; iref is not current. */
	if ( code < 0 )
	{	set_error(code);
		make_null_proc(&null_proc);
		ierror.obj = iref = &null_proc;
		goto error_exit;
	}
	/* Reload state information from memory. */
	iosp = osp;
	iesp = esp;
	goto up;
#if 0			/****** ****** ******/
sst:	/* Time-slice, but push the current object first. */
	store_state(iesp);
	if ( iesp >= estop )
	  return_with_error_iref(e_execstackoverflow);
	iesp++;
	ref_assign_inline(iesp, iref);
#endif			/****** ****** ******/
slice:	/* It's time to time-slice or garbage collect. */
	/* iref is not live, so we don't need to do a store_state. */
	osp = iosp;
	esp = iesp;
	/* If ticks_left <= -100, we need to GC now. */
	if ( ticks_left <= -100 )
	  {	/* We need to garbage collect now. */
		code = (*idmemory->reclaim)(idmemory, -1);
	  }
	else
		code = (*gs_interp_time_slice_proc)();
	ticks_left = gs_interp_time_slice_ticks;
	goto sched;

	/* Error exits. */

rweci:
	ierror.code = code;
rwei:
	ierror.obj = iref;
rwe:
	if ( !r_is_packed(iref) )
	  store_state(iesp);
	else
	  {	/* We need a real object to return as the error object. */
		/* (It only has to last long enough to store in *perror_object.) */
		packed_get((const ref_packed *)ierror.obj, &ierror.full);
		store_state_short(iesp);
		if ( iref == ierror.obj )
		  iref = &ierror.full;
		ierror.obj = &ierror.full;
	  }
error_exit:
	if ( error_is_interrupt(ierror.code) )
	{	/* We must push the current object being interpreted */
		/* back on the e-stack so it will be re-executed. */
		/* Currently, this is always an executable operator, */
		/* but it might be something else someday if we check */
		/* for interrupts in the interpreter loop itself. */
		if ( iesp >= estop )
			code = e_execstackoverflow;
		else
		{	iesp++;
			ref_assign_inline(iesp, iref);
		}
	}
	esp = iesp;
	osp = iosp;
	ref_assign_inline(perror_object, ierror.obj);
	return gs_log_error(ierror.code, __FILE__, ierror.line);

}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(interp_op_defs) {
		/* Internal operators */
	{"0%interp_exit", interp_exit},
END_OP_DEFS(0) }
