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

/* iname.h */
/* Name table interface */

/* ---------------- Structure definitions ---------------- */

/*
 * Define the structure of a name.  The next_index "pointer" is for
 * the chained hash table in the name_table.  The pvalue member implements
 * an important optimization to avoid lookup for operator and other
 * global names.
 */
struct name_s {
	ushort next_index;	/* next name in chain or 0 */
	unsigned foreign_string : 1; /* string is allocated statically */
	unsigned mark : 1;	/* GC mark bit */
	unsigned string_size : 14;
#define max_name_string 0x3fff
	const byte *string_bytes;
/* pvalue specifies the definition status of the name: */
/*	pvalue == pv_no_defn: no definitions */
#define pv_no_defn ((ref *)0)
/*	pvalue == pv_other: other status */
#define pv_other ((ref *)1)
#define pv_valid(pvalue) ((unsigned long)(pvalue) > 1)
/*	pvalue != pv_no_defn, pvalue != pv_other: pvalue is valid */
	ref *pvalue;		/* if only defined in systemdict */
				/* or userdict, this points to */
				/* the value */
};
/*typedef struct name_s name;*/		/* in iref.h */

/*
 * Define the structure of a name table.  Normally we would make this
 * an opaque type, but we want to be able to in-line some of the
 * access procedures.
 *
 * The name table is a two-level indexed table, consisting of
 * nt_sub_count sub-tables of size nt_sub_size each.
 *
 * First we define the name sub-table structure.
 */
#define nt_log2_sub_size 7
# define nt_sub_size (1 << nt_log2_sub_size)
# define nt_sub_index_mask (nt_sub_size - 1)
# define nt_sub_count (1 << (16 - nt_log2_sub_size))
#define max_name_count ((uint)0xffff)
typedef name name_sub_table[nt_sub_size];
/*
 * Now define the name table itself.
 */
#define nt_hash_size 1024		/* must be a power of 2 */
typedef struct name_table_s {
	ushort hash[nt_hash_size];
	name *table[nt_sub_count];	/* actually name_sub_table */
	uint count;
#define nt_count(nt) ((nt)->count)
#define set_nt_count(nt,cnt) ((nt)->count = (cnt))
	gs_memory_t *memory;
} name_table;

/* ---------------- Procedural interface ---------------- */

/* Allocate and initialize a name table. */
name_table *name_init(P1(gs_memory_t *));

/*
 * The name table machinery is designed so that multiple name tables
 * are possible, but the interpreter relies on there being only one,
 * and many of the procedures below assume this (by virtue of
 * not taking a name_table argument).  Therefore, we provide a procedure
 * to get our hands on that unique table (read-only, however).
 */
const name_table *the_name_table(P0());

/*
 * Look up and/or enter a name in the name table.
 * The size argument for name_ref should be a ushort, but this confuses
 * the Apollo compiler.  The values of enterflag are:
 *	-1 -- don't enter (return an error) if missing;
 *	 0 -- enter if missing, don't copy the string, which was allocated
 *		statically;
 *	 1 -- enter if missing, copy the string;
 *	 2 -- enter if missing, don't copy the string, which was already
 *		allocated dynamically.
 */
int name_ref(P4(const byte *ptr, uint size, ref *pnref, int enterflag));
void name_string_ref(P2(const ref *pnref, ref *psref));
/* name_enter_string calls name_ref with a (permanent) C string. */
int name_enter_string(P2(const char *str, ref *pnref));
/* name_from_string essentially implements cvn. */
/* It always enters the name, and copies the executable attribute. */
int name_from_string(P2(const ref *psref, ref *pnref));

/* Compare two names for equality. */
#define name_eq(pnref1, pnref2)\
  (name_index(pnref1) == name_index(pnref2))

/* Convert between names and indices.  Note that the inline versions, */
/* but not the procedure versions, take a name_table argument. */
		/* ref => index */
#define name_index(pnref) r_size(pnref)
		/* index => name */
name *name_index_ptr(P1(uint nidx /* should be ushort */));
#define name_index_ptr_inline(nt, nidx)\
  ((nt)->table[(nidx) >> nt_log2_sub_size] + ((nidx) & nt_sub_index_mask))
		/* index => ref */
void name_index_ref(P2(uint nidx /* should be ushort */,
		       ref *pnref));
#define name_index_ref_inline(nt, nidx, pnref)\
  make_name(pnref, nidx, name_index_ptr_inline(nt, nidx));
		/* name => ref */
/* We have to set the space to system so that the garbage collector */
/* won't think names are foreign and therefore untraceable. */
#define make_name(pnref, nidx, pnm)\
  make_tasv(pnref, t_name, avm_system, nidx, pname, pnm)

/* ---------------- Name count/index maintenance ---------------- */

/*
 * We scramble the assignment order within a sub-table, so that
 * dictionary lookup doesn't have to scramble the index.
 * The scrambling algorithm must have three properties:
 *	- It must map 0 to 0;
 *	- It must only scramble the sub-table index;
 *	- It must be a permutation on the sub-table index.
 * We do something very simple for now.
 */
#define name_count_to_index(cnt)\
  (((cnt) & (-nt_sub_size)) +\
   (((cnt) * 59) & nt_sub_index_mask))
#define max_name_index ((uint)0xffff)
/*
 * The reverse permutation requires finding a number R such that
 * 59*R = 1 mod nt_sub_size.  For nt_sub_size any power of 2 up to 2048,
 * R = 243 will work.
 */
#define name_index_to_count(nidx)\
  (((nidx) & (-nt_sub_size)) +\
   (((nidx) * 243) & nt_sub_index_mask))

/* Name count interface for GC and save/restore. */
uint name_count(P0());
#define name_is_since_count(pnref, ncnt)\
  name_index_is_since_count(name_index(pnref), ncnt)
#define name_index_is_since_count(nidx, ncnt)\
  (name_index_to_count(nidx) >= ncnt)
gs_memory_t *name_memory(P0());

/* Mark a name for the garbage collector. */
/* Return true if this is a new mark. */
bool name_mark_index(P1(uint));

/* Get the object (sub-table) containing a name. */
/* The garbage collector needs this so it can relocate pointers to names. */
void/*obj_header_t*/ *name_ref_sub_table(P1(ref *));
void/*obj_header_t*/ *name_index_ptr_sub_table(P2(uint, name *));

/* Clean up the name table before a restore, */
/* by removing names whose count is less than old_count. */
void name_restore(P1(uint old_count));
