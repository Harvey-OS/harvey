/* Copyright (C) 1991, 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* opdef.h */
/* Operator definition interface for Ghostscript */

/*
 * Operator procedures take the pointer to the top of the o-stack
 * as their argument.  They return 0 for success, a negative code
 * for an error, or a positive code for some uncommon situations (see below).
 */

/* Structure for initializing the operator table. */
/*
 * Each operator file declares an array of these, of the following kind:

BEGIN_OP_DEFS(my_defs) {
	{"1name", zname},
	    ...
END_OP_DEFS(iproc) }

 * where iproc is an initialization procedure for the file, or 0.
 * This definition always appears at the END of the file,
 * to avoid the need for forward declarations for all the
 * operator procedures.
 *
 * Operators may be stored in dictionaries other than systemdict.
 * We support this with op_def entries of a special form:

	op_def_begin_dict("dictname"),

 */
typedef struct {
	const char _ds *oname;
	op_proc_p proc;
} op_def;
typedef const op_def *op_def_ptr;
#define op_def_begin_dict(dname) {dname, 0}
#define op_def_begin_filter() op_def_begin_dict("filterdict")
#define op_def_begin_level2() op_def_begin_dict("level2dict")
#define op_def_is_begin_dict(def) ((def)->proc == 0)
#define op_def_end(iproc) {(char _ds *)0, (op_proc_p)iproc}

/*
 * We need to define each op_defs table as a procedure that returns
 * the actual table, because of cross-segment linking restrictions
 * in the Borland C compiler for MS Windows.
 */

#define BEGIN_OP_DEFS(xx_op_defs)\
const op_def *xx_op_defs(P0())\
{	static const far_data op_def op_defs_[] =

#define END_OP_DEFS(iproc)\
		op_def_end(iproc)\
	};\
	return op_defs_;

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
 * All operators are catalogued in a table, primarily so
 * that they can have a convenient packed representation.
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
 * is op_def_table, and their index is in the range [1..op_def_count).
 */
#define op_index_is_operator(index) ((index) < op_def_count)
/*
 * Because of a bug in Sun's SC1.0 compiler,
 * we have to spell out the typedef for op_def_ptr here:
 */
extern const op_def **op_def_table;
extern uint op_def_count;
#define op_num_args(opref) (op_def_table[op_index(opref)]->oname[0] - '0')
#define op_index_proc(index) (op_def_table[index]->proc)
/*
 * The catalog for t_oparrays is op_array_table, and their index is in
 * the range [op_def_count..op_def_count+op_array_count).  The actual
 * index in op_array_table is the operator index minus op_def_count.
 */
extern ref op_array_table;		/* t_array */
extern ushort *op_array_nx_table;
extern uint op_array_count;
/*
 * The procedure defining a t_oparray operator may be in any VM space.
 * Note that it is OK to store a local t_oparray operator into a global object
 * if the operator was defined when the save level was zero, because
 * restore cannot free local objects defined in this situation.
 * We support this with a .makeglobaloperator operator that defines
 * a local procedure as a global operator (if permitted).  We keep track of
 * this in a separate op_array_attrs_table.
 */
extern byte *op_array_attrs_table;
#define op_index_oparray_ref(index,pref)\
   (r_set_type_attrs(pref, t_oparray, op_array_attrs_table[(index) - op_def_count]),\
    r_set_size(pref, index))
#define op_index_ref(index,pref)\
  (op_index_is_operator(index) ?\
   make_oper(pref, index, op_index_proc(index)) :\
   op_index_oparray_ref(index, pref))
