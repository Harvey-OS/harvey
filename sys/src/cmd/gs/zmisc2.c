/* Copyright (C) 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zmisc2.c */
/* Miscellaneous Level 2 operators */
#include "memory_.h"
#include "string_.h"
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gscdefs.h"
#include "gsstruct.h"		/* for gxht.h */
#include "gsfont.h"		/* for user params */
#include "gxht.h"		/* for user params */
#include "gsutil.h"
#include "estack.h"
#include "ialloc.h"		/* for imemory for status */
#include "idict.h"
#include "idparam.h"
#include "iparam.h"
#include "dstack.h"
#include "ilevel.h"
#include "iname.h"
#include "iutil2.h"
#include "ivmspace.h"
#include "store.h"

/* The (global) font directory */
extern gs_font_dir *ifont_dir;		/* in zfont.c */

/* Import the GC parameters from zvmem2.c. */
extern int set_vm_reclaim(P1(long));
extern int set_vm_threshold(P1(long));

/* Import the Level 1 'where' operator from zdict.c. */
extern int zwhere(P1(os_ptr));

/* The system passwords. */
private password StartJobPassword = NULL_PASSWORD;
password SystemParamsPassword = NULL_PASSWORD;	/* exported for ziodev2.c. */

/* Define an individual user or system parameter. */
/* Eventually this will be made public. */
#define param_def_common\
	const char _ds *pname
typedef struct param_def_s {
	param_def_common;
} param_def;
typedef struct long_param_def_s {
	param_def_common;
	long min_value, max_value;
	long (*current)(P0());
	int (*set)(P1(long));
} long_param_def;
#if arch_sizeof_long > arch_sizeof_int
#  define max_uint_param max_uint
#else
#  define max_uint_param max_long
#endif
typedef struct bool_param_def_s {
	param_def_common;
	bool (*current)(P0());
	int (*set)(P1(bool));
} bool_param_def;
typedef struct string_param_def_s {
	param_def_common;
	void (*current)(P1(gs_param_string *));
	int (*set)(P1(gs_param_string *));
} string_param_def;
/* Define a parameter set (user or system). */
typedef struct param_set_s {
	const long_param_def *long_defs;
	  uint long_count;
	const bool_param_def *bool_defs;
	  uint bool_count;
	const string_param_def *string_defs;
	  uint string_count;
} param_set;

/* Forward references */
private int set_language_level(P1(int));
private int currentparams(P2(os_ptr, const param_set _ds *));

/* ------ Language level operators ------ */

/* - .languagelevel <1 or 2> */
private int
zlanguagelevel(register os_ptr op)
{	push(1);
	ref_assign(op, &ref_language_level);
	return 0;
}

/* <1 or 2> .setlanguagelevel - */
private int
zsetlanguagelevel(register os_ptr op)
{	int code = 0;
	check_type(*op, t_integer);
	if ( op->value.intval < 1 || op->value.intval > 2 )
		return_error(e_rangecheck);
	if ( op->value.intval != ref_language_level.value.intval )
	{	code = set_language_level((int)op->value.intval);
		if ( code < 0 ) return code;
	}
	pop(1);
	ref_assign_old(NULL, &ref_language_level, op, "setlanguagelevel");
	return code;
}

/* ------ Passwords ------ */

#define plist ((gs_param_list *)&list)

/* <string|int> .checkpassword <0|1|2> */
private int
zcheckpassword(register os_ptr op)
{	ref params[2];
	array_param_list list;
	int result = 0;
	int code = name_ref((const byte *)"Password", 8, &params[0], 0);
	if ( code < 0 )
	  return code;
	params[1] = *op;
	array_param_list_read(&list, params, 2, NULL, false);
	if ( param_check_password(plist, &StartJobPassword) == 0 )
		result = 1;
	if ( param_check_password(plist, &SystemParamsPassword) == 0 )
		result = 2;
	make_int(op, result);
	return 0;
}

/* ------ System parameters ------ */

/* Integer values */
private long
current_BuildTime(void)
{	return gs_buildtime;
}
private long
current_MaxFontCache(void)
{	uint cstat[7];
	gs_cachestatus(ifont_dir, cstat);
	return cstat[1];
}
private long
current_CurFontCache(void)
{	uint cstat[7];
	gs_cachestatus(ifont_dir, cstat);
	return cstat[0];
}
private const long_param_def system_long_params[] = {
	{"BuildTime", 0, max_uint_param, current_BuildTime, NULL},
	{"MaxFontCache", 0, max_uint_param, current_MaxFontCache, NULL},
	{"CurFontCache", 0, max_uint_param, current_CurFontCache, NULL}
};
/* Boolean values */
private bool
current_ByteOrder(void)
{	return !arch_is_big_endian;
}
private const bool_param_def system_bool_params[] = {
	{"ByteOrder", current_ByteOrder, NULL}
};
/* String values */
private void
current_RealFormat(gs_param_string *pval)
{
#if arch_floats_are_IEEE
	static const char *rfs = "IEEE";
#else
	static const char *rfs = "not IEEE";
#endif
	pval->data = (const byte *)rfs;
	pval->size = strlen(rfs);
	pval->persistent = true;
}
private const string_param_def system_string_params[] = {
	{"RealFormat", current_RealFormat, NULL}
};

/* The system parameter set */
private const param_set system_param_set = {
	system_long_params, countof(system_long_params),
	system_bool_params, countof(system_bool_params),
	system_string_params, countof(system_string_params)
};


/* <dict> setsystemparams - */
private int
zsetsystemparams(register os_ptr op)
{	int code;
	dict_param_list list;
	int ival;
	password pass;
	code = dict_param_list_read(&list, op, NULL, false);
	if ( code < 0 )
	  return code;
	code = param_check_password(plist, &SystemParamsPassword);
	if ( code != 0 )
	  return_error(code < 0 ? code : e_invalidaccess);
	code = param_read_password(plist, "StartJobPassword", &pass);
	switch ( code )
	{
	default:			/* invalid */
		return code;
	case 1:				/* missing */
		break;
	case 0:
		StartJobPassword = pass;
	}
	code = param_read_password(plist, "SystemParamsPassword", &pass);
	switch ( code )
	{
	default:			/* invalid */
		return code;
	case 1:				/* missing */
		break;
	case 0:
		SystemParamsPassword = pass;
	}
	code = dict_int_param(op, "MaxFontCache", 0, max_int, 0, &ival);
	switch ( code )
	{
	default:			/* invalid */
		return code;
	case 1:				/* missing */
		break;
	case 0:
		/****** NOT IMPLEMENTED YET ******/
		;
	}
	pop(1);
	return 0;
}

/* - .currentsystemparams <name1> <value1> ... */
private int
zcurrentsystemparams(os_ptr op)
{	return currentparams(op, &system_param_set);
}

/* ------ User parameters ------ */

/* Integer values */
private long
current_JobTimeout(void)
{	return 0;
}
private int
set_JobTimeout(long val)
{	return 0;
}
private long
current_MaxFontItem(void)
{	return gs_currentcacheupper(ifont_dir);
}
private int
set_MaxFontItem(long val)
{	return gs_setcacheupper(ifont_dir, val);
}
private long
current_MinFontCompress(void)
{	return gs_currentcachelower(ifont_dir);
}
private int
set_MinFontCompress(long val)
{	return gs_setcachelower(ifont_dir, val);
}
private long
current_MaxOpStack(void)
{	return ref_stack_max_count(&o_stack);
}
private int
set_MaxOpStack(long val)
{	return ref_stack_set_max_count(&o_stack, val);
}
private long
current_MaxDictStack(void)
{	return ref_stack_max_count(&d_stack);
}
private int
set_MaxDictStack(long val)
{	return ref_stack_set_max_count(&d_stack, val);
}
private long
current_MaxExecStack(void)
{	return ref_stack_max_count(&e_stack);
}
private int
set_MaxExecStack(long val)
{	return ref_stack_set_max_count(&e_stack, val);
}
private long
current_MaxLocalVM(void)
{	gs_memory_gc_status_t stat;
	gs_memory_gc_status(iimemory_local, &stat);
	return stat.max_vm;
}
private int
set_MaxLocalVM(long val)
{	gs_memory_gc_status_t stat;
	gs_memory_gc_status(iimemory_local, &stat);
	stat.max_vm = max(val, 0);
	gs_memory_set_gc_status(iimemory_local, &stat);
	return 0;
}
private long
current_VMReclaim(void)
{	gs_memory_gc_status_t gstat, lstat;
	gs_memory_gc_status(iimemory_global, &gstat);
	gs_memory_gc_status(iimemory_local, &lstat);
	return (!gstat.enabled ? -2 : !lstat.enabled ? -1 : 0);
}
private long
current_VMThreshold(void)
{	gs_memory_gc_status_t stat;
	gs_memory_gc_status(iimemory_local, &stat);
	return stat.vm_threshold;
}
#define current_WaitTimeout current_JobTimeout
#define set_WaitTimeout set_JobTimeout
private const long_param_def user_long_params[] = {
	{"JobTimeout", 0, max_uint_param,
	   current_JobTimeout, set_JobTimeout},
	{"MaxFontItem", 0, max_uint_param,
	   current_MaxFontItem, set_MaxFontItem},
	{"MinFontCompress", 0, max_uint_param,
	   current_MinFontCompress, set_MinFontCompress},
	{"MaxOpStack", 0, max_uint_param,
	   current_MaxOpStack, set_MaxOpStack},
	{"MaxDictStack", 0, max_uint_param,
	   current_MaxDictStack, set_MaxDictStack},
	{"MaxExecStack", 0, max_uint_param,
	   current_MaxExecStack, set_MaxExecStack},
	{"MaxLocalVM", 0, max_uint_param,
	   current_MaxLocalVM, set_MaxLocalVM},
	{"VMReclaim", -2, 0,
	   current_VMReclaim, set_vm_reclaim},
	{"VMThreshold", 0, max_uint_param,
	   current_VMThreshold, set_vm_threshold},
	{"WaitTimeout", 0, max_uint_param,
	   current_WaitTimeout, set_WaitTimeout}
};
/* Boolean values */
private bool
current_AccurateScreens(void)
{	return gs_currentaccuratescreens();
}
private int
set_AccurateScreens(bool val)
{	gs_setaccuratescreens(val);
	return 0;
}
private const bool_param_def user_bool_params[] = {
	{"AccurateScreens", current_AccurateScreens, set_AccurateScreens}
};
/* String values */
private void
current_JobName(gs_param_string *pval)
{	pval->data = 0;
	pval->size = 0;
	pval->persistent = true;
}
private int
set_JobName(gs_param_string *val)
{	return 0;
}
private const string_param_def user_string_params[] = {
	{"JobName", current_JobName, set_JobName}
};

/* The user parameter set */
private const param_set user_param_set = {
	user_long_params, countof(user_long_params),
	user_bool_params, countof(user_bool_params),
	user_string_params, countof(user_string_params)
};

/* <dict> setuserparams - */
private int
zsetuserparams(register os_ptr op)
{	dict_param_list list;
	int code = dict_param_list_read(&list, op, NULL, false);
	int i;
	if ( code < 0 )
	  return code;
	for ( i = 0; i < countof(user_long_params); i++ )
	  {	const long_param_def *pdef = &user_long_params[i];
		if ( pdef->set != NULL )
		  {	long val;
			int code = param_read_long((gs_param_list *)&list,
						   pdef->pname, &val);
			switch ( code )
			  {
			  default:			/* invalid */
			    return code;
			  case 1:			/* missing */
			    break;
			  case 0:
			    if ( val < pdef->min_value ||
				 val > pdef->max_value
			       )
			      return_error(e_rangecheck);
			    code = (*pdef->set)(val);
			    if ( code < 0 )
			      return code;
			  }
		  }
	  }
	pop(1);
	return 0;
}

/* - .currentuserparams <name1> <value1> ... */
private int
zcurrentuserparams(os_ptr op)
{	return currentparams(op, &user_param_set);
}

/* ------ The 'where' hack ------ */

private int
z2where(register os_ptr op)
{	/*
	 * Aldus Freehand versions 2.x check for the presence of the
	 * setcolor operator, and if it is missing, substitute a procedure.
	 * Unfortunately, the procedure takes different parameters from
	 * the operator.  As a result, files produced by this application
	 * cause an error if the setcolor operator is actually defined
	 * and 'bind' is ever used.  Aldus fixed this bug in Freehand 3.0,
	 * but there are a lot of files created by the older versions
	 * still floating around.  Therefore, at Adobe's suggestion,
	 * we implement the following dreadful hack in the 'where' operator:
	 *	If the key is /setcolor, and
	 *	  there is a dictionary named FreeHandDict, and
	 *	  currentdict is that dictionary,
	 *	then "where" consults only that dictionary and not any other
	 *	  dictionaries on the dictionary stack.
	 */
	ref rkns, rfh;
	const ref *pdref = dsp;
	ref *pvalue;
	if ( !r_has_type(op, t_name) ||
	     (name_string_ref(op, &rkns), r_size(&rkns)) != 8 ||
	     memcmp(rkns.value.bytes, "setcolor", 8) != 0 ||
	     name_ref((const byte *)"FreeHandDict", 12, &rfh, -1) < 0 ||
	     (pvalue = dict_find_name(&rfh)) == 0 ||
	     !obj_eq(pvalue, pdref)
	   )
		return zwhere(op);
	check_dict_read(*pdref);
	if ( dict_find(pdref, op, &pvalue) > 0 )
	{	ref_assign(op, pdref);
		push(1);
		make_true(op);
	}
	else
		make_false(op);
	return 0;
}

/* ------ Initialization procedure ------ */

/* The level setting ops are recognized even in Level 1 mode. */
BEGIN_OP_DEFS(zmisc2_op_defs) {
	{"0.languagelevel", zlanguagelevel},
	{"1.setlanguagelevel", zsetlanguagelevel},
		/* The rest of the operators are defined only in Level 2. */
		op_def_begin_level2(),
	{"1.checkpassword", zcheckpassword},
	{"0.currentsystemparams", zcurrentsystemparams},
	{"0.currentuserparams", zcurrentuserparams},
	{"1setsystemparams", zsetsystemparams},
	{"1setuserparams", zsetuserparams},
/* Note that this overrides the definition in zdict.c. */
	{"1where", z2where},
END_OP_DEFS(0) }

/* ------ Internal procedures ------ */

/* Get the current values of a parameter set to the stack. */
private int
currentparams(os_ptr op, const param_set _ds *pset)
{	stack_param_list list;
	int i;
	stack_param_list_write(&list, &o_stack, NULL);
	for ( i = 0; i < pset->long_count; i++ )
	{	long val = (*pset->long_defs[i].current)();
		int code = param_write_long((gs_param_list *)&list,
					    pset->long_defs[i].pname, &val);
		if ( code < 0 )
		  return code;
	}
	for ( i = 0; i < pset->bool_count; i++ )
	{	bool val = (*pset->bool_defs[i].current)();
		int code = param_write_bool((gs_param_list *)&list,
					    pset->bool_defs[i].pname, &val);
		if ( code < 0 )
		  return code;
	}
	for ( i = 0; i < pset->string_count; i++ )
	{	gs_param_string val;
		int code;
		(*pset->string_defs[i].current)(&val);
		code = param_write_string((gs_param_list *)&list,
					  pset->string_defs[i].pname, &val);
		if ( code < 0 )
		  return code;
	}
	return 0;
}

/* Adjust the interpreter for a change in language level. */
/* This is used for the .setlanguagelevel operator, */
/* and after a restore. */
private int swap_entry(P3(ref elt[2], ref *pdict, ref *pdict2));
private int
set_language_level(int level)
{	ref *pgdict =		/* globaldict, if present */
	  ref_stack_index(&d_stack, ref_stack_count(&d_stack) - 2);
	ref *level2dict;
	if (dict_find_string(systemdict, "level2dict", &level2dict) <= 0)
          return_error(e_undefined);          
	/* As noted in dstack.h, we allocate the extra d-stack entry */
	/* for globaldict even in Level 1 mode; in Level 1 mode, */
	/* this entry holds an extra copy of systemdict, and */
	/* [count]dictstack omit the very bottommost entry. */
	if ( level == 2 )		/* from Level 1 to Level 2 */
	{	/* Put globaldict in the dictionary stack. */
		ref *pdict;
		int code = dict_find_string(level2dict, "globaldict", &pdict);
		if ( code <= 0 )
			return_error(e_undefined);
		if ( !r_has_type(pdict, t_dictionary) )
			return_error(e_typecheck);
		*pgdict = *pdict;
		/* Set other flags for Level 2 operation. */
		dict_auto_expand = true;
	}
	else				/* from Level 2 to Level 1 */
	{	/* Clear the cached definition pointers of all names */
		/* defined in globaldict.  This will slow down */
		/* future lookups, but we don't care. */
		int index = dict_first(pgdict);
		ref elt[2];
		while ( (index = dict_next(pgdict, index, &elt[0])) >= 0 )
		  if ( r_has_type(&elt[0], t_name) )
		    elt[0].value.pname->pvalue = pv_other;
		/* Overwrite globaldict in the dictionary stack. */
		*pgdict = *systemdict;
		/* Set other flags for Level 1 operation. */
		dict_auto_expand = false;
	}
	/* Swap the contents of level2dict and systemdict. */
	/* If a value in level2dict is a dictionary, and it contains */
	/* a key/value pair referring to itself, swap its contents */
	/* with the contents of the same dictionary in systemdict. */
	/* (This is a hack to swap the contents of statusdict.) */
	{	int index = dict_first(level2dict);
		ref elt[2];		/* key, value */
		ref *subdict;
		while ( (index = dict_next(level2dict, index, &elt[0])) >= 0 )
		  if ( r_has_type(&elt[1], t_dictionary) &&
		       dict_find(&elt[1], &elt[0], &subdict) > 0 &&
		       obj_eq(&elt[1], subdict)
		     )
		{
#define sub2dict &elt[1]
			int isub = dict_first(sub2dict);
			ref subelt[2];
			int found = dict_find(systemdict, &elt[0], &subdict);
			if ( found <= 0 )
			  continue;
			while ( (isub = dict_next(sub2dict, isub, &subelt[0])) >= 0 )
			  if ( !obj_eq(&subelt[0], &elt[0]) )	/* don't swap dict itself */
			{	int code = swap_entry(subelt, subdict, sub2dict);
				if ( code < 0 )
				  return code;
			}
#undef sub2dict
		}
		  else
		{	int code = swap_entry(elt, systemdict, level2dict);
			if ( code < 0 )
			  return code;
		}
	}
	dict_set_top();		/* reload dict stack cache */
	return 0;
}

/* Swap an entry from a Level 2 dictionary into a base dictionary. */
/* elt[0] is the key, elt[1] is the value in the Level 2 dictionary. */
private int
swap_entry(ref elt[2], ref *pdict, ref *pdict2)
{	ref *pvalue;
	ref old_value;
	int found = dict_find(pdict, &elt[0], &pvalue);
	switch ( found )
	{
	default:		/* <0, error */
		return found;
	case 0:			/* missing */
		make_null(&old_value);
		break;
	case 1:			/* present */
		old_value = *pvalue;
	}
	/*
	 * Temporarily flag the dictionaries as local, so that we don't
	 * get invalidaccess errors.  (We know that they are both
	 * referenced from systemdict, so they are allowed to reference
	 * local objects even if they are global.)
	 */
	{	uint space2 = r_space(pdict2);
		int code;
		r_set_space(pdict2, avm_local);
		dict_put(pdict2, &elt[0], &old_value);
		if ( r_has_type(&elt[1], t_null) )
			code = dict_undef(pdict, &elt[0]);
		else
		{	uint space = r_space(pdict);
			r_set_space(pdict, avm_local);
			code = dict_put(pdict, &elt[0], &elt[1]);
			r_set_space(pdict, space);
		}
		r_set_space(pdict2, space2);
		return code;
	}
}
