/* Copyright (C) 1989, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: iname.c,v 1.7 2003/09/03 03:22:59 giles Exp $ */
/* Name lookup for Ghostscript interpreter */
#include "memory_.h"
#include "string_.h"
#include "ghost.h"
#include "gsstruct.h"
#include "gxobj.h"		/* for o_set_unmarked */
#include "ierrors.h"
#include "inamedef.h"
#include "imemory.h"		/* for isave.h */
#include "isave.h"
#include "store.h"

/* Public values */
const uint name_max_string = max_name_string;

/* Define the permutation table for name hashing. */
private const byte hash_permutation[256] = {
    NAME_HASH_PERMUTATION_DATA
};

/* Define the data for the 1-character names. */
private const byte nt_1char_names[NT_1CHAR_SIZE] = {
    NT_1CHAR_NAMES_DATA
};

/* Structure descriptors */
gs_private_st_simple(st_name_sub_table, name_sub_table, "name_sub_table");
gs_private_st_composite(st_name_string_sub_table, name_string_sub_table_t,
			"name_string_sub_table_t",
			name_string_sub_enum_ptrs, name_string_sub_reloc_ptrs);
gs_private_st_composite(st_name_table, name_table, "name_table",
			name_table_enum_ptrs, name_table_reloc_ptrs);

/* Forward references */
private int name_alloc_sub(name_table *);
private void name_free_sub(name_table *, uint);
private void name_scan_sub(name_table *, uint, bool);

/* Debugging printout */
#ifdef DEBUG
private void
name_print(const char *msg, const name_table *nt, uint nidx, const int *pflag)
{
    const name_string_t *pnstr = names_index_string_inline(nt, nidx);
    const name *pname = names_index_ptr_inline(nt, nidx);
    const byte *str = pnstr->string_bytes;

    dlprintf1("[n]%s", msg);
    if (pflag)
	dprintf1("(%d)", *pflag);
    dprintf2(" (0x%lx#%u)", (ulong)pname, nidx);
    debug_print_string(str, pnstr->string_size);
    dprintf2("(0x%lx,%u)\n", (ulong)str, pnstr->string_size);
}
#  define if_debug_name(msg, nt, nidx, pflag)\
     if ( gs_debug_c('n') ) name_print(msg, nt, nidx, pflag)
#else
#  define if_debug_name(msg, nt, nidx, pflag) DO_NOTHING
#endif

/* Initialize a name table */
name_table *
names_init(ulong count, gs_ref_memory_t *imem)
{
    gs_memory_t *mem = (gs_memory_t *)imem;
    name_table *nt;
    int i;

    if (count == 0)
	count = max_name_count + 1L;
    else if (count - 1 > max_name_count)
	return 0;
    nt = gs_alloc_struct(mem, name_table, &st_name_table, "name_init(nt)");
    if (nt == 0)
	return 0;
    memset(nt, 0, sizeof(name_table));
    nt->max_sub_count =
	((count - 1) | nt_sub_index_mask) >> nt_log2_sub_size;
    nt->name_string_attrs = imemory_space(imem) | a_readonly;
    nt->memory = mem;
    /* Initialize the one-character names. */
    /* Start by creating the necessary sub-tables. */
    for (i = 0; i < NT_1CHAR_FIRST + NT_1CHAR_SIZE; i += nt_sub_size) {
	int code = name_alloc_sub(nt);

	if (code < 0) {
	    while (nt->sub_next > 0)
		name_free_sub(nt, --(nt->sub_next));
	    gs_free_object(mem, nt, "name_init(nt)");
	    return 0;
	}
    }
    for (i = -1; i < NT_1CHAR_SIZE; i++) {
	uint ncnt = NT_1CHAR_FIRST + i;
	uint nidx = name_count_to_index(ncnt);
	name *pname = names_index_ptr_inline(nt, nidx);
	name_string_t *pnstr = names_index_string_inline(nt, nidx);

	if (i < 0)
	    pnstr->string_bytes = nt_1char_names,
		pnstr->string_size = 0;
	else
	    pnstr->string_bytes = nt_1char_names + i,
		pnstr->string_size = 1;
	pnstr->foreign_string = 1;
	pnstr->mark = 1;
	pname->pvalue = pv_no_defn;
    }
    nt->perm_count = NT_1CHAR_FIRST + NT_1CHAR_SIZE;
    /* Reconstruct the free list. */
    nt->free = 0;
    names_trace_finish(nt, NULL);
    return nt;
}

/* Get the allocator for the name table. */
gs_memory_t *
names_memory(const name_table * nt)
{
    return nt->memory;
}

/* Look up or enter a name in the table. */
/* Return 0 or an error code. */
/* The return may overlap the characters of the string! */
/* See iname.h for the meaning of enterflag. */
int
names_ref(name_table *nt, const byte *ptr, uint size, ref *pref, int enterflag)
{
    name *pname;
    name_string_t *pnstr;
    uint nidx;
    uint *phash;

    /* Compute a hash for the string. */
    /* Make a special check for 1-character names. */
    switch (size) {
    case 0:
	nidx = name_count_to_index(1);
	pname = names_index_ptr_inline(nt, nidx);
	goto mkn;
    case 1:
	if (*ptr < NT_1CHAR_SIZE) {
	    uint hash = *ptr + NT_1CHAR_FIRST;

	    nidx = name_count_to_index(hash);
	    pname = names_index_ptr_inline(nt, nidx);
	    goto mkn;
	}
	/* falls through */
    default: {
	uint hash;

	NAME_HASH(hash, hash_permutation, ptr, size);
	phash = nt->hash + (hash & (NT_HASH_SIZE - 1));
    }
    }

    for (nidx = *phash; nidx != 0;
	 nidx = name_next_index(nidx, pnstr)
	) {
	pnstr = names_index_string_inline(nt, nidx);
	if (pnstr->string_size == size &&
	    !memcmp_inline(ptr, pnstr->string_bytes, size)
	    ) {
	    pname = name_index_ptr_inline(nt, nidx);
	    goto mkn;
	}
    }
    /* Name was not in the table.  Make a new entry. */
    if (enterflag < 0)
	return_error(e_undefined);
    if (size > max_name_string)
	return_error(e_limitcheck);
    nidx = nt->free;
    if (nidx == 0) {
	int code = name_alloc_sub(nt);

	if (code < 0)
	    return code;
	nidx = nt->free;
    }
    pnstr = names_index_string_inline(nt, nidx);
    if (enterflag == 1) {
	byte *cptr = (byte *)gs_alloc_string(nt->memory, size,
					     "names_ref(string)");

	if (cptr == 0)
	    return_error(e_VMerror);
	memcpy(cptr, ptr, size);
	pnstr->string_bytes = cptr;
	pnstr->foreign_string = 0;
    } else {
	pnstr->string_bytes = ptr;
	pnstr->foreign_string = (enterflag == 0 ? 1 : 0);
    }
    pnstr->string_size = size;
    pname = name_index_ptr_inline(nt, nidx);
    pname->pvalue = pv_no_defn;
    nt->free = name_next_index(nidx, pnstr);
    set_name_next_index(nidx, pnstr, *phash);
    *phash = nidx;
    if_debug_name("new name", nt, nidx, &enterflag);
 mkn:
    make_name(pref, nidx, pname);
    return 0;
}

/* Get the string for a name. */
void
names_string_ref(const name_table * nt, const ref * pnref /* t_name */ ,
		 ref * psref /* result, t_string */ )
{
    const name_string_t *pnstr = names_string_inline(nt, pnref);

    make_const_string(psref,
		      (pnstr->foreign_string ? avm_foreign | a_readonly :
		       nt->name_string_attrs),
		      pnstr->string_size,
		      (const byte *)pnstr->string_bytes);
}

/* Convert a t_string object to a name. */
/* Copy the executable attribute. */
int
names_from_string(name_table * nt, const ref * psref, ref * pnref)
{
    int exec = r_has_attr(psref, a_executable);
    int code = names_ref(nt, psref->value.bytes, r_size(psref), pnref, 1);

    if (code < 0)
	return code;
    if (exec)
	r_set_attrs(pnref, a_executable);
    return code;
}

/* Enter a (permanently allocated) C string as a name. */
int
names_enter_string(name_table * nt, const char *str, ref * pref)
{
    return names_ref(nt, (const byte *)str, strlen(str), pref, 0);
}

/* Invalidate the value cache for a name. */
void
names_invalidate_value_cache(name_table * nt, const ref * pnref)
{
    pnref->value.pname->pvalue = pv_other;
}

/* Convert between names and indices. */
#undef names_index
name_index_t
names_index(const name_table * nt, const ref * pnref)
{
    return names_index_inline(nt, pnref);
}
void
names_index_ref(const name_table * nt, name_index_t index, ref * pnref)
{
    names_index_ref_inline(nt, index, pnref);
}
name *
names_index_ptr(const name_table * nt, name_index_t index)
{
    return names_index_ptr_inline(nt, index);
}

/* Get the index of the next valid name. */
/* The argument is 0 or a valid index. */
/* Return 0 if there are no more. */
name_index_t
names_next_valid_index(name_table * nt, name_index_t nidx)
{
    const name_string_sub_table_t *ssub =
	nt->sub[nidx >> nt_log2_sub_size].strings;
    const name_string_t *pnstr;

    do {
	++nidx;
	if ((nidx & nt_sub_index_mask) == 0)
	    for (;; nidx += nt_sub_size) {
		if ((nidx >> nt_log2_sub_size) >= nt->sub_count)
		    return 0;
		ssub = nt->sub[nidx >> nt_log2_sub_size].strings;
		if (ssub != 0)
		    break;
	    }
	pnstr = &ssub->strings[nidx & nt_sub_index_mask];
    }
    while (pnstr->string_bytes == 0);
    return nidx;
}

/* ------ Garbage collection ------ */

/* Unmark all non-permanent names before a garbage collection. */
void
names_unmark_all(name_table * nt)
{
    uint si;
    name_string_sub_table_t *ssub;

    for (si = 0; si < nt->sub_count; ++si)
	if ((ssub = nt->sub[si].strings) != 0) {
	    uint i;

	    /* We can make the test much more efficient if we want.... */
	    for (i = 0; i < nt_sub_size; ++i)
		if (name_index_to_count((si << nt_log2_sub_size) + i) >=
		    nt->perm_count)
		    ssub->strings[i].mark = 0;
	}
}

/* Mark a name.  Return true if new mark.  We export this so we can mark */
/* character names in the character cache. */
bool
names_mark_index(name_table * nt, name_index_t nidx)
{
    name_string_t *pnstr = names_index_string_inline(nt, nidx);

    if (pnstr->mark)
	return false;
    pnstr->mark = 1;
    return true;
}

/* Get the object (sub-table) containing a name. */
/* The garbage collector needs this so it can relocate pointers to names. */
void /*obj_header_t */ *
names_ref_sub_table(name_table * nt, const ref * pnref)
{
    /* When this procedure is called, the pointers from the name table */
    /* to the sub-tables may or may not have been relocated already, */
    /* so we can't use them.  Instead, we have to work backwards from */
    /* the name pointer itself. */
    return pnref->value.pname - (r_size(pnref) & nt_sub_index_mask);
}
void /*obj_header_t */ *
names_index_sub_table(name_table * nt, name_index_t index)
{
    return nt->sub[index >> nt_log2_sub_size].names;
}
void /*obj_header_t */ *
names_index_string_sub_table(name_table * nt, name_index_t index)
{
    return nt->sub[index >> nt_log2_sub_size].strings;
}

/*
 * Clean up the name table after the trace/mark phase of a garbage
 * collection, by removing names that aren't marked.  gcst == NULL indicates
 * we're doing this for initialization or restore rather than for a GC.
 */
void
names_trace_finish(name_table * nt, gc_state_t * gcst)
{
    uint *phash = &nt->hash[0];
    uint i;

    for (i = 0; i < NT_HASH_SIZE; phash++, i++) {
	name_index_t prev = 0;
	/*
	 * The following initialization is only to pacify compilers:
	 * pnprev is only referenced if prev has been set in the loop,
	 * in which case pnprev is also set.
	 */
	name_string_t *pnprev = 0;
	name_index_t nidx = *phash;

	while (nidx != 0) {
	    name_string_t *pnstr = names_index_string_inline(nt, nidx);
	    name_index_t next = name_next_index(nidx, pnstr);

	    if (pnstr->mark) {
		prev = nidx;
		pnprev = pnstr;
	    } else {
		if_debug_name("GC remove name", nt, nidx, NULL);
		/* Zero out the string data for the GC. */
		pnstr->string_bytes = 0;
		pnstr->string_size = 0;
		if (prev == 0)
		    *phash = next;
		else
		    set_name_next_index(prev, pnprev, next);
	    }
	    nidx = next;
	}
    }
    /* Reconstruct the free list. */
    nt->free = 0;
    for (i = nt->sub_count; i--;) {
	name_sub_table *sub = nt->sub[i].names;
	name_string_sub_table_t *ssub = nt->sub[i].strings;

	if (sub != 0) {
	    name_scan_sub(nt, i, true);
	    if (nt->sub[i].names == 0 && gcst != 0) {
		/* Mark the just-freed sub-table as unmarked. */
		o_set_unmarked((obj_header_t *)sub - 1);
		o_set_unmarked((obj_header_t *)ssub - 1);
	    }
	}
	if (i == 0)
	    break;
    }
    nt->sub_next = 0;
}

/* ------ Save/restore ------ */

/* Clean up the name table before a restore. */
/* Currently, this is never called, because the name table is allocated */
/* in system VM.  However, for a Level 1 system, we might choose to */
/* allocate the name table in global VM; in this case, this routine */
/* would be called before doing the global part of a top-level restore. */
/* Currently we don't make any attempt to optimize this. */
void
names_restore(name_table * nt, alloc_save_t * save)
{
    /* We simply mark all names older than the save, */
    /* and let names_trace_finish sort everything out. */
    uint si;

    for (si = 0; si < nt->sub_count; ++si)
	if (nt->sub[si].strings != 0) {
	    uint i;

	    for (i = 0; i < nt_sub_size; ++i) {
		name_string_t *pnstr =
		    names_index_string_inline(nt, (si << nt_log2_sub_size) + i);

		if (pnstr->string_bytes == 0)
		    pnstr->mark = 0;
		else if (pnstr->foreign_string) {
		    /* Avoid storing into a read-only name string. */
		    if (!pnstr->mark)
			pnstr->mark = 1;
		} else
		    pnstr->mark =
			!alloc_is_since_save(pnstr->string_bytes, save);
	    }
	}
    names_trace_finish(nt, NULL);
}

/* ------ Internal procedures ------ */

/* Allocate the next sub-table. */
private int
name_alloc_sub(name_table * nt)
{
    gs_memory_t *mem = nt->memory;
    uint sub_index = nt->sub_next;
    name_sub_table *sub;
    name_string_sub_table_t *ssub;

    for (;; ++sub_index) {
	if (sub_index > nt->max_sub_count)
	    return_error(e_limitcheck);
	if (nt->sub[sub_index].names == 0)
	    break;
    }
    nt->sub_next = sub_index + 1;
    if (nt->sub_next > nt->sub_count)
	nt->sub_count = nt->sub_next;
    sub = gs_alloc_struct(mem, name_sub_table, &st_name_sub_table,
			  "name_alloc_sub(sub-table)");
    ssub = gs_alloc_struct(mem, name_string_sub_table_t,
			   &st_name_string_sub_table,
			  "name_alloc_sub(string sub-table)");
    if (sub == 0 || ssub == 0) {
	gs_free_object(mem, ssub, "name_alloc_sub(string sub-table)");
	gs_free_object(mem, sub, "name_alloc_sub(sub-table)");
	return_error(e_VMerror);
    }
    memset(sub, 0, sizeof(name_sub_table));
    memset(ssub, 0, sizeof(name_string_sub_table_t));
    /* The following code is only used if EXTEND_NAMES is non-zero. */
#if name_extension_bits > 0
    sub->high_index = (sub_index >> (16 - nt_log2_sub_size)) << 16;
#endif
    nt->sub[sub_index].names = sub;
    nt->sub[sub_index].strings = ssub;
    /* Add the newly allocated entries to the free list. */
    /* Note that the free list will only be properly sorted if */
    /* it was empty initially. */
    name_scan_sub(nt, sub_index, false);
#ifdef DEBUG
    if (gs_debug_c('n')) {	/* Print the lengths of the hash chains. */
	int i0;

	for (i0 = 0; i0 < NT_HASH_SIZE; i0 += 16) {
	    int i;

	    dlprintf1("[n]chain %d:", i0);
	    for (i = i0; i < i0 + 16; i++) {
		int n = 0;
		uint nidx;

		for (nidx = nt->hash[i]; nidx != 0;
		     nidx = name_next_index(nidx,
					    names_index_string_inline(nt, nidx))
		    )
		    n++;
		dprintf1(" %d", n);
	    }
	    dputc('\n');
	}
    }
#endif
    return 0;
}

/* Free a sub-table. */
private void
name_free_sub(name_table * nt, uint sub_index)
{
    gs_free_object(nt->memory, nt->sub[sub_index].strings,
		   "name_free_sub(string sub-table)");
    gs_free_object(nt->memory, nt->sub[sub_index].names,
		   "name_free_sub(sub-table)");
    nt->sub[sub_index].names = 0;
    nt->sub[sub_index].strings = 0;
}

/* Scan a sub-table and add unmarked entries to the free list. */
/* We add the entries in decreasing count order, so the free list */
/* will stay sorted.  If all entries are unmarked and free_empty is true, */
/* free the sub-table. */
private void
name_scan_sub(name_table * nt, uint sub_index, bool free_empty)
{
    name_string_sub_table_t *ssub = nt->sub[sub_index].strings;
    uint free = nt->free;
    uint nbase = sub_index << nt_log2_sub_size;
    uint ncnt = nbase + (nt_sub_size - 1);
    bool keep = !free_empty;

    if (ssub == 0)
	return;
    if (nbase == 0)
	nbase = 1, keep = true;	/* don't free name 0 */
    for (;; --ncnt) {
	uint nidx = name_count_to_index(ncnt);
	name_string_t *pnstr = &ssub->strings[nidx & nt_sub_index_mask];

	if (pnstr->mark)
	    keep = true;
	else {
	    set_name_next_index(nidx, pnstr, free);
	    free = nidx;
	}
	if (ncnt == nbase)
	    break;
    }
    if (keep)
	nt->free = free;
    else {
	/* No marked entries, free the sub-table. */
	name_free_sub(nt, sub_index);
	if (sub_index == nt->sub_count - 1) {
	    /* Back up over a final run of deleted sub-tables. */
	    do {
		--sub_index;
	    } while (nt->sub[sub_index].names == 0);
	    nt->sub_count = sub_index + 1;
	    if (nt->sub_next > sub_index)
		nt->sub_next = sub_index;
	} else if (nt->sub_next == sub_index)
	    nt->sub_next--;
    }
}

/* Garbage collector enumeration and relocation procedures. */
private 
ENUM_PTRS_BEGIN_PROC(name_table_enum_ptrs)
{
    EV_CONST name_table *const nt = vptr;
    uint i = index >> 1;

    if (i >= nt->sub_count)
	return 0;
    if (index & 1)
	ENUM_RETURN(nt->sub[i].strings);
    else
	ENUM_RETURN(nt->sub[i].names);
}
ENUM_PTRS_END_PROC
private RELOC_PTRS_WITH(name_table_reloc_ptrs, name_table *nt)
{
    uint sub_count = nt->sub_count;
    uint i;

    /* Now we can relocate the sub-table pointers. */
    for (i = 0; i < sub_count; i++) {
	RELOC_VAR(nt->sub[i].names);
	RELOC_VAR(nt->sub[i].strings);
    }
    /*
     * We also need to relocate the cached value pointers.
     * We don't do this here, but in a separate scan over the
     * permanent dictionaries, at the very end of garbage collection.
     */
}
RELOC_PTRS_END

private ENUM_PTRS_BEGIN_PROC(name_string_sub_enum_ptrs)
{
    return 0;
}
ENUM_PTRS_END_PROC
private RELOC_PTRS_BEGIN(name_string_sub_reloc_ptrs)
{
    name_string_t *pnstr = ((name_string_sub_table_t *)vptr)->strings;
    uint i;

    for (i = 0; i < nt_sub_size; ++pnstr, ++i) {
	if (pnstr->string_bytes != 0 && !pnstr->foreign_string) {
	    gs_const_string nstr;

	    nstr.data = pnstr->string_bytes;
	    nstr.size = pnstr->string_size;
	    RELOC_CONST_STRING_VAR(nstr);
	    pnstr->string_bytes = nstr.data;
	}
    }
}
RELOC_PTRS_END
