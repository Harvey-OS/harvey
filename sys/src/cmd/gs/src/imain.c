/* Copyright (C) 1989, 1996, 1997, 1998, 1999, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: imain.c,v 1.41 2004/11/14 01:41:58 ghostgum Exp $ */
/* Common support for interpreter front ends */
#include "malloc_.h"
#include "memory_.h"
#include "string_.h"
#include "ghost.h"
#include "gp.h"
#include "gscdefs.h"		/* for gs_init_file */
#include "gslib.h"
#include "gsmatrix.h"		/* for gxdevice.h */
#include "gsutil.h"		/* for bytes_compare */
#include "gxdevice.h"
#include "gxalloc.h"
#include "gzstate.h"
#include "ierrors.h"
#include "oper.h"
#include "iconf.h"		/* for gs_init_* imports */
#include "idebug.h"
#include "idict.h"
#include "iname.h"		/* for name_init */
#include "dstack.h"
#include "estack.h"
#include "ostack.h"		/* put here for files.h */
#include "stream.h"		/* for files.h */
#include "files.h"
#include "ialloc.h"
#include "iinit.h"
#include "strimpl.h"		/* for sfilter.h */
#include "sfilter.h"		/* for iscan.h */
#include "iscan.h"
#include "main.h"
#include "store.h"
#include "isave.h"		/* for prototypes */
#include "interp.h"
#include "ivmspace.h"
#include "idisp.h"		/* for setting display device callback */
#include "iplugin.h"

/* ------ Exported data ------ */

/** using backpointers retrieve minst from any memory pointer 
 * 
 */
gs_main_instance* 
get_minst_from_memory(const gs_memory_t *mem)
{
#ifdef PSI_INCLUDED
    extern gs_main_instance *ps_impl_get_minst( const gs_memory_t *mem );
    return ps_impl_get_minst(mem);
#else
    return (gs_main_instance*)mem->gs_lib_ctx->top_of_system;  
#endif
}

/** construct main instance caller needs to retain */
gs_main_instance *
gs_main_alloc_instance(gs_memory_t *mem)
{
    gs_main_instance *minst = 0;
    if (mem) {
	minst = (gs_main_instance *) gs_alloc_bytes_immovable(mem, 
							      sizeof(gs_main_instance),
							      "init_main_instance");
	memcpy(minst, &gs_main_instance_init_values, sizeof(gs_main_instance_init_values));
	minst->heap = mem;
	
#       ifndef PSI_INCLUDED
	mem->gs_lib_ctx->top_of_system = minst;
        /* else top of system is pl_universe */
#       endif
    }
    return minst;
}

/* ------ Forward references ------ */

private int gs_run_init_file(gs_main_instance *, int *, ref *);
private void print_resource_usage(const gs_main_instance *,
				  gs_dual_memory_t *, const char *);

/* ------ Initialization ------ */

/* Initialization to be done before anything else. */
int
gs_main_init0(gs_main_instance * minst, FILE * in, FILE * out, FILE * err,
	      int max_lib_paths)
{
    ref *paths;

    /* Do platform-dependent initialization. */
    /* We have to do this as the very first thing, */
    /* because it detects attempts to run 80N86 executables (N>0) */
    /* on incompatible processors. */
    gp_init();

    /* Initialize the imager. */     
#   ifndef PSI_INCLUDED
       /* Reset debugging flags */
       memset(gs_debug, 0, 128);
       gs_log_errors = 0;  /* gs_debug['#'] = 0 */ 
#   else
       /* plmain settings remain in effect */
#   endif
    gp_get_usertime(minst->base_time);

    /* Initialize the file search paths. */
    paths = (ref *) gs_alloc_byte_array(minst->heap, max_lib_paths, sizeof(ref),
					"lib_path array");
    if (paths == 0) {
	gs_lib_finit(1, e_VMerror, minst->heap);
	return_error(e_VMerror);
    }
    make_array(&minst->lib_path.container, avm_foreign, max_lib_paths,
	       (ref *) gs_alloc_byte_array(minst->heap, max_lib_paths, sizeof(ref),
					   "lib_path array"));
    make_array(&minst->lib_path.list, avm_foreign | a_readonly, 0,
	       minst->lib_path.container.value.refs);
    minst->lib_path.env = 0;
    minst->lib_path.final = 0;
    minst->lib_path.count = 0;
    minst->user_errors = 1;
    minst->init_done = 0;
    return 0;
}

/* Initialization to be done before constructing any objects. */
int
gs_main_init1(gs_main_instance * minst)
{
    if (minst->init_done < 1) {
	gs_dual_memory_t idmem;
	int code =
	    ialloc_init(&idmem, minst->heap,
			minst->memory_chunk_size, gs_have_level2());

	if (code < 0)
	    return code;
	code = gs_lib_init1((gs_memory_t *)idmem.space_system);
	if (code < 0)
	    return code;
	alloc_save_init(&idmem);
	{
	    gs_memory_t *mem = (gs_memory_t *)idmem.space_system;
	    name_table *nt = names_init(minst->name_table_size,
					idmem.space_system);

	    if (nt == 0)
		return_error(e_VMerror);
	    mem->gs_lib_ctx->gs_name_table = nt;
	    code = gs_register_struct_root(mem, NULL,
					   (void **)&mem->gs_lib_ctx->gs_name_table,
					   "the_gs_name_table");
	    if (code < 0)
		return code;
	}
	code = obj_init(&minst->i_ctx_p, &idmem);  /* requires name_init */
	if (code < 0)
	    return code;
        code = i_plugin_init(minst->i_ctx_p);
	if (code < 0)
	    return code;
	minst->init_done = 1;
    }
    return 0;
}

/* Initialization to be done before running any files. */
private void
init2_make_string_array(i_ctx_t *i_ctx_p, const ref * srefs, const char *aname)
{
    const ref *ifp = srefs;
    ref ifa;

    for (; ifp->value.bytes != 0; ifp++);
    make_tasv(&ifa, t_array, a_readonly | avm_foreign,
	      ifp - srefs, const_refs, srefs);
    initial_enter_name(aname, &ifa);
}

/*
 * Invoke the interpreter, handling stdio callouts
 * e_NeedStdin, e_NeedStdout and e_NeedStderr.
 * We don't yet pass callouts all the way out because they
 * occur within gs_main_init2() and swproc().
 */
private int
gs_main_interpret(gs_main_instance *minst, ref * pref, int user_errors, 
	int *pexit_code, ref * perror_object)
{
    i_ctx_t *i_ctx_p;
    ref refnul;
    ref refpop;
    int code;

    /* set interpreter pointer to lib_path */
    minst->i_ctx_p->lib_path = &minst->lib_path;

    code = gs_interpret(&minst->i_ctx_p, pref, 
		user_errors, pexit_code, perror_object);
    while ((code == e_NeedStdin) || (code == e_NeedStdout) || 
	(code == e_NeedStderr)) {
        i_ctx_p = minst->i_ctx_p;
	if (code == e_NeedStdout) {
	    /*
	     * On entry:
	     *  esp[0]  = string, data to write to stdout
	     *  esp[-1] = bool, EOF (ignored)
	     *  esp[-2] = array, procedure (ignored)
	     *  esp[-3] = file, stdout stream
	     * We print the string then pop these 4 items.
	     */
	    if (r_type(&esp[0]) == t_string) {
		const char *str = (const char *)(esp[0].value.const_bytes); 
		int count = esp[0].tas.rsize;
		int rcode = 0;
		if (str != NULL)
		    rcode = outwrite(imemory, str, count);
		if (rcode < 0)
		    return_error(e_ioerror);
	    }

	    /* On return, we need to set 
	     *  osp[-1] = string buffer, 
	     *  osp[0] = file
	     */
	    gs_push_string(minst, (byte *)minst->stdout_buf, 
		sizeof(minst->stdout_buf), false);
	    gs_push_integer(minst, 0);	/* push integer */
	    osp[0] = esp[-3];		/* then replace with file */
	    /* remove items from execution stack */
	    esp -= 4;
	}
	else if (code == e_NeedStderr) {
	    if (r_type(&esp[0]) == t_string) {
		const char *str = (const char *)(esp[0].value.const_bytes); 
		int count = esp[0].tas.rsize;
		int rcode = 0;
		if (str != NULL)
		    rcode = errwrite(str, count);
		if (rcode < 0)
		    return_error(e_ioerror);
	    }
	    gs_push_string(minst, (byte *)minst->stderr_buf, 
		sizeof(minst->stderr_buf), false);
	    gs_push_integer(minst, 0);
	    osp[0] = esp[-3];
	    esp -= 4;
	}
	else if (code == e_NeedStdin) {
	    int count = sizeof(minst->stdin_buf);
	    /*
	     * On entry:
	     *  esp[0]  = array, procedure (ignored)
	     *  esp[-1] = file, stdin stream
	     * We read from stdin then pop these 2 items.
	     */
	    if (minst->heap->gs_lib_ctx->stdin_fn)
		count = (*minst->heap->gs_lib_ctx->stdin_fn)
		    (minst->heap->gs_lib_ctx->caller_handle, 
		     minst->stdin_buf, count);
	    else
		count = gp_stdin_read(minst->stdin_buf, count, 
				      minst->heap->gs_lib_ctx->stdin_is_interactive,
				      minst->heap->gs_lib_ctx->fstdin);
	    if (count < 0)
	        return_error(e_ioerror);

	    /* On return, we need to set 
	     *  osp[-1] = string buffer, 
	     *  osp[0] = file
	     */
	    gs_push_string(minst, (byte *)minst->stdin_buf, count, false);
	    gs_push_integer(minst, 0);	/* push integer */
	    osp[0] = esp[-1];		/* then replace with file */
	    /* remove items from execution stack */
	    esp -= 2;
	}
	/*
	 * To resume the interpreter, we call gs_interpret with a null ref.
	 * This copies the literal null onto the operand stack.
	 * To remove this we push a zpop onto the execution stack.
	 */
	make_null(&refnul);
	make_oper(&refpop, 0, zpop); 
	esp += 1;
	*esp = refpop;
	code = gs_interpret(&minst->i_ctx_p, &refnul, 
		    user_errors, pexit_code, perror_object);
    }
    return code;
}

int
gs_main_init2(gs_main_instance * minst)
{
    i_ctx_t *i_ctx_p;
    int code = gs_main_init1(minst);

    if (code < 0)
	return code;
    i_ctx_p = minst->i_ctx_p;
    if (minst->init_done < 2) {
	int code, exit_code;
	ref error_object;

	code = zop_init(i_ctx_p);
	if (code < 0)
	    return code;
	{
	    /*
	     * gs_iodev_init has to be called here (late), rather than
	     * with the rest of the library init procedures, because of
	     * some hacks specific to MS Windows for patching the
	     * stdxxx IODevices.
	     */
	    extern init_proc(gs_iodev_init);

	    code = gs_iodev_init(imemory);
	    if (code < 0)
		return code;
	}
	code = op_init(i_ctx_p);	/* requires obj_init */
	if (code < 0)
	    return code;

	/* Set up the array of additional initialization files. */
	init2_make_string_array(i_ctx_p, gs_init_file_array, "INITFILES");
	/* Set up the array of emulator names. */
	init2_make_string_array(i_ctx_p, gs_emulator_name_array, "EMULATORS");
	/* Pass the search path. */
	code = initial_enter_name("LIBPATH", &minst->lib_path.list);
	if (code < 0)
	    return code;

	/* Execute the standard initialization file. */
	code = gs_run_init_file(minst, &exit_code, &error_object);
	if (code < 0)
	    return code;
	minst->init_done = 2;
	i_ctx_p = minst->i_ctx_p; /* init file may change it */
	/* NB this is to be done with device parameters
	 * both minst->display and  display_set_callback() are going away
	*/
	if (minst->display)
	    code = display_set_callback(minst, minst->display);

	if (code < 0)
	    return code;
    }
    if (gs_debug_c(':'))
	print_resource_usage(minst, &gs_imemory, "Start");
    gp_readline_init(&minst->readline_data, imemory_system);
    return 0;
}

/* ------ Search paths ------ */

/* Internal routine to add a set of directories to a search list. */
/* Returns 0 or an error code. */
private int
file_path_add(gs_file_path * pfp, const char *dirs)
{
    uint len = r_size(&pfp->list);
    const char *dpath = dirs;

    if (dirs == 0)
	return 0;
    for (;;) {			/* Find the end of the next directory name. */
	const char *npath = dpath;

	while (*npath != 0 && *npath != gp_file_name_list_separator)
	    npath++;
	if (npath > dpath) {
	    if (len == r_size(&pfp->container))
		return_error(e_limitcheck);
	    make_const_string(&pfp->container.value.refs[len],
			      avm_foreign | a_readonly,
			      npath - dpath, (const byte *)dpath);
	    ++len;
	}
	if (!*npath)
	    break;
	dpath = npath + 1;
    }
    r_set_size(&pfp->list, len);
    return 0;
}

/* Add a library search path to the list. */
int
gs_main_add_lib_path(gs_main_instance * minst, const char *lpath)
{
    /* Account for the possibility that the first element */
    /* is gp_current_directory name added by set_lib_paths. */
    int first_is_here =
	(r_size(&minst->lib_path.list) != 0 &&
	 minst->lib_path.container.value.refs[0].value.bytes ==
	 (const byte *)gp_current_directory_name ? 1 : 0);
    int code;

    r_set_size(&minst->lib_path.list, minst->lib_path.count +
	       first_is_here);
    code = file_path_add(&minst->lib_path, lpath);
    minst->lib_path.count = r_size(&minst->lib_path.list) - first_is_here;
    if (code < 0)
	return code;
    return gs_main_set_lib_paths(minst);
}

/* ------ Execution ------ */

/* Complete the list of library search paths. */
/* This may involve adding or removing the current directory */
/* as the first element. */
int
gs_main_set_lib_paths(gs_main_instance * minst)
{
    ref *paths = minst->lib_path.container.value.refs;
    int first_is_here =
	(r_size(&minst->lib_path.list) != 0 &&
	 paths[0].value.bytes == (const byte *)gp_current_directory_name ? 1 : 0);
    int count = minst->lib_path.count;
    int code = 0;

    if (minst->search_here_first) {
	if (!(first_is_here ||
	      (r_size(&minst->lib_path.list) != 0 &&
	       !bytes_compare((const byte *)gp_current_directory_name,
			      strlen(gp_current_directory_name),
			      paths[0].value.bytes,
			      r_size(&paths[0]))))
	    ) {
	    memmove(paths + 1, paths, count * sizeof(*paths));
	    make_const_string(paths, avm_foreign | a_readonly,
			      strlen(gp_current_directory_name),
			      (const byte *)gp_current_directory_name);
	}
    } else {
	if (first_is_here)
	    memmove(paths, paths + 1, count * sizeof(*paths));
    }
    r_set_size(&minst->lib_path.list,
	       count + (minst->search_here_first ? 1 : 0));
    if (minst->lib_path.env != 0)
	code = file_path_add(&minst->lib_path, minst->lib_path.env);
    if (minst->lib_path.final != 0 && code >= 0)
	code = file_path_add(&minst->lib_path, minst->lib_path.final);
    return code;
}

/* Open a file, using the search paths. */
int
gs_main_lib_open(gs_main_instance * minst, const char *file_name, ref * pfile)
{
    /* This is a separate procedure only to avoid tying up */
    /* extra stack space while running the file. */
    i_ctx_t *i_ctx_p = minst->i_ctx_p;
#define maxfn 200
    byte fn[maxfn];
    uint len;

    return lib_file_open( &minst->lib_path,
			  NULL /* Don't check permissions here, because permlist 
				  isn't ready running init files. */
			  , file_name, strlen(file_name), fn, maxfn,
			  &len, pfile, imemory);
}

/* Open and execute a file. */
int
gs_main_run_file(gs_main_instance * minst, const char *file_name, int user_errors, int *pexit_code, ref * perror_object)
{
    ref initial_file;
    int code = gs_main_run_file_open(minst, file_name, &initial_file);

    if (code < 0)
	return code;
    return gs_main_interpret(minst, &initial_file, user_errors,
			pexit_code, perror_object);
}
int
gs_main_run_file_open(gs_main_instance * minst, const char *file_name, ref * pfref)
{
    gs_main_set_lib_paths(minst);
    if (gs_main_lib_open(minst, file_name, pfref) < 0) {
	eprintf1("Can't find initialization file %s.\n", file_name);
	return_error(e_Fatal);
    }
    r_set_attrs(pfref, a_execute + a_executable);
    return 0;
}

/* Open and run the very first initialization file. */
private int
gs_run_init_file(gs_main_instance * minst, int *pexit_code, ref * perror_object)
{
    i_ctx_t *i_ctx_p = minst->i_ctx_p;
    ref ifile;
    ref first_token;
    int code;
    scanner_state state;

    gs_main_set_lib_paths(minst);
    if (gs_init_string_sizeof == 0) {	/* Read from gs_init_file. */
	code = gs_main_run_file_open(minst, gs_init_file, &ifile);
    } else {			/* Read from gs_init_string. */
	code = file_read_string(gs_init_string, gs_init_string_sizeof, &ifile,
				iimemory);
    }
    if (code < 0) {
	*pexit_code = 255;
	return code;
    }
    /* Check to make sure the first token is an integer */
    /* (for the version number check.) */
    scanner_state_init(&state, false);
    code = scan_token(i_ctx_p, ifile.value.pfile, &first_token,
		      &state);
    if (code != 0 || !r_has_type(&first_token, t_integer)) {
	eprintf1("Initialization file %s does not begin with an integer.\n", gs_init_file);
	*pexit_code = 255;
	return_error(e_Fatal);
    }
    *++osp = first_token;
    r_set_attrs(&ifile, a_executable);
    return gs_main_interpret(minst, &ifile, minst->user_errors,
			pexit_code, perror_object);
}

/* Run a string. */
int
gs_main_run_string(gs_main_instance * minst, const char *str, int user_errors,
		   int *pexit_code, ref * perror_object)
{
    return gs_main_run_string_with_length(minst, str, (uint) strlen(str),
					  user_errors,
					  pexit_code, perror_object);
}
int
gs_main_run_string_with_length(gs_main_instance * minst, const char *str,
	 uint length, int user_errors, int *pexit_code, ref * perror_object)
{
    int code;

    code = gs_main_run_string_begin(minst, user_errors,
				    pexit_code, perror_object);
    if (code < 0)
	return code;
    code = gs_main_run_string_continue(minst, str, length, user_errors,
				       pexit_code, perror_object);
    if (code != e_NeedInput)
	return code;
    return gs_main_run_string_end(minst, user_errors,
				  pexit_code, perror_object);
}

/* Set up for a suspendable run_string. */
int
gs_main_run_string_begin(gs_main_instance * minst, int user_errors,
			 int *pexit_code, ref * perror_object)
{
    const char *setup = ".runstringbegin";
    ref rstr;
    int code;

    gs_main_set_lib_paths(minst);
    make_const_string(&rstr, avm_foreign | a_readonly | a_executable,
		      strlen(setup), (const byte *)setup);
    code = gs_main_interpret(minst, &rstr, user_errors, pexit_code,
			perror_object);
    return (code == e_NeedInput ? 0 : code == 0 ? e_Fatal : code);
}
/* Continue running a string with the option of suspending. */
int
gs_main_run_string_continue(gs_main_instance * minst, const char *str,
	 uint length, int user_errors, int *pexit_code, ref * perror_object)
{
    ref rstr;

    if (length == 0)
	return 0;		/* empty string signals EOF */
    make_const_string(&rstr, avm_foreign | a_readonly, length,
		      (const byte *)str);
    return gs_main_interpret(minst, &rstr, user_errors, pexit_code,
			perror_object);
}
/* Signal EOF when suspended. */
int
gs_main_run_string_end(gs_main_instance * minst, int user_errors,
		       int *pexit_code, ref * perror_object)
{
    ref rstr;

    make_empty_const_string(&rstr, avm_foreign | a_readonly);
    return gs_main_interpret(minst, &rstr, user_errors, pexit_code,
			perror_object);
}

/* ------ Operand stack access ------ */

/* These are built for comfort, not for speed. */

private int
push_value(gs_main_instance *minst, ref * pvalue)
{
    i_ctx_t *i_ctx_p = minst->i_ctx_p;
    int code = ref_stack_push(&o_stack, 1);

    if (code < 0)
	return code;
    *ref_stack_index(&o_stack, 0L) = *pvalue;
    return 0;
}

int
gs_push_boolean(gs_main_instance * minst, bool value)
{
    ref vref;

    make_bool(&vref, value);
    return push_value(minst, &vref);
}

int
gs_push_integer(gs_main_instance * minst, long value)
{
    ref vref;

    make_int(&vref, value);
    return push_value(minst, &vref);
}

int
gs_push_real(gs_main_instance * minst, floatp value)
{
    ref vref;

    make_real(&vref, value);
    return push_value(minst, &vref);
}

int
gs_push_string(gs_main_instance * minst, byte * chars, uint length,
	       bool read_only)
{
    ref vref;

    make_string(&vref, avm_foreign | (read_only ? a_readonly : a_all),
		length, (byte *) chars);
    return push_value(minst, &vref);
}

private int
pop_value(i_ctx_t *i_ctx_p, ref * pvalue)
{
    if (!ref_stack_count(&o_stack))
	return_error(e_stackunderflow);
    *pvalue = *ref_stack_index(&o_stack, 0L);
    return 0;
}

int
gs_pop_boolean(gs_main_instance * minst, bool * result)
{
    i_ctx_t *i_ctx_p = minst->i_ctx_p;
    ref vref;
    int code = pop_value(i_ctx_p, &vref);

    if (code < 0)
	return code;
    check_type_only(vref, t_boolean);
    *result = vref.value.boolval;
    ref_stack_pop(&o_stack, 1);
    return 0;
}

int
gs_pop_integer(gs_main_instance * minst, long *result)
{
    i_ctx_t *i_ctx_p = minst->i_ctx_p;
    ref vref;
    int code = pop_value(i_ctx_p, &vref);

    if (code < 0)
	return code;
    check_type_only(vref, t_integer);
    *result = vref.value.intval;
    ref_stack_pop(&o_stack, 1);
    return 0;
}

int
gs_pop_real(gs_main_instance * minst, float *result)
{
    i_ctx_t *i_ctx_p = minst->i_ctx_p;
    ref vref;
    int code = pop_value(i_ctx_p, &vref);

    if (code < 0)
	return code;
    switch (r_type(&vref)) {
	case t_real:
	    *result = vref.value.realval;
	    break;
	case t_integer:
	    *result = (float)(vref.value.intval);
	    break;
	default:
	    return_error(e_typecheck);
    }
    ref_stack_pop(&o_stack, 1);
    return 0;
}

int
gs_pop_string(gs_main_instance * minst, gs_string * result)
{
    i_ctx_t *i_ctx_p = minst->i_ctx_p;
    ref vref;
    int code = pop_value(i_ctx_p, &vref);

    if (code < 0)
	return code;
    switch (r_type(&vref)) {
	case t_name:
	    name_string_ref(minst->heap, &vref, &vref);
	    code = 1;
	    goto rstr;
	case t_string:
	    code = (r_has_attr(&vref, a_write) ? 0 : 1);
	  rstr:result->data = vref.value.bytes;
	    result->size = r_size(&vref);
	    break;
	default:
	    return_error(e_typecheck);
    }
    ref_stack_pop(&o_stack, 1);
    return code;
}

/* ------ Termination ------ */

/* Get the names of temporary files.
 * Each name is null terminated, and the last name is 
 * terminated by a double null.
 * We retrieve the names of temporary files just before
 * the interpreter finishes, and then delete the files 
 * after the interpreter has closed all files.
 */
private char *gs_main_tempnames(gs_main_instance *minst)
{
    i_ctx_t *i_ctx_p = minst->i_ctx_p;
    ref *SAFETY;
    ref *tempfiles;
    ref keyval[2];	/* for key and value */
    char *tempnames = NULL;
    int i;
    int idict;
    int len = 0;
    const byte *data = NULL;
    uint size;
    if (minst->init_done >= 2) {
        if (dict_find_string(systemdict, "SAFETY", &SAFETY) <= 0 ||
	    dict_find_string(SAFETY, "tempfiles", &tempfiles) <= 0)
	    return NULL;
	/* get lengths of temporary filenames */
	idict = dict_first(tempfiles);
	while ((idict = dict_next(tempfiles, idict, &keyval[0])) >= 0) {
	    if (obj_string_data(minst->heap, &keyval[0], &data, &size) >= 0)
		len += size + 1;
	}
	if (len != 0)
	    tempnames = (char *)malloc(len+1);
	if (tempnames) {
	    memset(tempnames, 0, len+1);
	    /* copy temporary filenames */
	    idict = dict_first(tempfiles);
	    i = 0;
	    while ((idict = dict_next(tempfiles, idict, &keyval[0])) >= 0) {
	        if (obj_string_data(minst->heap, &keyval[0], &data, &size) >= 0) {
		    memcpy(tempnames+i, (const char *)data, size);
		    i+= size;
		    tempnames[i++] = '\0';
		}
	    }
	}
    }
    return tempnames;
}

/* Free all resources and return. */
int
gs_main_finit(gs_main_instance * minst, int exit_status, int code)
{
    i_ctx_t *i_ctx_p = minst->i_ctx_p;
    int exit_code;
    ref error_object;
    char *tempnames;

    /* NB: need to free gs_name_table
     */

    /*
     * Previous versions of this code closed the devices in the
     * device list here.  Since these devices are now prototypes,
     * they cannot be opened, so they do not need to be closed;
     * alloc_restore_all will close dynamically allocated devices.
     */
    tempnames = gs_main_tempnames(minst);
    /* 
     * Close the "main" device, because it may need to write out
     * data before destruction. pdfwrite needs so.
     */
    if (minst->init_done >= 1) {
	int code;

	if (idmemory->reclaim != 0) {
	    code = interp_reclaim(&minst->i_ctx_p, avm_global);

	    if (code < 0) {
		eprintf1("ERROR %d reclaiming the memory while the interpreter finalization.\n", code);
		return e_Fatal;
	    }
	    i_ctx_p = minst->i_ctx_p; /* interp_reclaim could change it. */
	}
	if (i_ctx_p->pgs != NULL && i_ctx_p->pgs->device != NULL) {
	    gx_device *pdev = i_ctx_p->pgs->device;

	    /* deactivate the device just before we close it for the last time */
	    gs_main_run_string(minst, 
		".uninstallpagedevice "
		"serverdict /.jobsavelevel get 0 eq {/quit} {/stop} ifelse .systemvar exec",
		0 , &exit_code, &error_object);
	    code = gs_closedevice(pdev);
	    if (code < 0)
		eprintf2("ERROR %d closing the device. See gs/src/ierrors.h for code explanation.\n", code, i_ctx_p->pgs->device->dname);
	    if (exit_status == 0 || exit_status == e_Quit)
		exit_status = code;
	}
    }
    /* Flush stdout and stderr */
    if (minst->init_done >= 2)
      gs_main_run_string(minst, 
	"(%stdout) (w) file closefile (%stderr) (w) file closefile "
        "serverdict /.jobsavelevel get 0 eq {/quit} {/stop} ifelse .systemvar exec",
	0 , &exit_code, &error_object);
    gp_readline_finit(minst->readline_data);
    if (gs_debug_c(':'))
	print_resource_usage(minst, &gs_imemory, "Final");
    /* Do the equivalent of a restore "past the bottom". */
    /* This will release all memory, close all open files, etc. */
    if (minst->init_done >= 1) {
        gs_memory_t *mem_raw = i_ctx_p->memory.current->non_gc_memory;
        i_plugin_holder *h = i_ctx_p->plugin_list;
        alloc_restore_all(idmemory);
        i_plugin_finit(mem_raw, h);
    }
    /* clean up redirected stdout */
    if (minst->heap->gs_lib_ctx->fstdout2 
	&& (minst->heap->gs_lib_ctx->fstdout2 != minst->heap->gs_lib_ctx->fstdout)
	&& (minst->heap->gs_lib_ctx->fstdout2 != minst->heap->gs_lib_ctx->fstderr)) {
	fclose(minst->heap->gs_lib_ctx->fstdout2);
	minst->heap->gs_lib_ctx->fstdout2 = (FILE *)NULL;
    }
    minst->heap->gs_lib_ctx->stdout_is_redirected = 0;
    minst->heap->gs_lib_ctx->stdout_to_stderr = 0;
    /* remove any temporary files, after ghostscript has closed files */
    if (tempnames) {
	char *p = tempnames;
	while (*p) {
	    unlink(p);
	    p += strlen(p) + 1;
	}
	free(tempnames);
    }
    gs_lib_finit(exit_status, code, minst->heap);
    return exit_status;
}
int
gs_to_exit_with_code(const gs_memory_t *mem, int exit_status, int code)
{
    return gs_main_finit(get_minst_from_memory(mem), exit_status, code);
}
int
gs_to_exit(const gs_memory_t *mem, int exit_status)
{
    return gs_to_exit_with_code(mem, exit_status, 0);
}
void
gs_abort(const gs_memory_t *mem)
{
    gs_to_exit(mem, 1);
    /* it's fatal calling OS independent exit() */
    gp_do_exit(1);	
}

/* ------ Debugging ------ */

/* Print resource usage statistics. */
private void
print_resource_usage(const gs_main_instance * minst, gs_dual_memory_t * dmem,
		     const char *msg)
{
    ulong allocated = 0, used = 0;
    long utime[2];

    gp_get_usertime(utime);
    {
	int i;

	for (i = 0; i < countof(dmem->spaces_indexed); ++i) {
	    gs_ref_memory_t *mem = dmem->spaces_indexed[i];

	    if (mem != 0 && (i == 0 || mem != dmem->spaces_indexed[i - 1])) {
		gs_memory_status_t status;
		gs_ref_memory_t *mem_stable =
		    (gs_ref_memory_t *)gs_memory_stable((gs_memory_t *)mem);

		gs_memory_status((gs_memory_t *)mem, &status);
		allocated += status.allocated;
		used += status.used;
		if (mem_stable != mem) {
		    gs_memory_status((gs_memory_t *)mem_stable, &status);
		    allocated += status.allocated;
		    used += status.used;
		}
	    }
	}
    }
    dprintf4("%% %s time = %g, memory allocated = %lu, used = %lu\n",
	     msg, utime[0] - minst->base_time[0] +
	     (utime[1] - minst->base_time[1]) / 1000000000.0,
	     allocated, used);
}

/* Dump the stacks after interpretation */
void
gs_main_dump_stack(gs_main_instance *minst, int code, ref * perror_object)
{
    i_ctx_t *i_ctx_p = minst->i_ctx_p;

    zflush(i_ctx_p);		/* force out buffered output */
    dprintf1("\nUnexpected interpreter error %d.\n", code);
    if (perror_object != 0) {
	dputs("Error object: ");
	debug_print_ref(minst->heap, perror_object);
	dputc('\n');
    }
    debug_dump_stack(minst->heap, &o_stack, "Operand stack");
    debug_dump_stack(minst->heap, &e_stack, "Execution stack");
    debug_dump_stack(minst->heap, &d_stack, "Dictionary stack");
}

