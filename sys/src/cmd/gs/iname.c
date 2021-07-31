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

/* iname.c */
/* Name lookup for Ghostscript interpreter */
#include "memory_.h"
#include "ghost.h"
#include "gsstruct.h"
#include "errors.h"
#include "iname.h"
#include "store.h"

/* In the code below, we use the hashing method described in */
/* "Fast Hashing of Variable-Length Text Strings" by Peter K. Pearson, */
/* pp. 677-680, CACM 33(6), June 1990. */

/* Define a pseudo-random permutation of the integers 0..255. */
/* Pearson's article claims this permutation gave good results. */
private const far_data byte hash_permutation[256] = {
  1,  87,  49,  12, 176, 178, 102, 166, 121, 193,   6,  84, 249, 230,  44, 163,
 14, 197, 213, 181, 161,  85, 218,  80,  64, 239,  24, 226, 236, 142,  38, 200,
110, 177, 104, 103, 141, 253, 255,  50,  77, 101,  81,  18,  45,  96,  31, 222,
 25, 107, 190,  70,  86, 237, 240,  34,  72, 242,  20, 214, 244, 227, 149, 235,
 97, 234,  57,  22,  60, 250,  82, 175, 208,   5, 127, 199, 111,  62, 135, 248,
174, 169, 211,  58,  66, 154, 106, 195, 245, 171,  17, 187, 182, 179,   0, 243,
132,  56, 148,  75, 128, 133, 158, 100, 130, 126,  91,  13, 153, 246, 216, 219,
119,  68, 223,  78,  83,  88, 201,  99, 122,  11,  92,  32, 136, 114,  52,  10,
138,  30,  48, 183, 156,  35,  61,  26, 143,  74, 251,  94, 129, 162,  63, 152,
170,   7, 115, 167, 241, 206,   3, 150,  55,  59, 151, 220,  90,  53,  23, 131,
125, 173,  15, 238,  79,  95,  89,  16, 105, 137, 225, 224, 217, 160,  37, 123,
118,  73,   2, 157,  46, 116,   9, 145, 134, 228, 207, 212, 202, 215,  69, 229,
 27, 188,  67, 124, 168, 252,  42,   4,  29, 108,  21, 247,  19, 205,  39, 203,
233,  40, 186, 147, 198, 192, 155,  33, 164, 191,  98, 204, 165, 180, 117,  76,
140,  36, 210, 172,  41,  54, 159,   8, 185, 232, 113, 196, 231,  47, 146, 120,
 51,  65,  28, 144, 254, 221,  93, 189, 194, 139, 112,  43,  71, 109, 184, 209
};

/*
 * Definitions and structure for the name table.
 * The very first entry is left unused.
 * 1-character names are the next nt_1char_size entries.
 */
#define nt_1char_size 128
/* Get the pointer to a name with a given index. */
#define nt_index_ptr(nt, nidx)\
  name_index_ptr_inline(nt, nidx)

gs_private_st_simple(st_name_sub_table, name_sub_table, "name_sub_table");
gs_private_st_composite(st_name_table, name_table,
  "name_table", name_table_enum_ptrs, name_table_reloc_ptrs);

/* The one and only name table (for now). */
private name_table *the_nt;
private gs_gc_root_t the_nt_root;

/* Forward references */
private int name_alloc_sub(P1(name_table *));

/* Debugging printout */
#ifdef DEBUG
private void
name_print(const char *msg, name *pname, uint nidx, uint ncnt,
  const int *pflag)
{	const byte *ptr = pname->string_bytes;
	dprintf1("[n]%s", msg);
	if ( pflag )
	  dprintf1("(%d)", *pflag);
	dprintf2(" (0x%lx#%u)", (ulong)pname, nidx);
	debug_print_string(ptr, pname->string_size);
	dprintf3("(0x%lx,%u), count=%u\n",
		 (ulong)ptr, pname->string_size, ncnt);
}
#  define if_debug_name(msg, pname, nidx, ncnt, pflag)\
     if ( gs_debug_c('n') ) name_print(msg, pname, nidx, ncnt, pflag)
#else
#  define if_debug_name(msg, pname, nidx, ncnt, pflag) DO_NOTHING
#endif

/* Initialize the name table */
name_table *
name_init(gs_memory_t *mem)
{	register uint i;
	name_table *nt =
	  gs_alloc_struct(mem, name_table, &st_name_table, "name_init(nt)");
	byte *one_char_names =
	  gs_alloc_string(mem, nt_1char_size, "name_init(one_char_names)");
	the_nt = nt;
	memset(nt, 0, sizeof(name_table));
	set_nt_count(nt, 1);
	nt->memory = mem;
	/* Initialize the one-character names. */
	for ( i = 0; i < nt_1char_size; i++ )
	{	uint ncnt = i + 1;
		uint nidx = name_count_to_index(ncnt);
		register name *pname;
		if ( nt->table[nidx >> nt_log2_sub_size] == 0 )
			name_alloc_sub(nt);
		pname = nt_index_ptr(nt, nidx);
		set_nt_count(nt, ncnt + 1);
		one_char_names[i] = i;
		pname->string_size = 1;
		pname->foreign_string = 0;
		pname->string_bytes = one_char_names + i;
		pname->pvalue = pv_no_defn;
	}
	/* Register the name table root. */
	gs_register_struct_root(mem, &the_nt_root, (void **)&the_nt,
				"name table");
	return nt;
}

/* Return the one and only table. */
const name_table *
the_name_table(void)
{	return the_nt;
}

/* Look up or enter a name in the table. */
/* Return 0 or an error code. */
/* The return may overlap the characters of the string! */
/* See iname.h for the meaning of enterflag. */
int
name_ref(const byte *ptr, uint size, ref *pref, int enterflag)
{	name_table *nt = the_nt;
	register name *pname;
	const byte *cptr;
	uint nidx, ncnt;
	ushort *phash;
	/* Compute a hash for the string. */
	{	uint hash;
		const byte *p = ptr;
		uint n = size;
		/* Make a special check for 1-character names. */
		switch ( size )
		  {
		  case 0:
			hash = 0;
			break;
		  case 1:
			if ( *p < nt_1char_size )
			  {	hash = *p + 1;
				nidx = name_count_to_index(hash);
				pname = nt_index_ptr(nt, nidx);
				make_name(pref, nidx, pname);
				return 0;
			  }
		  default:
			hash = hash_permutation[*p++];
			while ( --n > 0 )
			  hash = (hash << 8) |
			    hash_permutation[(byte)hash ^ *p++];
		  }
		phash = nt->hash + (hash & (nt_hash_size - 1));
	}

	for ( nidx = *phash; nidx != 0; nidx = pname->next_index )
	{	pname = nt_index_ptr(nt, nidx);
		if ( pname->string_size == size &&
		     !memcmp_inline(ptr, pname->string_bytes, size)
		   )
		{	make_name(pref, nidx, pname);
			return 0;
		}
	}
	/* Name was not in the table.  Make a new entry. */
	if ( enterflag < 0 )
		return_error(e_undefined);
	ncnt = nt_count(nt);
	if ( !(ncnt & (nt_sub_size - 1)) )
	   {	int code = name_alloc_sub(nt);
		if ( code < 0 ) return code;
	   }
	nidx = name_count_to_index(ncnt);
	pname = nt_index_ptr(nt, nidx);
	if ( enterflag == 1 )
	{	cptr = (const byte *)gs_alloc_string(nt->memory, size,
						     "name_ref(string)");
		if ( cptr == 0 )
		  {	/* If we just allocated a sub-table, deallocate it */
			/* before bailing out. */
			if ( !(ncnt & (nt_sub_size - 1)) )
			  gs_free_object(nt->memory,
					 nt->table[ncnt >> nt_log2_sub_size],
					 "name_ref(sub_table)");
			return_error(e_VMerror);
		  }
		memcpy((byte *)cptr, ptr, size);
		pname->foreign_string = 0;
	}
	else
	{	cptr = ptr;
		pname->foreign_string = (enterflag == 0 ? 1 : 0);
	}
	pname->string_size = size;
	pname->string_bytes = cptr;
	pname->pvalue = pv_no_defn;
	pname->next_index = *phash;
	*phash = nidx;
	set_nt_count(nt, ncnt + 1);
	make_name(pref, nidx, pname);
	if_debug_name("new name", pname, nidx, name_index_to_count(nidx),
		      &enterflag);
	return 0;
}

/* Get the string for a name. */
void
name_string_ref(const ref *pnref /* t_name */,
  ref *psref /* result, t_string */)
{	name *pname = pnref->value.pname;
	const name_table *nt = the_nt;
	make_const_string(psref,
			  (pname->foreign_string ? avm_foreign :
			   imemory_space((gs_ref_memory_t *)nt->memory))
			   | a_readonly,
			  pname->string_size,
			  (const byte *)pname->string_bytes);
}

/* Convert a t_string object to a name. */
/* Copy the executable attribute. */
int
name_from_string(const ref *psref, ref *pnref)
{	int exec = r_has_attr(psref, a_executable);
	int code = name_ref(psref->value.bytes, r_size(psref), pnref, 1);
	if ( code < 0 )
	  return code;
	if ( exec )
	  r_set_attrs(pnref, a_executable);
	return code;
}

/* Enter a (permanently allocated) C string as a name. */
int
name_enter_string(const char *str, ref *pref)
{	return name_ref((const byte *)str, strlen(str), pref, 0);
}

/* Get the name with a given index. */
void
name_index_ref(uint index, ref *pnref)
{	name_index_ref_inline(the_nt, index, pnref);
}
name *
name_index_ptr(uint index)
{	return nt_index_ptr(the_nt, index);
}

/* Get the current name count. */
uint
name_count(void)
{	return nt_count(the_nt);
}

/* Get the allocator for names. */
gs_memory_t *
name_memory(void)
{	return the_nt->memory;
}

/* Mark a name.  Return true if new mark.  We export this so we can mark */
/* character names in the character cache. */
bool
name_mark_index(uint nidx)
{	name *pname = name_index_ptr(nidx);
	if ( pname->mark )
	  return false;
	pname->mark = 1;
	return true;
}

/* Get the object (sub-table) containing a name. */
/* The garbage collector needs this so it can relocate pointers to names. */
void/*obj_header_t*/ *
name_ref_sub_table(ref *pnref)
{	/* When this procedure is called, the pointers from the name table */
	/* to the sub-tables may or may not have been relocated already, */
	/* so we can't use them.  Instead, we have to work backwards from */
	/* the name pointer itself. */
	return pnref->value.pname - (name_index(pnref) & nt_sub_index_mask);
}
void/*obj_header_t*/ *
name_index_ptr_sub_table(uint index, name *pname)
{	return pname - (index & nt_sub_index_mask);
}

/* Clean up the name table before a restore. */
/* The count will be reset, and added subtables will be freed. */
/* All we have to do is remove initial entries from the hash chains, */
/* since we know they are linked in decreasing index order */
/* (by sub-table, but not within each sub-table.) */
/* Some spurious non-zero entries may remain in the subtable table, */
/* but this doesn't matter since they will never be accessed. */
void
name_restore(uint old_count)
{	name_table *nt = the_nt;
	ushort *phash = &nt->hash[0];
	uint old_sub = old_count & -nt_sub_size;
	register uint i;
	/* Often no new names will have been created since the save. */
	/* This is worth checking for. */
	if ( old_count >= nt_count(nt) )
		return;
	for ( i = 0; i < nt_hash_size; phash++, i++ )
	   {	register ushort *pnh = phash;
		while ( *pnh >= old_sub )
		   {	if ( name_index_to_count(*pnh) < old_count )
			  pnh = &nt_index_ptr(nt, *pnh)->next_index;
			else
			   {
#ifdef DEBUG
				if ( gs_debug_c('n') | gs_debug_c('U') )
				  name_print("restore remove name",
					     nt_index_ptr(nt, *pnh), *pnh,
					     name_index_to_count(*pnh),
					     NULL);
#endif
				*pnh = nt_index_ptr(nt, *pnh)->next_index;
			   }
		   }
	   }
	set_nt_count(nt, old_count);
}

/* Clean up the name table after a garbage collection, by removing */
/* names that aren't marked, and relocating name string pointers. */
/* Currently we don't reuse the deleted name slots, */
/* so some space is permanently lost. */
void
name_gc_cleanup(gc_state_t *gcst)
{	name_table *nt = the_nt;
	ushort *phash = &nt->hash[0];
	register uint i;
	for ( i = 0; i < nt_hash_size; phash++, i++ )
	{	register ushort *pnh = phash;
		while ( *pnh != 0 )
		{	name *pname = nt_index_ptr(nt, *pnh);
			if ( pname->mark )
			{	if ( !pname->foreign_string )
				  {	gs_const_string nstr;
					nstr.data = pname->string_bytes;
					nstr.size = pname->string_size;
					gs_reloc_const_string(&nstr, gcst);
					pname->string_bytes = nstr.data;
				  }
				pnh = &pname->next_index;
			}
			else
			{	if_debug_name("GC remove name", pname, *pnh,
					      name_index_to_count(*pnh),
					      NULL);
				*pnh = pname->next_index;
			}
		}
	}
}


/* ------ Internal procedures ------ */

/* Allocate the next sub-table. */
private int
name_alloc_sub(name_table *nt)
{	name *sub = gs_alloc_struct(nt->memory, name, &st_name_sub_table,
				    "name_alloc_sub");
	if ( sub == 0 )
		return_error(e_VMerror);
	memset(sub, 0, sizeof(name_sub_table));
	nt->table[nt_count(nt) >> nt_log2_sub_size] = sub;
#ifdef DEBUG
	if ( gs_debug_c('n') )
	  {	/* Print the lengths of the hash chains. */
		int i0;
		for ( i0 = 0; i0 < nt_hash_size; i0 += 16 )
		  {	int i;
			dprintf1("[n]chain %d:", i0);
			for ( i = i0; i < i0 + 16; i++ )
			  {	int n = 0;
				uint nidx;
				for ( nidx = nt->hash[i]; nidx != 0;
				      nidx = nt_index_ptr(nt, nidx)->next_index
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

/* Garbage collector enumeration and relocation procedures. */
#define ntptr ((name_table *)vptr)
private ENUM_PTRS_BEGIN_PROC(name_table_enum_ptrs) {
	if ( index > (nt_count(ntptr)  - 1) >> nt_log2_sub_size )
	  return 0;
	*pep = ntptr->table[index];
	return ptr_struct_type;
} ENUM_PTRS_END_PROC
private RELOC_PTRS_BEGIN(name_table_reloc_ptrs) {
	name_sub_table **sub = (name_sub_table **)ntptr->table;
	uint sub_count = (nt_count(ntptr) - 1) >> nt_log2_sub_size;
	uint i;
	/* Now we can relocate the sub-table pointers. */
	for ( i = 0; i <= sub_count; i++, sub++ )
	  *sub = gs_reloc_struct_ptr(*sub, gcst);
	/*
	 * We also need to relocate the cached value pointers.
	 * We don't do this here, but in a separate scan over the
	 * permanent dictionaries, at the very end of garbage collection.
	 */
} RELOC_PTRS_END
