/* Copyright (C) 1992, 1996, 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: dstack.h,v 1.6 2004/08/04 19:36:12 stefan Exp $ */
/* Definitions for the interpreter's dictionary stack */

#ifndef dstack_INCLUDED
#  define dstack_INCLUDED

#include "idstack.h"
#include "icstate.h"		/* for access to dict_stack */

/* Define the dictionary stack instance for operators. */
#define idict_stack (i_ctx_p->dict_stack)
#define d_stack (idict_stack.stack)

/* Define the interpreter-specific versions of the generic dstack API. */
#define min_dstack_size (idict_stack.min_size)
#define dstack_userdict_index (idict_stack.userdict_index)
#define dsspace (idict_stack.def_space)
#define dtop_can_store(pvalue) ((int)r_space(pvalue) <= dsspace)
#define dtop_keys (idict_stack.top_keys)
#define dtop_npairs (idict_stack.top_npairs)
#define dtop_values (idict_stack.top_values)
#define dict_set_top() dstack_set_top(&idict_stack);
#define dict_is_permanent_on_dstack(pdict)\
  dstack_dict_is_permanent(&idict_stack, pdict)
#define dicts_gc_cleanup() dstack_gc_cleanup(&idict_stack)
#define systemdict (&idict_stack.system_dict)

/* Define the dictionary stack pointers. */
#define dsbot (d_stack.bot)
#define dsp (d_stack.p)
#define dstop (d_stack.top)

/* Macro to ensure enough room on the dictionary stack */
#define check_dstack(n)\
  if ( dstop - dsp < (n) )\
    { d_stack.requested = (n); return_error(e_dictstackoverflow); }

/*
 * The dictionary stack is implemented as a linked list of blocks;
 * operators that access the entire d-stack must take this into account.
 * These are:
 *      countdictstack  dictstack
 * In addition, name lookup requires searching the entire stack, not just
 * the top block, and the underflow check for the dictionary stack
 * (`end' operator) is not just a check for underflowing the top block.
 */

/* Name lookup */
#define dict_find_name_by_index(nidx)\
  dstack_find_name_by_index(&idict_stack, nidx)
#define dict_find_name(pnref) dict_find_name_by_index(name_index(imemory, pnref))
#define dict_find_name_by_index_inline(nidx, htemp)\
  dstack_find_name_by_index_inline(&idict_stack, nidx, htemp)
#define if_dict_find_name_by_index_top(nidx, htemp, pvslot)\
  if_dstack_find_name_by_index_top(&idict_stack, nidx, htemp, pvslot)

/*
Notes on dictionary lookup performance
======================================

We mark heavily used operations with a * below; moderately heavily used
operations with a +.

The following operations look up keys on the dictionary stack:
	*(interpreter name lookup)
	load
	where

The following operations change the contents of dictionaries:
	*def, +put
	undef
	restore
	(grow)
We implement store in PostScript, and copy as a series of puts.  Many
other operators also do puts (e.g., ScaleMatrix in makefont,
Implementation in makepattern, ...).  Note that put can do an implicit
.setmaxlength (if it has to grow the dictionary).

The following operations change the dictionary stack:
	+begin, +end
	+?(context switch)
	readonly (on a dictionary that is on the stack)
	noaccess (on a dictionary that is on the stack)
We implement cleardictstack as a series of ends.

Current design
==============

Each name N has a pointer N.V that has one of 3 states:
	- This name has no definitions.
	- This name has exactly one definition, in systemdict or userdict.
	In this case, N.V points to the value slot.
	- This name has some other status.

We cache some pointers to the top dictionary on the stack if it is a
readable dictionary with packed keys, which allows us to do fast,
single-probe lookups in this dictionary.  We also cache a value that
allows us to do a fast check for stores into the top dictionary
(writability + space check).

Improved design
===============

Data structures
---------------

With each dictionary stack (or equivalently with each context), we
associate:

    * A name lookup cache, C.  Each entry C[i] in the cache consists of:

	A key, K (a name index).

	A dictionary stack level (depth), L.  C[i] is valid iff the
	current dictionary stack depth, |dstack|, is equal to L.
	(L is always less than or equal to |dstack|.)

	A value pointer, V, which points to some value slot of some
	dictionary on the stack.

    * A lookup cache restoration stack, R.  Each entry R[j] on this stack
    consists of:

	An index i in C.

	The previous (K,D,V) of C[i].

C needs to be large enough to satisfy the overwhelming majority of name
lookups with only 1-3 probes.  In a single-context system, C can be large
(perhaps 8K entries = 80K bytes, which is enough to accommodate every name
in a typical run with no reprobes).  In a multiple-context system, one can
choose a variety of different strategies for managing C, such as:

	A single cache that is cleared on every context switch;

	A small cache (e.g., .5K entries) for each context;

	A cache that starts out small and grows adaptively if the hit rate
	is too low.

R needs to be able to grow dynamically; in the worst case, it may have |C|
entries per level of the dictionary stack.  We assume that R will always be
able to grow as needed (i.e., inability to allocate space for R is a
VMerror, like inability to allocate space for the undo-save information for
'save').

With each entry E[j] on the dictionary stack, we associate:

    * A value U that gives the depth of R at the time that E[j] was pushed
    on the stack.  E[j].U = 0 for 0 <= j < the initial depth of the dictionary
    stack (normally 3).

With each dictionary D we associate:

    * A counter S that gives the total number of occurrences of D on all
    dictionary stacks.  If this counter overflows, it is pinned at its maximum
    value.

In order to be effective, D.S needs to be able to count up to a small
multiple of the total number of contexts: 16 bits should be plenty.

As at present, we also maintain a pair of pointers that bracket the value
region of the top dictionary on the stack, for fast checking in def.  If the
top dictionary is readonly or noaccess, the pointers designate an empty
area.  We call this the "def region cache".

Now we describe the implementation of each of the above operations.

(name lookup)
-------------

To look up a name with index N, compute a hash index 0 <= i < |C|.  There
are three cases:

	1. C[i].K == N and C[i].L == |dstack|.  Nothing more is needed:
	C[i].V points to the N's value.

	2. C[i].K == N but C[i].L < |dstack|.  Look up N in the top |dstack|
	- L entries on the dictionary stack; push i and C[i] onto R; set
	C[i].V to point to the value if found, and in any case set C[i].L =
	|dstack|.

	3. C[i].K != N.  Reprobe some small number of times.

If all reprobes fail, look up N on the (full) dictionary stack.  Pick an
index i (one of the probed entries) in C to replace.  If C[i].L != |dstack|,
push i and C[i] onto R.  Then replace C[i] with K = N, L = |dstack|, and V
pointing to N's value.

load
----

Proceed as for name lookup.  However, it might be worth not making the new
cache entry in case 3, since names looked up by load will rarely be looked
up again.

where
-----

Just do a full lookup, ignoring C.

def
---

As at present: start by doing one or two fast probes in the def region
cache; if they succeed, just store the new value; otherwise, do a normal
dictionary lookup and access check.  If a new dictionary entry is created
and the key is a name, check all possible probe slots of the name in C; if
the name is present, update its entry in C as for a lookup.  Then if D.S >
1, scan as for 'grow' below.

put
---

If the key is a name, the dictionary entry is new, and D.S != 0, scan as for
'grow' below.

undef
-----

If the key is a name and D.S != 0, scan as for 'grow' below.  It might be
worth checking for D.S == 1 and D = the top dictionary on the stack as a
special case, which only requires removing the name from C, similar to
'def'.

restore
-------

The undo-save machinery must be enhanced so that grow, def/put, and undef
operations can be recognized as such.  TBD.

(grow)
------

If D.S == 0, do nothing special.  Otherwise, scan C, R, and the dictionary
stack (first for the current context, and then for other contexts if needed)
until D.S occurrences of D have been processed.  (If D is in local VM, it is
only necessary to scan contexts that share local VM with the current one; if
D is in global VM, it is necessary to scan contexts that share global VM
with the current one.)  Entries in C whose V pointed within D's old values
array are updated; entries in R whose V pointed within the old values array
are replaced with empty entries.

begin
-----

After pushing the new dictionary, set dstack[|dstack| - 1].U = |R|.  Reset
the def region cache.

end
---

Before popping the top entry, for dstack[|dstack| - 1].U <= j < |R|, restore
C[R[j].i] from R[j].(K,L,V), popping R.  Reset the def region cache.

(context switch)
----------------

Reset the def region cache.

readonly
--------

If the dictionary is the top one on the stack, reset the def region cache.

noaccess
--------

If D.S != 0, scan as for 'grow' above, removing every C and R entry whose V
points into D.  Also reset the def region cache if the dictionary is the top
one on the stack.

(garbage collection)
--------------------

The garbage collector must mark names referenced from C and R.  Dictionaries
referenced from C and R are also referenced from the dictionary stack, so
they do not need to be marked specially; however, pointers to those
dictionaries' values arrays from C and R need to be relocated.

 */

#endif /* dstack_INCLUDED */
