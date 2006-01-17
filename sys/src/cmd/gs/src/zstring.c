/* Copyright (C) 1989, 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zstring.c,v 1.6 2004/08/04 19:36:13 stefan Exp $ */
/* String operators */
#include "memory_.h"
#include "ghost.h"
#include "gsutil.h"
#include "ialloc.h"
#include "iname.h"
#include "ivmspace.h"
#include "oper.h"
#include "store.h"

/* The generic operators (copy, get, put, getinterval, putinterval, */
/* length, and forall) are implemented in zgeneric.c. */

/* <int> .bytestring <bytestring> */
private int
zbytestring(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    byte *sbody;
    uint size;

    check_int_leu(*op, max_int);
    size = (uint)op->value.intval;
    sbody = ialloc_bytes(size, ".bytestring");
    if (sbody == 0)
	return_error(e_VMerror);
    make_astruct(op, a_all | icurrent_space, sbody);
    memset(sbody, 0, size);
    return 0;
}

/* <int> string <string> */
int
zstring(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    byte *sbody;
    uint size;

    check_int_leu(*op, max_string_size);
    size = op->value.intval;
    sbody = ialloc_string(size, "string");
    if (sbody == 0)
	return_error(e_VMerror);
    make_string(op, a_all | icurrent_space, size, sbody);
    memset(sbody, 0, size);
    return 0;
}

/* <name> .namestring <string> */
private int
znamestring(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_type(*op, t_name);
    name_string_ref(imemory, op, op);
    return 0;
}

/* <string> <pattern> anchorsearch <post> <match> -true- */
/* <string> <pattern> anchorsearch <string> -false- */
private int
zanchorsearch(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    uint size = r_size(op);

    check_read_type(*op1, t_string);
    check_read_type(*op, t_string);
    if (size <= r_size(op1) && !memcmp(op1->value.bytes, op->value.bytes, size)) {
	os_ptr op0 = op;

	push(1);
	*op0 = *op1;
	r_set_size(op0, size);
	op1->value.bytes += size;
	r_dec_size(op1, size);
	make_true(op);
    } else
	make_false(op);
    return 0;
}

/* <string> <pattern> search <post> <match> <pre> -true- */
/* <string> <pattern> search <string> -false- */
private int
zsearch(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    uint size = r_size(op);
    uint count;
    byte *pat;
    byte *ptr;
    byte ch;

    check_read_type(*op1, t_string);
    check_read_type(*op, t_string);
    if (size > r_size(op1)) {	/* can't match */
	make_false(op);
	return 0;
    }
    count = r_size(op1) - size;
    ptr = op1->value.bytes;
    if (size == 0)
	goto found;
    pat = op->value.bytes;
    ch = pat[0];
    do {
	if (*ptr == ch && (size == 1 || !memcmp(ptr, pat, size)))
	    goto found;
	ptr++;
    }
    while (count--);
    /* No match */
    make_false(op);
    return 0;
found:
    op->tas.type_attrs = op1->tas.type_attrs;
    op->value.bytes = ptr;
    r_set_size(op, size);
    push(2);
    op[-1] = *op1;
    r_set_size(op - 1, ptr - op[-1].value.bytes);
    op1->value.bytes = ptr + size;
    r_set_size(op1, count);
    make_true(op);
    return 0;
}

/* <string> <charstring> .stringbreak <int|null> */
private int
zstringbreak(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint i, j;

    check_read_type(op[-1], t_string);
    check_read_type(*op, t_string);
    /* We can't use strpbrk here, because C doesn't allow nulls in strings. */
    for (i = 0; i < r_size(op - 1); ++i)
	for (j = 0; j < r_size(op); ++j)
	    if (op[-1].value.const_bytes[i] == op->value.const_bytes[j]) {
		make_int(op - 1, i);
		goto done;
	    }
    make_null(op - 1);
 done:
    pop(1);
    return 0;
}

/* <obj> <pattern> .stringmatch <bool> */
private int
zstringmatch(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    bool result;

    check_read_type(*op, t_string);
    switch (r_type(op1)) {
	case t_string:
	    check_read(*op1);
	    goto cmp;
	case t_name:
	    name_string_ref(imemory, op1, op1);	/* can't fail */
cmp:
	    result = string_match(op1->value.const_bytes, r_size(op1),
				  op->value.const_bytes, r_size(op),
				  NULL);
	    break;
	default:
	    result = (r_size(op) == 1 && *op->value.bytes == '*');
    }
    make_bool(op1, result);
    pop(1);
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zstring_op_defs[] =
{
    {"1.bytestring", zbytestring},
    {"2anchorsearch", zanchorsearch},
    {"1.namestring", znamestring},
    {"2search", zsearch},
    {"1string", zstring},
    {"2.stringbreak", zstringbreak},
    {"2.stringmatch", zstringmatch},
    op_def_end(0)
};
