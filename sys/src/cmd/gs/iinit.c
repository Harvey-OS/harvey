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

/* iinit.c */
/* Initialize internally known objects for Ghostscript interpreter */
#include "string_.h"
#include "ghost.h"
#include "gscdefs.h"
#include "gsexit.h"
#include "gsstruct.h"
#define INCLUDE_ERROR_NAMES		/* see errors.h */
#include "errors.h"
#include "ialloc.h"
#include "idict.h"
#include "dstack.h"
#include "ilevel.h"
#include "iname.h"
#include "interp.h"
#include "ipacked.h"
#include "iparray.h"
#include "ivmspace.h"
#include "oper.h"
#include "store.h"

/* Implementation parameters. */
/* The size of systemdict can be set in the makefile. */
/* We want the sizes to be prime numbers large enough to cover */
/* all the operators, plus everything in the init files, */
/* even if all the optional features are selected. */
#ifndef SYSTEMDICT_SIZE
#  define SYSTEMDICT_SIZE 479
#endif
#ifndef SYSTEMDICT_LEVEL2_SIZE
#  define SYSTEMDICT_LEVEL2_SIZE 631
#endif
/* The size of level2dict, if applicable, can be set in the makefile. */
#ifndef LEVEL2DICT_SIZE
#  define LEVEL2DICT_SIZE 149
#endif
/* Ditto the size of filterdict. */
#ifndef FILTERDICT_SIZE
#  define FILTERDICT_SIZE 31
#endif
/* Define an arbitrary size for the pseudo-operator table. */
#ifndef OP_ARRAY_TABLE_SIZE
#  define OP_ARRAY_TABLE_SIZE 100
#endif

/* Set up the .ps file name string array. */
/* We fill in the lengths at initialization time. */
#define ref_(t) struct { struct tas_s tas; t value; }
#define string_(s)\
 { { (t_string<<r_type_shift) + a_readonly + avm_foreign, 0 }, s },
#define psfile_(fns) string_(fns)
ref_(const char *) gs_init_file_array[] = {
#include "gconfig.h"
	string_(0)
};
#undef psfile_

/* The operator tables */
/* Because of a bug in Sun's SC1.0 compiler, */
/* we have to spell out the typedef for op_def_ptr here: */
const op_def **op_def_table;
uint op_def_count;
ref op_array_table;	/* t_array, definitions of `operator' procedures */
ushort *op_array_nx_table;	/* name indices for same */
byte *op_array_attrs_table;	/* space attributes for same (see opdef.h) */
uint op_array_count;
/* GC roots for the same */
private ref *op_array_p = &op_array_table;
private gs_gc_root_t
  op_def_root, op_array_root, op_array_nx_root, op_array_attrs_root;

/* Enter a name and value into a dictionary. */
void
initial_enter_name_in(const char *nstr, const ref *pref, ref *pdict)
{	int code = dict_put_string(pdict, nstr, pref);
	if ( code < 0 )
	  {	lprintf4("initial_enter failed (%d), entering /%s in -dict:%u/%u-\n",
			 code, nstr, dict_length(pdict), dict_maxlength(pdict));
		gs_exit(1);
	  }
}
void
initial_enter_name(const char *nstr, const ref *pref)
{	initial_enter_name_in(nstr, pref, systemdict);
}

/* Create a name.  Fatal error if it fails. */
private void
name_enter(const char *str, ref *pref)
{	if ( name_enter_string(str, pref) != 0 )
	  {	lprintf1("name_enter failed - %s\n", str);
		gs_exit(1);
	  }
}

/* Initialize the operators. */
extern op_def_ptr
	/* Initialization operators */
  gsmain_op_defs(P0()),
#define oper_(defs) defs(P0()),
#include "gconfig.h"
#undef oper_
	/* Interpreter operators */
  interp_op_defs(P0());
private op_def_ptr (*(op_defs_all[]))(P0()) = {
	/* Initialization operators */
  gsmain_op_defs,
#define oper_(defs) defs,
#include "gconfig.h"
#undef oper_
	/* Interpreter operators */
  interp_op_defs,
	/* end marker */
  0
};

/* Define the names and sizes of the initial dictionaries. */
/* The names are used to create references in systemdict. */
const struct { const char *name; uint size; bool local; }
  initial_dictionaries[] = {
#ifdef INITIAL_DICTIONARIES
    INITIAL_DICTIONARIES
#else
    /* systemdict is created and named automagically */
    { "level2dict", LEVEL2DICT_SIZE, false },
    { "globaldict", 0, false },
    { "userdict", 0, true },
    { "filterdict", FILTERDICT_SIZE, false }
#endif
};
/* systemdict and globaldict are magically inserted at the bottom */
const char *initial_dstack[] = {
#ifdef INITIAL_DSTACK
    INITIAL_DSTACK
#else
    "userdict"
#endif
};
#define MIN_DSTACK_SIZE (countof(initial_dstack) + 1) /* +1 for systemdict */


/* Detect whether we have any Level 2 operators. */
/* We export this for gs_init1 in gsmain.c. */
bool
gs_have_level2(void)
{	int i;
	for ( i = 0; i < countof(initial_dictionaries); i++ )
	  if ( !strcmp(initial_dictionaries[i].name, "level2dict") )
	    return true;
	return false;
}

/* Create an initial dictionary if necessary. */
private ref *
make_initial_dict(const char *iname, uint namelen, ref idicts[])
{	int i;
	static const char sysname[] = "systemdict";

	/*
	 * We wait to create systemdict until after everything else,
	 * so we can size it according to whether Level 2 is present.
	 */
	if ( (namelen == countof(sysname) - 1) &&
	     !memcmp(iname, sysname, namelen)
	   )
	  return 0;

	for ( i = 0; i < countof(initial_dictionaries); i++ )
	  {	const char *dname = initial_dictionaries[i].name;
		const int dsize = initial_dictionaries[i].size;

		if ( (namelen == strlen(dname)) &&
		     !memcmp(iname, dname, namelen)
		   )
		  {	ref *dref = &idicts[i];
			if ( r_has_type(dref, t_null) )
			  {	int code;
				/* Perhaps dict_create should take */
				/* the allocator as an argument.... */
				uint space = ialloc_space(idmemory);
				ialloc_set_space(idmemory,
					(initial_dictionaries[i].local ?
					 avm_local : avm_global));
				code = dict_create(dsize, dref);
				ialloc_set_space(idmemory, space);
				if ( code < 0 ) gs_abort();
			  }
			return dref;
		  }
	  }

	/*
	 * Name mentioned in some op_def,
	 * but not in initial_dictionaries.
	 */
	gs_abort();
}

/* Initialize objects other than operators.  In particular, */
/* initialize the dictionaries that hold operator definitions. */
void
obj_init(void)
{	uint space = ialloc_space(idmemory);
	bool level2 = gs_have_level2();

	/* Initialize the language level. */
	make_int(&ref_language_level, 1);

	/* Initialize the interpreter. */
	gs_interp_init();

	{
#define icount countof(initial_dictionaries)
	  ref idicts[icount];
	  int i;
	  op_def_ptr (**tptr)(P0());

	  min_dstack_size = MIN_DSTACK_SIZE;

	  refset_null(idicts, icount);

	  ialloc_set_space(idmemory, avm_global);
	  if ( level2 )
	    {	dsp += 2;
		dict_create(SYSTEMDICT_LEVEL2_SIZE, dsp);
		/* For the moment, let globaldict be an alias */
		/* for systemdict. */
		dsp[-1] = *dsp;
		min_dstack_size++;
	    }
	  else
	    {	++dsp;
		dict_create(SYSTEMDICT_SIZE, dsp);
	    }
	  ref_systemdict = *dsp;
	  ialloc_set_space(idmemory, space);

	  /* Create dictionaries which are to be homes for operators. */
	  for ( tptr = op_defs_all; *tptr != 0; tptr++ )
	    {	const op_def *def;
		for ( def = (*tptr)(); def->oname != 0; def++ )
		  if ( op_def_is_begin_dict(def) )
		    make_initial_dict(def->oname, strlen(def->oname), idicts);
	    }

	  /* Set up the initial dstack. */
	  for ( i = 0; i < countof(initial_dstack); i++ )
	    {	const char *dname = initial_dstack[i];
		++dsp;
		ref_assign(dsp, 
			   make_initial_dict(dname, strlen(dname), idicts));
	    }

	  /* Enter names of referenced initial dictionaries into systemdict. */
	  initial_enter_name("systemdict", &ref_systemdict);
	  for (i = 0; i < icount; i++)
	    {	ref *idict = &idicts[i];
		if ( !r_has_type(idict, t_null) )
		  {	/*
			 * Note that we enter the dictionary in systemdict
			 * even if it is in local VM.  There is a special
			 * provision in the garbage collector for this:
			 * see ivmspace.h for more information.
			 * In order to do this, we must temporarily
			 * identify systemdict as local, so that the
			 * store check in dict_put won't fail.
			 */
			r_set_space(systemdict, avm_local);
			initial_enter_name(initial_dictionaries[i].name,
					   idict);
			r_set_space(systemdict, avm_global);
		  }
	    }
#undef icount
	}

	gs_interp_reset();

	{	ref vtemp;
		make_null(&vtemp);
		initial_enter_name("null", &vtemp);
	}

	/* Create the error name table */
	{	int n = countof(gs_error_names) - 1;
		int i;
		ref era;
		ialloc_ref_array(&era, a_readonly, n, "ErrorNames");
		for ( i = 0; i < n; i++ )
		  name_enter((char *)gs_error_names[i], era.value.refs + i);
		dict_put_string(systemdict, "ErrorNames", &era);
	}
}

/* Run the initialization procedures of the individual operator files. */
void
zop_init(void)
{	op_def_ptr (**tptr)(P0());
	/* Because of a bug in Sun's SC1.0 compiler, */
	/* we have to spell out the typedef for op_def_ptr here: */
	const op_def *def;
	for ( tptr = op_defs_all; *tptr != 0; tptr++ )
	  {	for ( def = (*tptr)(); def->oname != 0; def++ )
		  DO_NOTHING;
		if ( def->proc != 0 )
		  ((void (*)(P0()))(def->proc))();
	  }

	/* Initialize the predefined names other than operators. */
	/* Do this here in case op_init changed any of them. */
	{	ref vtemp;
		make_const_string(&vtemp, a_readonly | avm_foreign,
				  strlen(gs_copyright),
				  (const byte *)gs_copyright);
		initial_enter_name("copyright", &vtemp);
		make_const_string(&vtemp, a_readonly | avm_foreign,
				  strlen(gs_product),
				  (const byte *)gs_product);
		initial_enter_name("product", &vtemp);
		make_int(&vtemp, gs_revision);
		initial_enter_name("revision", &vtemp);
		make_int(&vtemp, gs_revisiondate);
		initial_enter_name("revisiondate", &vtemp);
	}
}
/* Initialize the operator table. */
void
op_init(void)
{	int count = 1;
	op_def_ptr (**tptr)(P0());
	/* Because of a bug in Sun's SC1.0 compiler, */
	/* we have to spell out the typedef for op_def_ptr here: */
	const op_def *def;
	const char _ds *nstr;

	/* Do a first pass just to count the operators. */

	for ( tptr = op_defs_all; *tptr != 0; tptr ++ )
	{	for ( def = (*tptr)(); def->oname != 0; def++ )
		  if ( !op_def_is_begin_dict(def) )
		    count++;
	}
	
	/* Do a second pass to construct the operator table, */
	/* and enter the operators into the appropriate dictionary. */

	/* Because of a bug in Sun's SC1.0 compiler, */
	/* we have to spell out the typedef for op_def_ptr here: */
	op_def_table =
	  (const op_def **)ialloc_byte_array(count, sizeof(op_def_ptr),
					     "op_init(op_def_table)");
	op_def_count = count;
	for (count = 0; count <= gs_interp_num_special_ops; count++)
	  op_def_table[count] = 0;
	count = gs_interp_num_special_ops + 1; /* leave space for magic entries */
	for ( tptr = op_defs_all; *tptr != 0; tptr ++ )
	  {	ref *pdict = systemdict;
		for ( def = (*tptr)(); (nstr = def->oname) != 0; def++ )
		  if ( op_def_is_begin_dict(def) )
		    {	int code;
			ref nref;
			code = name_ref((byte *)nstr, strlen(nstr), &nref, -1);
			if ( code != 0 ) gs_abort();
			if ( !dict_find(systemdict, &nref, &pdict)) gs_abort();
			if ( !r_has_type(pdict, t_dictionary) ) gs_abort();
		    }
		  else
		    {	ref oper;
			uint opidx;
			gs_interp_make_oper(&oper, def->proc, count);
			opidx = r_size(&oper);
			/* The first character of the name is a digit */
			/* giving the minimum acceptable number of operands. */
			/* Check to make sure it's within bounds. */
			if ( *nstr - '0' > gs_interp_max_op_num_args )
			  gs_abort();
			nstr++;
			/* Don't enter internal operators into */
			/* the dictionary. */
			if ( *nstr != '%' )
			  initial_enter_name_in(nstr, &oper, pdict);
			op_def_table[opidx] = def;
			if ( opidx == count )
			  count++;
		    }
	  }
	/* All of the built-ins had better be defined somewhere, */
	/* or things like op_find_index will choke. */
	for ( count = 1; count <= gs_interp_num_special_ops; count++ )
	  if ( op_def_table[count] == 0 )
	    gs_abort();
	gs_register_struct_root(imemory, &op_def_root,
				(void **)&op_def_table, "op_def_table");

	/* Allocate the table for `operator' procedures. */
	/* Make this local so we can have local operators. */

	{	uint space = ialloc_space(idmemory);
		ialloc_set_space(idmemory, avm_local);
		ialloc_ref_array(&op_array_table, a_readonly,
				 OP_ARRAY_TABLE_SIZE, "op_array table");
		ialloc_set_space(idmemory, space);
	 }
	refset_null(op_array_table.value.refs, OP_ARRAY_TABLE_SIZE);
	gs_register_ref_root(imemory, &op_array_root, (void **)&op_array_p,
			     "op_array_table");

	op_array_nx_table =
	  (ushort *)ialloc_byte_array(OP_ARRAY_TABLE_SIZE, sizeof(ushort),
				      "op_array nx table");
	gs_register_struct_root(imemory, &op_array_nx_root,
				(void **)&op_array_nx_table,
				"op_array nx table");

	op_array_attrs_table =
	  (byte *)ialloc_byte_array(OP_ARRAY_TABLE_SIZE, 1,
				    "op_array attrs table");
	gs_register_struct_root(imemory, &op_array_attrs_root,
				(void **)&op_array_attrs_table,
				"op_array attrs table");

	op_array_count = 0;

}
