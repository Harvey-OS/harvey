/* Copyright (C) 1991, 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: opdef.h,v 1.2 2000/09/19 19:00:47 lpd Exp $ */
/* Operator definition interface for Ghostscript */

#ifndef opdef_INCLUDED
#  define opdef_INCLUDED

/*
 * Define the structure for initializing the operator table.  Each operator
 * file zxxx.c declares an array of these as follows:

 const op_def * const zxxx_op_defs[] = {
    {"1name", zname},
    ...
    op_def_end(iproc)
 };

 * where iproc is an initialization procedure for the file, or 0.  This
 * definition always appears at the END of the file, to avoid the need for
 * forward declarations for all the operator procedures.  For backward
 * compatibility with an older convention, we also allow (deprecated)

 BEGIN_OP_DEFS(my_defs) {
    {"1name", zname},
    ...
 END_OP_DEFS(iproc) }

 * Operators may be stored in dictionaries other than systemdict.
 * We support this with op_def entries of a special form:

 op_def_begin_dict("dictname"),

 */
typedef struct {
    const char *oname;
    op_proc_t proc;
} op_def;

#define op_def_begin_dict(dname) {dname, 0}
#define op_def_begin_filter() op_def_begin_dict("filterdict")
#define op_def_begin_level2() op_def_begin_dict("level2dict")
#define op_def_begin_ll3() op_def_begin_dict("ll3dict")
#define op_def_is_begin_dict(def) ((def)->proc == 0)
#define op_def_end(iproc) {0, iproc}

/*
 * NOTE: for implementation reasons, a single table of operator definitions
 * is limited to 16 entries, including op_def_begin_xxx entries.  If a file
 * defines more operators than this, it must split them into multiple
 * tables and have multiple -oper entries in the makefile.  Currently,
 * only 4 out of 85 operator files require this.
 */
#define OP_DEFS_LOG2_MAX_SIZE 4
#define OP_DEFS_MAX_SIZE (1 << OP_DEFS_LOG2_MAX_SIZE)

/*
 * Define the table of pointers to all operator definition tables.
 */
extern const op_def *const op_defs_all[];

/*
 * Formerly, we needed to define each op_defs table as a procedure that
 * returns the actual table, because of cross-segment linking restrictions
 * in the 16-bit Borland C compiler for MS Windows.  This is no longer
 * relevant, but for backward compatibility, we need to retain
 * BEGIN/END_OP_DEFS with the same syntax.  This involves a kludge to create
 * a dummy procedure to match up with a closing brace, and another kludge to
 * prevent a "defined but not used" warning.
 */

#define BEGIN_OP_DEFS(xx_op_defs)\
const op_def xx_op_defs[] =

#define END_OP_DEFS(iproc)\
    op_def_end(iproc)\
};\
static int op_defs_dummy(void)\
{	return 1 || op_defs_dummy();	/* generates the least wasted code */

/*
 * Internal operators whose names begin with %, such as continuation
 * operators, do not appear in systemdict.  Ghostscript assumes
 * that these operators cannot appear anywhere (in executable form)
 * except on the e-stack; to maintain this invariant, the execstack
 * operator converts them to literal form, and cvx refuses to convert
 * them back.  As a result of this invariant, they do not need to
 * push themselves back on the e-stack when executed, since the only
 * place they could have come from was the e-stack.
 */
#define op_def_is_internal(def) ((def)->oname[1] == '%')

/*
 * All operators are catalogued in a table; this is necessary because
 * they must have a short packed representation for the sake of 'bind'.
 * The `size' of an operator is normally its index in this table;
 * however, internal operators have a `size' of 0, and their true index
 * must be found by searching the table for their procedure address.
 */
ushort op_find_index(P1(const ref *));

#define op_index(opref)\
  (r_size(opref) == 0 ? op_find_index(opref) : r_size(opref))

/*
 * There are actually two kinds of operators: the real ones (t_operator),
 * and ones defined by procedures (t_oparray).  The catalog for t_operators
 * is (indirectly) op_defs_all, and their index is in the range
 * [1..op_def_count-1].
 */
#define op_index_is_operator(index) ((index) < op_def_count)
extern const uint op_def_count;

#define op_index_def(index)\
  (&op_defs_all[(index) >> OP_DEFS_LOG2_MAX_SIZE]\
    [(index) & (OP_DEFS_MAX_SIZE - 1)])
#define op_num_args(opref) (op_index_def(op_index(opref))->oname[0] - '0')
#define op_index_proc(index) (op_index_def(index)->proc)

/*
 * There are two catalogs for t_oparrays, one global and one local.
 * Operator indices for the global table are in the range
 *      [op_def_count..op_def_count+op_array_global.count-1]
 * Operator indices for the local table are in the range
 *      [op_def_count+r_size(&op_array_global.table)..
 *        op_def_count+r_size(&op_array_global.table)+op_array_local.count-1]
 */
typedef struct op_array_table_s {
    ref table;			/* t_array */
    ushort *nx_table;		/* name indices */
    uint count;			/* # of occupied entries */
    uint base_index;		/* operator index of first entry */
    uint attrs;			/* ref attrs of ops in this table */
    ref *root_p;		/* self-pointer for GC root */
} op_array_table;
extern op_array_table
       op_array_table_global, op_array_table_local;

#define op_index_op_array_table(index)\
  ((index) < op_array_table_local.base_index ?\
   &op_array_table_global : &op_array_table_local)

/*
 * Convert an operator index to an operator or oparray ref.
 * This is only used for debugging and for 'get' from packed arrays,
 * so it doesn't have to be very fast.
 */
void op_index_ref(P2(uint, ref *));

#endif /* opdef_INCLUDED */
