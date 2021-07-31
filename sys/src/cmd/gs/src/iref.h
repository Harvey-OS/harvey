/* Copyright (C) 1989, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: iref.h,v 1.5 2000/11/01 21:24:42 lpd Exp $ */
/* Object structure and type definitions for Ghostscript */

#ifndef iref_INCLUDED
#  define iref_INCLUDED

/* The typedef for object references */
#ifndef ref_DEFINED
typedef struct ref_s ref;
#  define ref_DEFINED
#endif

/*
 * Define the type for packed object references.  This is opaque here:
 * the details are in ipacked.h.
 */
typedef ushort ref_packed;

#define log2_sizeof_ref_packed arch_log2_sizeof_short
#define sizeof_ref_packed (1 << log2_sizeof_ref_packed)

/*
 * Define the object types.
 * The types marked with @ are composite and hence use the a_space field;
 * objects of all other types must have a_space cleared.
 * The types marked with ! behave differently in the interpreter
 * depending on whether they are executable or literal.
 * The types marked with + use the read/write/execute
 * attributes; the rest only use the executable attribute.
 * The types marked with # use the size field.
 *
 * Note that for the object types that support getinterval (array and
 * string types), there is no way to tell whether a given reference
 * designates an original object or a sub-interval.  This is a deliberate
 * design decision.
 */
typedef enum {

/*
 * Type 0 must be left unassigned, so that the type (and type_attrs)
 * of a valid ref will never be zero.  This speeds up simultaneous
 * type/space checking in def (see dstack.h for details) and a similar
 * check in ref_save (see store.h for details).  We may as well use
 * type 0 for t__invalid, which will never appear in a real ref.
 *
 * The "invalid" type is only used in a few special places: the guard
 * entries at the bottom of the o-stack that detect stack underflow, and
 * (perhaps eventually) the ref that the cached value pointer in names
 * points to if the binding isn't known.  It never appears on a stack or in
 * a program-visible data structure.
 */

    t__invalid,			/*      (no value) */
    t_boolean,			/*      value.boolval */
    t_dictionary,		/* @ +  value.pdict */
    t_file,			/* @!+# value.pfile, uses size for id */

/*
 * The 4 array types must be kept together, and must start at
 * a multiple of 4, for the sake of r_is_array and r_is_proc (see below).
 */

#define _t_array_span 4
    t_array,			/* @!+# value.refs */
    /*
     * The following are the two implementations of packed arrays.
     * See ipacked.h for details.
     */
    t_mixedarray,		/* @!+# value.packed */
    t_shortarray,		/* @!+# value.packed */
    t_unused_array_,		/*      (an unused array type) */

/*
 * t_[a]struct is an "umbrella" for other types that are represented by
 * allocated objects (structures).  Objects of these types are composite
 * and hence use the a_local attribute.  The type name is taken from
 * the allocator template for the structure.  t_astruct objects use the
 * access attributes; t_struct objects do not.  Neither t_struct nor
 * t_astruct objects use the size.
 *
 * t_struct is currently used for the following PostScript types:
 *      condition, lock.
 * We could use it for fontIDs, except that they may have subclasses.
 * Eventually it will also be used for the new 'device' type.
 * t_astruct is currently used for the following PostScript types:
 *      gstate.
 *
 * The 2 structure types must be kept together, and must start at
 * a multiple of 2, for the sake of r_has_stype (see below).
 */

#define _t_struct_span 2
    t_struct,			/* @    value.pstruct */
    t_astruct,			/* @ +  value.pstruct */

/*
 * We now continue with individual types.
 */
    t_fontID,			/* @    value.pstruct */
    t_integer,			/*      value.intval */
    t_mark,			/*        (no value) */
/*
 * Name objects use the a_space field because they really are composite
 * objects internally.
 */
    t_name,			/* @! # value.pname, uses size for index */
    t_null,			/*  ! # (value.opproc, uses size for mark */
				/*        type, on e-stack only) */
/*
 * Operator objects use the a_space field because they may actually be
 * disguised procedures.  (Real operators always have a_space = 0.)
 */
    t_operator,			/* @! # value.opproc, uses size for index */
    t_real,			/*      value.realval */
    t_save,			/*      value.saveid, see isave.h for why */
				/*        this isn't a t_struct */
    t_string,			/* @!+# value.bytes */
/*
 * The following are extensions to the PostScript type set.
 * If you add new types, be sure to edit:
 *      - REF_TYPE_*STRINGS* and REF_TYPE_PROPERTIES_DATA below;
 *      - the table in gs_init.ps (==only operator);
 *      - the printing routine in idebug.c;
 *      - the dispatches in igc.c, igcref.c, and interp.c;
 *      - obj_cvs, obj_cvp, and obj_eq in iutil.c;
 *      - restore_check_stack in zvmem.c.
 */
    t_device,			/* @ +   value.pdevice */
    t_oparray,			/* @! #  value.const_refs, uses size */
				/*         for index */
    t_next_index		/*** first available index ***/
} ref_type;

/*
 * The interpreter uses types starting at t_next_index for representing
 * a few high-frequency operators.
 * Since there are no operations specifically on operators,
 * there is no need for any operators to check specifically for these
 * types.  The r_btype macro takes care of the conversion when required.
 */
/*extern const int tx_next_index; *//* in interp.c */
/*
 * Define a table giving properties of types, similar to the table used
 * by the isxxx functions (macros) in <ctype.h>.
 */
#define _rtype_uses_access 1	/* type uses w/r/x attributes */
#define _rtype_uses_size 2
#define _rtype_is_null 4
#define _rtype_is_dictionary 8
extern const byte ref_type_properties[1 << 6];	/* r_type_bits */

#define REF_TYPE_PROPERTIES_DATA\
  0,				/* t__invalid */\
  0,				/* t_boolean */\
  _rtype_uses_access | _rtype_is_dictionary, /* t_dictionary */\
  _rtype_uses_access | _rtype_uses_size, /* t_file */\
  _rtype_uses_access | _rtype_uses_size, /* t_array */\
  _rtype_uses_access | _rtype_uses_size, /* t_mixedarray */\
  _rtype_uses_access | _rtype_uses_size, /* t_shortarray */\
  _rtype_uses_access | _rtype_uses_size, /* (unused array type) */\
  0,				/* t_struct */\
  _rtype_uses_access,		/* t_astruct */\
  0,				/* t_fontID */\
  0,				/* t_integer */\
  0,				/* t_mark */\
  _rtype_uses_size,		/* t_name */\
  _rtype_is_null,		/* t_null, uses size only on e-stack */\
  _rtype_uses_size,		/* t_operator */\
  0,				/* t_real */\
  0,				/* t_save */\
  _rtype_uses_access | _rtype_uses_size, /* t_string */\
  _rtype_uses_access,		/* t_device */\
  _rtype_uses_size,		/* t_oparray */\
    /*\
     * The remaining types are the extended pseudo-types used by the\
     * interpreter for operators.  We need to fill up the table.\
     */\
  _rtype_uses_size,_rtype_uses_size,_rtype_uses_size, /*24*/\
  _rtype_uses_size,_rtype_uses_size,_rtype_uses_size,_rtype_uses_size, /*28*/\
  _rtype_uses_size,_rtype_uses_size,_rtype_uses_size,_rtype_uses_size, /*32*/\
  _rtype_uses_size,_rtype_uses_size,_rtype_uses_size,_rtype_uses_size, /*36*/\
  _rtype_uses_size,_rtype_uses_size,_rtype_uses_size,_rtype_uses_size, /*40*/\
  _rtype_uses_size,_rtype_uses_size,_rtype_uses_size,_rtype_uses_size, /*44*/\
  _rtype_uses_size,_rtype_uses_size,_rtype_uses_size,_rtype_uses_size, /*48*/\
  _rtype_uses_size,_rtype_uses_size,_rtype_uses_size,_rtype_uses_size, /*52*/\
  _rtype_uses_size,_rtype_uses_size,_rtype_uses_size,_rtype_uses_size, /*56*/\
  _rtype_uses_size,_rtype_uses_size,_rtype_uses_size,_rtype_uses_size, /*60*/\
  _rtype_uses_size,_rtype_uses_size,_rtype_uses_size,_rtype_uses_size	/*64 */
#define _rtype_has(rtype,props)\
  ((ref_type_properties[rtype] & (props)) != 0)
#define ref_type_uses_access(rtype)\
 _rtype_has(rtype, _rtype_uses_access)
#define ref_type_uses_size(rtype)\
 _rtype_has(rtype, _rtype_uses_size)
#define ref_type_uses_size_or_null(rtype)\
 _rtype_has(rtype, _rtype_uses_size | _rtype_is_null)
/*
 * Define the type names for debugging printout.
 * All names must be the same length, so that columns will line up.
 */
#define REF_TYPE_DEBUG_PRINT_STRINGS\
  "INVL","bool","dict","file",\
  "arry","mpry","spry","u?ry",\
  "STRC","ASTR",\
  "font","int ","mark","name","null",\
  "oper","real","save","str ",\
  "devc","opry"
/*
 * Define the type names for the type operator.
 */
#define REF_TYPE_NAME_STRINGS\
  0,"booleantype","dicttype","filetype",\
  "arraytype","packedarraytype","packedarraytype","arraytype",\
  0,0,\
  "fonttype","integertype","marktype","nametype","nulltype",\
  "operatortype","realtype","savetype","stringtype",\
  "devicetype","operatortype"
/*
 * Define the type names for obj_cvp (the == operator).  We only need these
 * for types that obj_cvp and obj_cvs don't handle specially.
 */
#define REF_TYPE_PRINT_STRINGS\
  0,0,"-dict-","-file-",\
  "-array-","-packedarray-","-packedarray-","-array-",\
  0,0,\
  "-fontID-",0,"-mark-",0,0,\
  0,0,"-save-","-string-",\
  "-device-",0

/*
 * The following factors affect the encoding of attributes:
 *
 *      - The packed array format requires the high-order bits of the
 *        type/attributes field to be 0.  (see packed.h)
 *
 *      - The interpreter wants the type, executable bit, and execute
 *        permission to be adjacent, and in that order from high to low.
 *
 *      - Type testing is most efficient if the type is in a byte by itself.
 *
 * The layout given below results in the most efficient code overall.
 */

/*
 * A few of the fields of a ref are associated with the *location*;
 * most are associated with the ref that is *stored* in that location.
 * When a ref is copied from one location to another, the former are
 * preserved in the destination, the latter are copied.
 */
/*
 * The following are the attributes associated with the location:
 */
#define l_mark 1		/* mark for garbage collector */
#define l_new 2			/* stored into since last save */
/*
 * The following are attributes associated with the ref itself (the
 * contents of the location).  These are visible to PostScript code.
 */
/*
 * Reserve bits for VM space information (defined in ivmspace.h).  Note that
 * These bits refer to the VM space of the pointer in the ref, if any, not
 * the location in which the ref itself is stored.  For scalars, these bits
 * are always zero.
 */
#define r_space_bits 2
#define r_space_shift 2
/*
 * Define the protection attributes.  Only 4 combinations are legal:
 * 0, execute, execute + read, execute + read + write.  Note that some
 * refs (such as scalars) do not use these: in such refs, they are
 * always zero.
 */
#define a_write 0x10
#define a_read 0x20
#define a_execute 0x40
#define a_readonly (a_read + a_execute)
#define a_all (a_write + a_read+a_execute)
/*
 * Define the executable attribute.  All refs use this.
 */
#define a_executable 0x80
/*
 * Define the bits used for the ref type.  See ipacked.h for more
 * information about the possible values of the type byte.
 */
#define r_type_shift 8
#define r_type_bits 6

/* Define the attribute names for debugging printout. */
/* Each entry has the form <mask, value, character>. */
typedef struct attr_print_mask_s {
    ushort mask;
    ushort value;
    char print;
} attr_print_mask;

#define attr_print_flag(m,c)\
  {m,m,c},{m,0,'-'}
#define attr_print_space(v,c)\
  {((1<<r_space_bits)-1)<<r_space_shift,v,c}
#define attr_print_masks\
  attr_print_flag(l_mark,'m'),\
  attr_print_flag(l_new,'n'),\
  attr_print_space(avm_foreign,'F'),\
  attr_print_space(avm_system,'S'),\
  attr_print_space(avm_global,'G'),\
  attr_print_space(avm_local,'L'),\
  attr_print_flag(a_write,'w'),\
  attr_print_flag(a_read,'r'),\
  attr_print_flag(a_execute,'x'),\
  attr_print_flag(a_executable,'e'),\
  attr_print_flag(0x4000,'?'),\
  attr_print_flag(0x8000,'?')

/* Abstract types */
typedef struct dict_s dict;
typedef struct name_s name;

#ifndef stream_DEFINED
#  define stream_DEFINED
typedef struct stream_s stream;
#endif
#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif
#ifndef obj_header_DEFINED
#  define obj_header_DEFINED
typedef struct obj_header_s obj_header_t;
#endif

/*
 * Define the argument type for operator procedures.  Note that the
 * argument name is not arbitrary: it is used in access macros, so all
 * operator procedures must use it.
 */
#ifndef i_ctx_t_DEFINED
#  define i_ctx_t_DEFINED
typedef struct gs_context_state_s i_ctx_t;
#endif
typedef int (*op_proc_t)(P1(i_ctx_t *i_ctx_p));
/* real_opproc is a holdover.... */
#define real_opproc(pref) ((pref)->value.opproc)

/* Object reference */
/*
 * Note that because of the way packed arrays are represented,
 * the type_attrs member must be the first one in the ref structure.
 */
struct tas_s {
    ushort type_attrs;
    ushort rsize;
};
struct ref_s {

    struct tas_s tas;

#define r_size(rp) ((rp)->tas.rsize)
#define r_inc_size(rp,inc) ((rp)->tas.rsize += (inc))
#define r_dec_size(rp,dec) ((rp)->tas.rsize -= (dec))
#define r_set_size(rp,siz) ((rp)->tas.rsize = (siz))
/* type_attrs is a single element for fast dispatching in the interpreter */
#if r_type_shift == 8
#  if arch_is_big_endian
#    define r_type(rp) (((const byte *)&((rp)->tas.type_attrs))[sizeof(ushort)-2])
#  else
#    define r_type(rp) (((const byte *)&((rp)->tas.type_attrs))[1])
#  endif
#  define r_has_type(rp,typ) (r_type(rp) == (typ))
#else
#  define r_type(rp) ((rp)->tas.type_attrs >> r_type_shift)
#  define r_has_type(rp,typ) r_has_type_attrs(rp,typ,0)		/* see below */
#endif
/* A special macro for testing arrayhood. */
#define r_is_array(rp) _r_has_masked_type_attrs(rp,t_array,_t_array_span,0)
#define r_set_type(rp,typ) ((rp)->tas.type_attrs = (typ) << r_type_shift)
#define r_btype(rp)\
 ((rp)->tas.type_attrs >= (t_next_index << r_type_shift) ?\
  t_operator : r_type(rp))
#define r_type_xe_shift (r_type_shift - 2)
#define type_xe_(tas) ((tas) >> r_type_xe_shift)	/* internal use only */
/*
 * The r_type_xe macro is used in (and only in) the main interpreter loop,
 * where its rp operand may be a ref_packed, not necessarily aligned as
 * strictly as a full-size ref.  The DEC C compiler, and possibly others,
 * may compile code assuming that rp is ref-aligned.  Therefore, we
 * explicitly cast the pointer to a less-strictly-aligned type.
 * In order to convince the compiler, we have to do the cast before
 * indexing into the structure.
 */
#define r_type_xe(rp)\
  type_xe_(((const ushort *)(rp))[offset_of(ref, tas.type_attrs) / sizeof(ushort)])
#define type_xe_value(t,xe) type_xe_(((t) << r_type_shift) + (xe))
#define r_type_attrs(rp) ((rp)->tas.type_attrs)		/* reading only */
#define r_has_attrs(rp,mask) !(~r_type_attrs(rp) & (mask))
#define r_has_masked_attrs(rp,attrs,mask)\
  ((r_type_attrs(rp) & (mask)) == (attrs))
#define r_has_attr(rp,mask1)		/* optimize 1-bit case */\
   (r_type_attrs(rp) & (mask1))
/* The following macro is not for external use. */
#define _r_has_masked_type_attrs(rp,typ,tspan,mask)\
 (((rp)->tas.type_attrs &\
   ((((1 << r_type_bits) - (tspan)) << r_type_shift) + (mask))) ==\
  (((typ) << r_type_shift) + (mask)))
#define r_has_type_attrs(rp,typ,mask)\
  _r_has_masked_type_attrs(rp,typ,1,mask)
/* A special macro for testing procedurehood. */
#define r_is_proc(rp)\
  _r_has_masked_type_attrs(rp,t_array,_t_array_span,a_execute+a_executable)
#define r_set_attrs(rp,mask) ((rp)->tas.type_attrs |= (mask))
#define r_clear_attrs(rp,mask) ((rp)->tas.type_attrs &= ~(mask))
#define r_store_attrs(rp,mask,attrs)\
  ((rp)->tas.type_attrs = ((rp)->tas.type_attrs & ~(mask)) | (attrs))
#define r_copy_attrs(rp,mask,sp)\
  r_store_attrs(rp,mask,(sp)->tas.type_attrs & (mask))
#define r_set_type_attrs(rp,typ,mask)\
  ((rp)->tas.type_attrs = ((typ) << r_type_shift) + (mask))
/* Macros for t_[a]struct objects. */
#define r_is_struct(rp) _r_has_masked_type_attrs(rp,t_struct,_t_struct_span,0)
#define r_has_stype(rp,mem,styp)\
  (r_is_struct(rp) && gs_object_type(mem, (rp)->value.pstruct) == &styp)
#define r_ptr(rp,typ) ((typ *)((rp)->value.pstruct))
#define r_set_ptr(rp,ptr) ((rp)->value.pstruct = (obj_header_t *)(ptr))

    union v {			/* name the union to keep gdb happy */
	long intval;
	ushort boolval;
	float realval;
	ulong saveid;
	byte *bytes;
	const byte *const_bytes;
	ref *refs;
	const ref *const_refs;
	name *pname;
	const name *const_pname;
	dict *pdict;
	const dict *const_pdict;
	/*
	 * packed is the normal variant for referring to packed arrays,
	 * but we need a writable variant for memory management and for
	 * storing into packed dictionary key arrays.
	 */
	const ref_packed *packed;
	ref_packed *writable_packed;
	op_proc_t opproc;
	struct stream_s *pfile;
	struct gx_device_s *pdevice;
	obj_header_t *pstruct;
    } value;
};

/* Define data for initializing an empty array or string. */
#define empty_ref_data(type, attrs)\
  { /*tas*/ { /*type_attrs*/ ((type) << r_type_shift) | (attrs),\
	      /*rsize*/ 0 } }

/* Define the size of a ref. */
#define arch_sizeof_ref sizeof(ref)
/* Define the required alignment for refs. */
/* We assume all alignment values are powers of 2. */
#define arch_align_ref_mod\
 (((arch_align_long_mod - 1) | (arch_align_float_mod - 1) |\
   (arch_align_ptr_mod - 1)) + 1)

/* Define the maximum size of an array or a string. */
/* The maximum array size is determined by the fact that */
/* the allocator cannot allocate a block larger than max_uint. */
#define max_array_size (max_ushort & (max_uint / (uint)arch_sizeof_ref))
#define max_string_size max_ushort

#endif /* iref_INCLUDED */
