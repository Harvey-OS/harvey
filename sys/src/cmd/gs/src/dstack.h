/* Copyright (C) 1992, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: dstack.h,v 1.1 2000/03/09 08:40:40 lpd Exp $ */
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
#define dict_find_name(pnref) dict_find_name_by_index(name_index(pnref))
#define dict_find_name_by_index_inline(nidx, htemp)\
  dstack_find_name_by_index_inline(&idict_stack, nidx, htemp)
#define if_dict_find_name_by_index_top(nidx, htemp, pvslot)\
  if_dstack_find_name_by_index_top(&idict_stack, nidx, htemp, pvslot)

/*
Notes on dictionary lookup performance
--------------------------------------

We mark heavily used operations with a * below; moderately heavily used
operations with a +.

The following operations change the dictionary stack:
	+begin, +end
	readonly (on a dictionary that is on the stack)
	noaccess (on a dictionary that is on the stack)
	context switch
We implement cleardictstack as a series of ends.

The following operations change the contents of dictionaries:
	*def, +put
	undef
	restore
	.setmaxlength
We implement store in PostScript, and copy as a series of puts.  Many
other operators also do puts (e.g., ScaleMatrix in makefont,
Implementation in makepattern, ...).  Note that put can do an implicit
.setmaxlength (if it has to grow the dictionary).

The following operations look up keys on the dictionary stack:
	*(interpreter name lookup)
	load
	where

Current design
--------------

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

Cheap shallow binding
---------------------

We define a global event counter, Event.  Each name N has, in addition to
its value pointer N.V, an associated event stamp N.E.  The invariant we want
to preserve is that N.V is valid iff N.E >= Event.  Similarly, each entry B
on the dictionary stack has, in addition to its dictionary B.D, an
associated event stamp B.E which is the value of Event when the entry was
made, and a slow lookup flag B.F which indicates whether any slow lookups
have been done since the entry was made.  Finally, each dictionary D has a
counter D.N which indicates how many times it appears on the dictionary
stack.  (The counter is an optional feature of this scheme: if we omit it,
we consider it to have a permanently non-zero value in all dictionaries.)

The idea of the B.F flag is for 'end' to be able to reset Event, rather than
incrementing it, if no names had their stamp set to the incremented Event
value.  This in turn prevents the mass invalidation of the N.V cache that
would otherwise occur.

Here are the implementations of the various operations with respect to the
cache.  We preserve the current scheme for fast implementation of 'def',
which we don't discuss here.

Initialize:
	Event = 0
When creating a name:
	N.V = NULL, N.E = 0
begin(D):
	create B with B.D = D, B.E = Event, B.F = false
	++Event
end(B):
	if !B.F, Event = B.E; else ++Event, and set B.F in the new top entry
readonly(D):
	<< nothing >>
noaccess(D):
	if D.N, ++Event
def(D,K,V):
	if K is already defined in D or K is not a name N, nothing to do
	if N.V is NULL and D is systemdict,
	  set N.V to point into D, N.E = infinite
	else
	  set N.V to point into D, N.E = Event, B.F = true in top entry
put(D,K,V):
	if K is already defined in D, K is not a name N, or D.N == 0,
	  nothing to do
	if N.V is NULL and D is systemdict,
	  set N.V to point into D, N.E = infinite
	else if D is the top dict on the dict stack
	  set N.V to point into D, N.E = Event, B.F = true in top entry
	else
	  set N.V = invalid, N.E = 0
undef(D,K):
	if K is defined in D, D.N > 0, and K is a name N, clear N.E
restore:
	****** TBC ******
.setmaxlength(D):
	for each name key N in D, if N.V points into D, repoint it
value = lookup(N): (load and where are similar)
	<< do the fast lookup in the top dict >>
	if N.E >= Event, use N.V
	otherwise, do the dict stack lookup
	set B.F in the top dict stack entry
	set N.E = Event, N.V = the binding pointer
context switch:
	++Event
	for each B on the old dict stack, --(B.D.N)
	for each B on the new dict stack, ++(B.D.N)
Event counter overflow:
	clear N.E in all names where it isn't infinite

Full shallow binding
--------------------

We implement shallow binding with a pointer in each name that points to
the value slot that holds the name's definition.  If the name is
undefined, or if we don't know where the slot is, the binding pointer
points to a ref with a special type t__invalid, which cannot occur
anywhere else.  "Clearing" the pointer means setting it to point to this
ref.

We also maintain a pair of pointers that bracket the value region of the
top dictionary on the stack, for fast checking in def.  If the top
dictionary is readonly or noaccess, the pointers designate an empty area.
We call this the "def region" cache.

We implement the above operations as follows:
	begin - push the dictionary on the stack; set the pointers of
		all name keys to point to the corresponding value slots.
	end - pop the stack; clear the pointers of all name keys.
	readonly - if the dictionary is the top one on the stack,
		reset the def region cache.
	noaccess - clear the pointers of all name keys.  (This is overly
		conservative, but this is a very rare operation.)
		Also reset the def region cache if the dictionary is
		the top one on the stack.
	def - if the key is a name and its pointer points within the cached
		def region, store the value through the pointer; otherwise,
		look up the key in the top dictionary, store the value,
		and if the key is a name, set its pointer to the value slot.
	put - if the key is a name and wasn't in the dictionary before,
		clear its pointer.  (Conservative, but rare.)
	undef - if the key is a name, clear its pointer.  (Overly
		conservative, but rare.)
	restore - if either the old or the new value of a change is a name
		(possibly in a packed array), clear its pointer.  This is
		conservative, but easy to detect, and probably not *too*
		conservative.
	.setmaxlength - clear all the pointers, like noaccess.
	(name lookup) - fetch the value through the pointer and dispatch
		on its type; if the type is t__invalid, do a full search
		and set the pointer.  This avoids a separate check for a
		clear pointer in the usual case where the pointer is valid.
	load - if the pointer is clear, do a search and set the pointer;
		then fetch the value.
	where - always do a full search and set the pointer.
		(Conservative, but rare.)

One place where shallow binding will result in major new overhead is the
extra push of systemdict for loading fonts.  This probably isn't a problem
in real life.

****** Context switching is horrendously expensive: it has to do the
equivalent of 'end' for every dictionary on the old stack followed by
'begin' for every dictionary on the new stack.

Adaptive shallow binding
------------------------

We do validity checking for the name value cache using an epoch counter.
For each dictionary D, we keep an on-stack flag F.  Each dictionary stack
entry is <D,M,F,E> where D is the actual dictionary, M is a mark vector of
V bits (V is a system constant, probably 64), F is D's former on-stack
flag, and E is the epoch at which the entry was made.  For each name K, we
keep a cache <P,E> where P is a pointer to the dictionary value slot that
holds the current value of K, and E is an epoch value; the cache is valid
if K->E >= dsp->E.  Here is what happens for each operation:

****** Still need to handle names defined only in systemdict or userdict?

To initialize:
	Epoch = 0
To clear the cache entry for K:
	*K = <ptr to invalid value, 0>
begin(D):
	*++dsp = <D, {0...}, D->F, ++Epoch>
	set D->F
value = lookup(K):
	if K->E >= dsp->E
	  value = *K->P
	else
	  do lookup as usual
	  *K = <ptr to value, Epoch>
	  set dp->M[i mod V] where dp is the dstack slot of the dictionary
	    where K was found and i is the index within that dictionary
end:
	for each i such that dsp->M[i] is set,
	  clear the cache entry for dsp->D->keys[i, i+V, ...]
	dsp->D->F = dsp->F
	--dsp
noaccess(D):
	if D->F is set,
	  clear the cache entries for all name keys of D
readonly(D):
	<< nothing >>
.setmaxlength(D,N):
	same as noaccess
restore:
	If either the old or the new value of a change is a name
	  (possibly in a packed array), clear its cache entry.  This is
	  conservative, but easy to detect, and probably not *too*
	  conservative.
def(K,V):
	if K->P points into dsp->D
	  *K->P = V
	else
	  put the new value in dsp->D
	  set *K and dsp->M[i mod V] as for a lookup
put(D,K,V):
	if K is already defined in D, do nothing special
	otherwise, if D->F isn't set, do nothing special
	otherwise, clear K's cache entry
undef(D,K):
	if D->F is set,
	  clear K's cache entry

****** Same problem for context switching as for full shallow binding.

 */

#endif /* dstack_INCLUDED */
