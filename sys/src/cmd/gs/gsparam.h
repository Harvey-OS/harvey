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

/* gsparam.h */
/* Client interface to parameter dictionaries */

/*
 * Several interfaces use parameter dictionaries to communicate sets of
 * (key, value) pairs from a client to an object with complex state.
 * (Several of these correspond directly to similar interfaces in the
 * PostScript language.) This file defines the API for parameter dictionaries.
 */

#ifndef gs_param_list_DEFINED
#  define gs_param_list_DEFINED
typedef struct gs_param_list_s gs_param_list;
#endif
typedef const char _ds *gs_param_name;

/*
 * Define a structure for representing a variable-size value
 * (string/name, integer array, or floating point array).
 * The size is the number of elements, not the size in bytes.
 * A value is persistent if it is defined as static const,
 * or if it is allocated in garbage-collectable space and never freed.
 */

#define _param_array_struct(sname,etype)\
  struct sname { const etype *data; uint size; bool persistent; }
typedef _param_array_struct(gs_param_string_s, byte) gs_param_string;
typedef _param_array_struct(gs_param_int_array_s, int) gs_param_int_array;
typedef _param_array_struct(gs_param_float_array_s, float) gs_param_float_array;
typedef _param_array_struct(gs_param_string_array_s, gs_param_string) gs_param_string_array;

#define param_string_from_string(ps, str)\
  (ps).data = (const byte *)(str), (ps).size = strlen((char *)(ps).data),\
  (ps).persistent = true

/* Define the 'policies' for handling out-of-range parameter values. */
/* This is not an enum, because some parameters may recognize other values. */
#define gs_param_policy_signal_error 0
#define gs_param_policy_ignore 1
#define gs_param_policy_consult_user 2

/*
 * Define the object procedures.  Note that the same interface is used
 * both for getting and for setting parameter values.  (This is a bit
 * of a hack, and we might change it someday.)  The procedures return
 * 1 for a missing parameter, 0 for a valid parameter, <0 on error.
 * Note that 'reading' a parameter corresponds to 'put' operations from
 * the client's point of view, and 'writing' corresponds to 'get'.
 */

/*
 * Setting parameters must use a "two-phase commit" policy.  Specifically,
 * any put_params procedure must observe the following discipline:
 *	1. Do all validity checks on the parameters.
 *	2. Call the put_params procedure of the superclass, if relevant,
 *	   or the commit procedure in the parameter list, if not.
 *	3. Make all changes in the state of the object whose parameters
 *	   are being set.
 * Step 1 is allowed to make changes in the object state, as long as
 * those changes are undone if step 2 returns an error indication.
 */

typedef struct gs_param_list_procs_s {

	/* Transmit a null value. */
	/* Note that this is the only 'transmit' operation */
	/* that does not actually pass any data. */

#define param_proc_xmit_null(proc)\
  int proc(P2(gs_param_list *, gs_param_name))
	param_proc_xmit_null((*xmit_null));
#define param_read_null(plist, pkey)\
  (*(plist)->procs->xmit_null)(plist, pkey)
#define param_write_null(plist, pkey)\
  (*(plist)->procs->xmit_null)(plist, pkey)

	/* Transmit a Boolean value. */

#define param_proc_xmit_bool(proc)\
  int proc(P3(gs_param_list *, gs_param_name, bool *))
	param_proc_xmit_bool((*xmit_bool));
#define param_read_bool(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_bool)(plist, pkey, pvalue)
#define param_write_bool(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_bool)(plist, pkey, pvalue)

	/* Transmit an integer value. */

#define param_proc_xmit_int(proc)\
  int proc(P3(gs_param_list *, gs_param_name, int *))
	param_proc_xmit_int((*xmit_int));
#define param_read_int(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_int)(plist, pkey, pvalue)
#define param_write_int(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_int)(plist, pkey, pvalue)

	/* Transmit a long value. */

#define param_proc_xmit_long(proc)\
  int proc(P3(gs_param_list *, gs_param_name, long *))
	param_proc_xmit_long((*xmit_long));
#define param_read_long(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_long)(plist, pkey, pvalue)
#define param_write_long(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_long)(plist, pkey, pvalue)

	/* Transmit a float value. */

#define param_proc_xmit_float(proc)\
  int proc(P3(gs_param_list *, gs_param_name, float *))
	param_proc_xmit_float((*xmit_float));
#define param_read_float(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_float)(plist, pkey, pvalue)
#define param_write_float(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_float)(plist, pkey, pvalue)

	/* Transmit a string value. */

#define param_proc_xmit_string(proc)\
  int proc(P3(gs_param_list *, gs_param_name, gs_param_string *))
	param_proc_xmit_string((*xmit_string));
#define param_read_string(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_string)(plist, pkey, pvalue)
#define param_write_string(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_string)(plist, pkey, pvalue)

	/* Transmit a name value. */

#define param_proc_xmit_name(proc)\
  int proc(P3(gs_param_list *, gs_param_name, gs_param_string *))
	param_proc_xmit_name((*xmit_name));
#define param_read_name(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_name)(plist, pkey, pvalue)
#define param_write_name(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_name)(plist, pkey, pvalue)

	/* Transmit an integer array value. */

#define param_proc_xmit_int_array(proc)\
  int proc(P3(gs_param_list *, gs_param_name, gs_param_int_array *))
	param_proc_xmit_int_array((*xmit_int_array));
#define param_read_int_array(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_int_array)(plist, pkey, pvalue)
#define param_write_int_array(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_int_array)(plist, pkey, pvalue)

	/* Transmit a float array value. */

#define param_proc_xmit_float_array(proc)\
  int proc(P3(gs_param_list *, gs_param_name, gs_param_float_array *))
	param_proc_xmit_float_array((*xmit_float_array));
#define param_read_float_array(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_float_array)(plist, pkey, pvalue)
#define param_write_float_array(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_float_array)(plist, pkey, pvalue)

	/* Transmit a string array value. */

#define param_proc_xmit_string_array(proc)\
  int proc(P3(gs_param_list *, gs_param_name, gs_param_string_array *))
	param_proc_xmit_string_array((*xmit_string_array));
#define param_read_string_array(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_string_array)(plist, pkey, pvalue)
#define param_write_string_array(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_string_array)(plist, pkey, pvalue)

	/* Transmit a name array value. */

#define param_proc_xmit_name_array(proc)\
  int proc(P3(gs_param_list *, gs_param_name, gs_param_string_array *))
	param_proc_xmit_name_array((*xmit_name_array));
#define param_read_name_array(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_name_array)(plist, pkey, pvalue)
#define param_write_name_array(plist, pkey, pvalue)\
  (*(plist)->procs->xmit_name_array)(plist, pkey, pvalue)

	/* Get the 'policy' associated with an out-of-range parameter value. */
	/* (Only used when reading.) */

#define param_proc_get_policy(proc)\
  int proc(P2(gs_param_list *, gs_param_name))
	param_proc_get_policy((*get_policy));
#define param_get_policy(plist, pkey)\
  (*(plist)->procs->get_policy)(plist, pkey)

	/*
	 * Signal an error.  (Only used when reading.)
	 * The procedure may return a different error code,
	 * or may return 0 indicating that the error is to be ignored.
	 */

#define param_proc_signal_error(proc)\
  int proc(P3(gs_param_list *, gs_param_name, int))
	param_proc_signal_error((*signal_error));
#define param_signal_error(plist, pkey, code)\
  (*(plist)->procs->signal_error)(plist, pkey, code)
#define param_return_error(plist, pkey, code)\
  return_error(param_signal_error(plist, pkey, code))

	/*
	 * "Commit" a set of changes.  (Only used when reading.)
	 * This is called at the end of the first phase.
	 */

#define param_proc_commit(proc)\
  int proc(P1(gs_param_list *))
	param_proc_commit((*commit));
#define param_commit(plist)\
  (*(plist)->procs->commit)(plist)

} gs_param_list_procs;

/* Define an abstract parameter dictionary.  Implementations are */
/* concrete subclasses. */
#define gs_param_list_common\
	const gs_param_list_procs _ds *procs
struct gs_param_list_s {
	gs_param_list_common;
};
