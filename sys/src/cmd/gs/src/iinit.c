/* Copyright (C) 1989, 1995, 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: iinit.c,v 1.10 2004/08/04 19:36:13 stefan Exp $ */
/* Initialize internally known objects for Ghostscript interpreter */
#include "string_.h"
#include "ghost.h"
#include "gscdefs.h"
#include "gsexit.h"
#include "gsstruct.h"
#include "ierrors.h"
#include "ialloc.h"
#include "iddict.h"
#include "dstack.h"
#include "ilevel.h"
#include "iinit.h"
#include "iname.h"
#include "interp.h"
#include "ipacked.h"
#include "iparray.h"
#include "iutil.h"
#include "ivmspace.h"
#include "opdef.h"
#include "store.h"

/* Implementation parameters. */
/*
 * Define the (initial) sizes of the various system dictionaries.  We want
 * the sizes to be prime numbers large enough to cover all the operators,
 * plus everything in the init files, even if all the optional features are
 * selected.  Note that these sizes must be large enough to get us through
 * initialization, since we start up in Level 1 mode where dictionaries
 * don't expand automatically.
 */
/* The size of systemdict can be set in the makefile. */
#ifndef SYSTEMDICT_SIZE
#  define SYSTEMDICT_SIZE 631
#endif
#ifndef SYSTEMDICT_LEVEL2_SIZE
#  define SYSTEMDICT_LEVEL2_SIZE 983
#endif
#ifndef SYSTEMDICT_LL3_SIZE
#  define SYSTEMDICT_LL3_SIZE 1123
#endif
/* The size of level2dict, if applicable, can be set in the makefile. */
#ifndef LEVEL2DICT_SIZE
#  define LEVEL2DICT_SIZE 251
#endif
/* Ditto the size of ll3dict. */
#ifndef LL3DICT_SIZE
#  define LL3DICT_SIZE 43
#endif
/* Ditto the size of filterdict. */
#ifndef FILTERDICT_SIZE
#  define FILTERDICT_SIZE 43
#endif
/* Define an arbitrary size for the operator procedure tables. */
#ifndef OP_ARRAY_TABLE_SIZE
#  define OP_ARRAY_TABLE_SIZE 300
#endif
#ifndef OP_ARRAY_TABLE_GLOBAL_SIZE
#  define OP_ARRAY_TABLE_GLOBAL_SIZE OP_ARRAY_TABLE_SIZE
#endif
#ifndef OP_ARRAY_TABLE_LOCAL_SIZE
#  define OP_ARRAY_TABLE_LOCAL_SIZE (OP_ARRAY_TABLE_SIZE / 2)
#endif
#define OP_ARRAY_TABLE_TOTAL_SIZE\
  (OP_ARRAY_TABLE_GLOBAL_SIZE + OP_ARRAY_TABLE_LOCAL_SIZE)

/* Define the list of error names. */
const char *const gs_error_names[] =
{
    ERROR_NAMES
};

/* The operator tables */
op_array_table op_array_table_global, op_array_table_local;	/* definitions of `operator' procedures */

/* Enter a name and value into a dictionary. */
private int
i_initial_enter_name_in(i_ctx_t *i_ctx_p, ref *pdict, const char *nstr,
			const ref * pref)
{
    int code = idict_put_string(pdict, nstr, pref);

    if (code < 0)
	lprintf4("initial_enter failed (%d), entering /%s in -dict:%u/%u-\n",
		 code, nstr, dict_length(pdict), dict_maxlength(pdict));
    return code;
}
int
i_initial_enter_name(i_ctx_t *i_ctx_p, const char *nstr, const ref * pref)
{
    return i_initial_enter_name_in(i_ctx_p, systemdict, nstr, pref);
}

/* Remove a name from systemdict. */
void
i_initial_remove_name(i_ctx_t *i_ctx_p, const char *nstr)
{
    ref nref;

    if (name_ref(imemory, (const byte *)nstr, strlen(nstr), &nref, -1) >= 0)
	idict_undef(systemdict, &nref);
}

/* Define the names and sizes of the initial dictionaries. */
/* The names are used to create references in systemdict. */
const struct {
    const char *name;
    uint size;
    bool local;
} initial_dictionaries[] = {
#ifdef INITIAL_DICTIONARIES
    INITIAL_DICTIONARIES
#else
    /* systemdict is created and named automagically */
    {
	"level2dict", LEVEL2DICT_SIZE, false
    },
    {
	"ll3dict", LL3DICT_SIZE, false
    },
    {
	"globaldict", 0, false
    },
    {
	"userdict", 0, true
    },
    {
	"filterdict", FILTERDICT_SIZE, false
    },
#endif
};
/* systemdict and globaldict are magically inserted at the bottom */
const char *const initial_dstack[] =
{
#ifdef INITIAL_DSTACK
    INITIAL_DSTACK
#else
    "userdict"
#endif
};

#define MIN_DSTACK_SIZE (countof(initial_dstack) + 1)	/* +1 for systemdict */


/*
 * Detect whether we have any Level 2 or LanguageLevel 3 operators.
 * We export this for gs_init1 in imain.c.
 * This is slow, but we only call it a couple of times.
 */
private int
gs_op_language_level(void)
{
    const op_def *const *tptr;
    int level = 1;

    for (tptr = op_defs_all; *tptr != 0; ++tptr) {
	const op_def *def;

	for (def = *tptr; def->oname != 0; ++def)
	    if (op_def_is_begin_dict(def)) {
		if (!strcmp(def->oname, "level2dict"))
		    level = max(level, 2);
		else if (!strcmp(def->oname, "ll3dict"))
		    level = max(level, 3);
	    }
    }
    return level;
}
bool
gs_have_level2(void)
{
    return (gs_op_language_level() >= 2);
}

/* Create an initial dictionary if necessary. */
private ref *
make_initial_dict(i_ctx_t *i_ctx_p, const char *iname, ref idicts[])
{
    int i;

    /* systemdict was created specially. */
    if (!strcmp(iname, "systemdict"))
	return systemdict;
    for (i = 0; i < countof(initial_dictionaries); i++) {
	const char *dname = initial_dictionaries[i].name;
	const int dsize = initial_dictionaries[i].size;

	if (!strcmp(iname, dname)) {
	    ref *dref = &idicts[i];

	    if (r_has_type(dref, t_null)) {
		gs_ref_memory_t *mem =
		    (initial_dictionaries[i].local ?
		     iimemory_local : iimemory_global);
		int code = dict_alloc(mem, dsize, dref);

		if (code < 0)
		    return 0;	/* disaster */
	    }
	    return dref;
	}
    }

    /*
     * Name mentioned in some op_def, but not in initial_dictionaries.
     * Punt.
     */
    return 0;
}

/* Initialize objects other than operators.  In particular, */
/* initialize the dictionaries that hold operator definitions. */
int
obj_init(i_ctx_t **pi_ctx_p, gs_dual_memory_t *idmem)
{
    int level = gs_op_language_level();
    ref system_dict;
    i_ctx_t *i_ctx_p;
    int code;

    /*
     * Create systemdict.  The context machinery requires that
     * we do this before initializing the interpreter.
     */
    code = dict_alloc(idmem->space_global,
		      (level >= 3 ? SYSTEMDICT_LL3_SIZE :
		       level >= 2 ? SYSTEMDICT_LEVEL2_SIZE : SYSTEMDICT_SIZE),
		      &system_dict);
    if (code < 0)
	return code;

    /* Initialize the interpreter. */
    code = gs_interp_init(pi_ctx_p, &system_dict, idmem);
    if (code < 0)
	return code;
    i_ctx_p = *pi_ctx_p;

    {
#define icount countof(initial_dictionaries)
	ref idicts[icount];
	int i;
	const op_def *const *tptr;

	min_dstack_size = MIN_DSTACK_SIZE;

	refset_null(idicts, icount);

	/* Put systemdict on the dictionary stack. */
	if (level >= 2) {
	    dsp += 2;
	    /*
	     * For the moment, let globaldict be an alias for systemdict.
	     */
	    dsp[-1] = system_dict;
	    min_dstack_size++;
	} else {
	    ++dsp;
	}
	*dsp = system_dict;

	/* Create dictionaries which are to be homes for operators. */
	for (tptr = op_defs_all; *tptr != 0; tptr++) {
	    const op_def *def;

	    for (def = *tptr; def->oname != 0; def++)
		if (op_def_is_begin_dict(def)) {
		    if (make_initial_dict(i_ctx_p, def->oname, idicts) == 0)
			return_error(e_VMerror);
		}
	}

	/* Set up the initial dstack. */
	for (i = 0; i < countof(initial_dstack); i++) {
	    const char *dname = initial_dstack[i];

	    ++dsp;
	    if (!strcmp(dname, "userdict"))
		dstack_userdict_index = dsp - dsbot;
	    ref_assign(dsp, make_initial_dict(i_ctx_p, dname, idicts));
	}

	/* Enter names of referenced initial dictionaries into systemdict. */
	initial_enter_name("systemdict", systemdict);
	for (i = 0; i < icount; i++) {
	    ref *idict = &idicts[i];

	    if (!r_has_type(idict, t_null)) {
		/*
		 * Note that we enter the dictionary in systemdict
		 * even if it is in local VM.  There is a special
		 * provision in the garbage collector for this:
		 * see ivmspace.h for more information.
		 * In order to do this, we must temporarily
		 * identify systemdict as local, so that the
		 * store check in dict_put won't fail.
		 */
		uint save_space = r_space(systemdict);

		r_set_space(systemdict, avm_local);
		code = initial_enter_name(initial_dictionaries[i].name,
					  idict);
		r_set_space(systemdict, save_space);
		if (code < 0)
		    return code;
	    }
	}
#undef icount
    }

    gs_interp_reset(i_ctx_p);

    {
	ref vnull, vtrue, vfalse;

	make_null(&vnull);
	make_true(&vtrue);
	make_false(&vfalse);
	if ((code = initial_enter_name("null", &vnull)) < 0 ||
	    (code = initial_enter_name("true", &vtrue)) < 0 ||
	    (code = initial_enter_name("false", &vfalse)) < 0
	    )
	    return code;
    }

    /* Create the error name table */
    {
	int n = countof(gs_error_names) - 1;
	int i;
	ref era;

	code = ialloc_ref_array(&era, a_readonly, n, "ErrorNames");
	if (code < 0)
	    return code;
	for (i = 0; i < n; i++)
	  if ((code = name_enter_string(imemory, (const char *)gs_error_names[i],
					  era.value.refs + i)) < 0)
		return code;
	return initial_enter_name("ErrorNames", &era);
    }
}

/* Run the initialization procedures of the individual operator files. */
int
zop_init(i_ctx_t *i_ctx_p)
{
    const op_def *const *tptr;
    int code;

    /* Because of a bug in Sun's SC1.0 compiler, */
    /* we have to spell out the typedef for op_def_ptr here: */
    const op_def *def;

    for (tptr = op_defs_all; *tptr != 0; tptr++) {
	for (def = *tptr; def->oname != 0; def++)
	    DO_NOTHING;
	if (def->proc != 0) {
	    code = def->proc(i_ctx_p);
	    if (code < 0) {
		lprintf2("op_init proc 0x%lx returned error %d!\n",
			 (ulong)def->proc, code);
		return code;
	    }
	}
    }

    /* Initialize the predefined names other than operators. */
    /* Do this here in case op_init changed any of them. */
    {
	ref vcr, vpr, vpf, vre, vrd;

	make_const_string(&vcr, a_readonly | avm_foreign,
			  strlen(gs_copyright), (const byte *)gs_copyright);
	make_const_string(&vpr, a_readonly | avm_foreign,
			  strlen(gs_product), (const byte *)gs_product);
	make_const_string(&vpf, a_readonly | avm_foreign,
			  strlen(gs_productfamily),
			  (const byte *)gs_productfamily);
	make_int(&vre, gs_revision);
	make_int(&vrd, gs_revisiondate);
	if ((code = initial_enter_name("copyright", &vcr)) < 0 ||
	    (code = initial_enter_name("product", &vpr)) < 0 ||
	    (code = initial_enter_name("productfamily", &vpf)) < 0 ||
	    (code = initial_enter_name("revision", &vre)) < 0 ||
	    (code = initial_enter_name("revisiondate", &vrd)) < 0)
	    return code;
    }

    return 0;
}

/* Create an op_array table. */
private int
alloc_op_array_table(i_ctx_t *i_ctx_p, uint size, uint space,
		     op_array_table *opt)
{
    uint save_space = ialloc_space(idmemory);
    int code;

    ialloc_set_space(idmemory, space);
    code = ialloc_ref_array(&opt->table, a_readonly, size,
			    "op_array table");
    ialloc_set_space(idmemory, save_space);
    if (code < 0)
	return code;
    refset_null(opt->table.value.refs, size);
    opt->nx_table =
	(ushort *) ialloc_byte_array(size, sizeof(ushort),
				     "op_array nx_table");
    if (opt->nx_table == 0)
	return_error(e_VMerror);
    opt->count = 0;
    opt->root_p = &opt->table;
    opt->attrs = space | a_executable;
    return 0;
}

/* Initialize the operator table. */
int
op_init(i_ctx_t *i_ctx_p)
{
    const op_def *const *tptr;
    int code;

    /* Enter each operator into the appropriate dictionary. */

    for (tptr = op_defs_all; *tptr != 0; tptr++) {
	ref *pdict = systemdict;
	const op_def *def;
	const char *nstr;

	for (def = *tptr; (nstr = def->oname) != 0; def++)
	    if (op_def_is_begin_dict(def)) {
		ref nref;

		code = name_ref(imemory, (const byte *)nstr, strlen(nstr), &nref, -1);
		if (code < 0)
		    return code;
		if (!dict_find(systemdict, &nref, &pdict))
		    return_error(e_Fatal);
		if (!r_has_type(pdict, t_dictionary))
		    return_error(e_Fatal);
	    } else {
		ref oper;
		uint index_in_table = def - *tptr;
		uint opidx = (tptr - op_defs_all) * OP_DEFS_MAX_SIZE +
		    index_in_table;

		if (index_in_table >= OP_DEFS_MAX_SIZE) {
		    lprintf1("opdef overrun! %s\n", def->oname);
		    return_error(e_Fatal);
		}
		gs_interp_make_oper(&oper, def->proc, opidx);
		/* The first character of the name is a digit */
		/* giving the minimum acceptable number of operands. */
		/* Check to make sure it's within bounds. */
		if (*nstr - '0' > gs_interp_max_op_num_args)
		    return_error(e_Fatal);
		nstr++;
		/*
		 * Skip internal operators, and the second occurrence of
		 * operators with special indices.
		 */
		if (*nstr != '%' && r_size(&oper) == opidx) {
		    code =
			i_initial_enter_name_in(i_ctx_p, pdict, nstr, &oper);
		    if (code < 0)
			return code;
		}
	    }
    }
    /* Allocate the tables for `operator' procedures. */
    /* Make one of them local so we can have local operators. */

    if ((code = alloc_op_array_table(i_ctx_p, OP_ARRAY_TABLE_GLOBAL_SIZE,
				     avm_global, &op_array_table_global) < 0))
	return code;
    op_array_table_global.base_index = op_def_count;
    if ((code = gs_register_ref_root(imemory, NULL,
				     (void **)&op_array_table_global.root_p,
				     "op_array_table(global)")) < 0 ||
	(code = gs_register_struct_root(imemory, NULL,
				(void **)&op_array_table_global.nx_table,
					"op_array nx_table(global)")) < 0 ||
	(code = alloc_op_array_table(i_ctx_p, OP_ARRAY_TABLE_LOCAL_SIZE,
				     avm_local, &op_array_table_local) < 0)
	)
	return code;
    op_array_table_local.base_index =
	op_array_table_global.base_index +
	r_size(&op_array_table_global.table);
    if ((code = gs_register_ref_root(imemory, NULL,
				     (void **)&op_array_table_local.root_p,
				     "op_array_table(local)")) < 0 ||
	(code = gs_register_struct_root(imemory, NULL,
				(void **)&op_array_table_local.nx_table,
					"op_array nx_table(local)")) < 0
	)
	return code;

    return 0;
}
