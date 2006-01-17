/* Copyright (C) 1992-2004 artofcode LLC.  All rights reserved.
  
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

/* $Id: iccfont.c,v 1.11 2004/10/26 02:50:56 giles Exp $ */
/* Initialization support for compiled fonts */

#include "string_.h"
#include "ghost.h"
#include "gsstruct.h"		/* for iscan.h */
#include "gscencs.h"
#include "gsmatrix.h"
#include "gxfont.h"		/* for ifont.h */
#include "ccfont.h"
#include "ierrors.h"
#include "ialloc.h"
#include "idict.h"
#include "ifont.h"
#include "iname.h"
#include "isave.h"		/* for ialloc_ref_array */
#include "iutil.h"
#include "oper.h"
#include "ostack.h"		/* for iscan.h */
#include "store.h"
#include "stream.h"		/* for iscan.h */
#include "strimpl.h"		/* for sfilter.h for picky compilers */
#include "sfilter.h"		/* for iscan.h */
#include "iscan.h"

/* ------ Private code ------ */

/* Forward references */
private int cfont_ref_from_string(i_ctx_t *, ref *, const char *, uint);

typedef struct {
    i_ctx_t *i_ctx_p;
    const char *str_array;
    ref next;
} str_enum;

inline private void
init_str_enum(str_enum *pse, i_ctx_t *i_ctx_p, const char *ksa)
{
    pse->i_ctx_p = i_ctx_p;
    pse->str_array = ksa;
}

typedef struct {
    cfont_dict_keys keys;
    str_enum strings;
} key_enum;

inline private void
init_key_enum(key_enum *pke, i_ctx_t *i_ctx_p, const cfont_dict_keys *pkeys,
	      const char *ksa)
{
    pke->keys = *pkeys;
    init_str_enum(&pke->strings, i_ctx_p, ksa);
}

/* Check for reaching the end of the keys. */
inline private bool
more_keys(const key_enum *pke)
{
    return (pke->keys.num_enc_keys | pke->keys.num_str_keys);
}

/* Get the next string from a string array. */
/* Return 1 if it was a string, 0 if it was something else, */
/* or an error code. */
private int
cfont_next_string(str_enum * pse)
{
    const byte *str = (const byte *)pse->str_array;
    uint len = (str[0] << 8) + str[1];

    if (len == 0xffff) {
	make_null(&pse->next);
	pse->str_array += 2;
	return 0;
    } else if (len >= 0xff00) {
	int code;

	len = ((len & 0xff) << 8) + str[2];
	code = cfont_ref_from_string(pse->i_ctx_p, &pse->next,
				     pse->str_array + 3, len);
	if (code < 0)
	    return code;
	pse->str_array += 3 + len;
	return 0;
    }
    make_const_string(&pse->next, avm_foreign, len, str + 2);
    pse->str_array += 2 + len;
    return 1;
}

/* Put the next entry into a dictionary. */
/* We know that more_keys(kp) is true. */
private int
cfont_put_next(ref * pdict, key_enum * kep, const ref * pvalue)
{
    i_ctx_t *i_ctx_p = kep->strings.i_ctx_p;
    cfont_dict_keys * const kp = &kep->keys;
    ref kname;
    int code;

    if (pdict->value.pdict == 0) {
	/* First time, create the dictionary. */
	code = dict_create(kp->num_enc_keys + kp->num_str_keys +
			   kp->extra_slots, pdict);
	if (code < 0)
	    return code;
    }
    if (kp->num_enc_keys) {
	const charindex *skp = kp->enc_keys++;
	gs_glyph glyph = gs_c_known_encode((gs_char)skp->charx, skp->encx);
	gs_const_string gstr;

	if (glyph == GS_NO_GLYPH)
	    code = gs_note_error(e_undefined);
	else if ((code = gs_c_glyph_name(glyph, &gstr)) >= 0)
	    code = name_ref(imemory, gstr.data, gstr.size, &kname, 0);
	kp->num_enc_keys--;
    } else {			/* must have kp->num_str_keys != 0 */
	code = cfont_next_string(&kep->strings);
	if (code != 1)
	    return (code < 0 ? code : gs_note_error(e_Fatal));
	code = name_ref(imemory, kep->strings.next.value.const_bytes,
			r_size(&kep->strings.next), &kname, 0);
	kp->num_str_keys--;
    }
    if (code < 0)
	return code;
    return dict_put(pdict, &kname, pvalue, &i_ctx_p->dict_stack);
}

/* ------ Routines called from compiled font initialization ------ */

/* Create a dictionary with general ref values. */
private int
cfont_ref_dict_create(i_ctx_t *i_ctx_p, ref *pdict,
		      const cfont_dict_keys *kp, cfont_string_array ksa,
		      const ref *values)
{
    key_enum kenum;
    const ref *vp = values;

    init_key_enum(&kenum, i_ctx_p, kp, ksa);
    pdict->value.pdict = 0;
    while (more_keys(&kenum)) {
	const ref *pvalue = vp++;
	int code = cfont_put_next(pdict, &kenum, pvalue);

	if (code < 0)
	    return code;
    }
    r_store_attrs(dict_access_ref(pdict), a_all, kp->dict_attrs);
    return 0;
}

/* Create a dictionary with string/null values. */
private int
cfont_string_dict_create(i_ctx_t *i_ctx_p, ref *pdict,
			 const cfont_dict_keys *kp, cfont_string_array ksa,
			 cfont_string_array kva)
{
    key_enum kenum;
    str_enum senum;
    uint attrs = kp->value_attrs;

    init_key_enum(&kenum, i_ctx_p, kp, ksa);
    init_str_enum(&senum, i_ctx_p, kva);
    pdict->value.pdict = 0;
    while (more_keys(&kenum)) {
	int code = cfont_next_string(&senum);

	switch (code) {
	    default:		/* error */
		return code;
	    case 1:		/* string */
		r_set_attrs(&senum.next, attrs);
	    case 0:		/* other */
		;
	}
	code = cfont_put_next(pdict, &kenum, &senum.next);
	if (code < 0)
	    return code;
    }
    r_store_attrs(dict_access_ref(pdict), a_all, kp->dict_attrs);
    return 0;
}

/* Create a dictionary with number values. */
private int
cfont_num_dict_create(i_ctx_t *i_ctx_p, ref * pdict,
		      const cfont_dict_keys * kp, cfont_string_array ksa,
		      const ref * values, const char *lengths)
{
    key_enum kenum;
    const ref *vp = values;
    const char *lp = lengths;
    ref vnum;

    init_key_enum(&kenum, i_ctx_p, kp, ksa);
    pdict->value.pdict = 0;
    while (more_keys(&kenum)) {
	int len = (lp == 0 ? 0 : *lp++);
	int code;

	if (len == 0)
	    vnum = *vp++;
	else {
	    --len;
	    make_const_array(&vnum, avm_foreign | a_readonly, len, vp);
	    vp += len;
	}
	code = cfont_put_next(pdict, &kenum, &vnum);
	if (code < 0)
	    return code;
    }
    r_store_attrs(dict_access_ref(pdict), a_all, kp->dict_attrs);
    return 0;
}

/* Create an array with name values. */
private int
cfont_name_array_create(i_ctx_t *i_ctx_p, ref * parray, cfont_string_array ksa,
			int size)
{
    int code = ialloc_ref_array(parray, a_readonly, size,
				"cfont_name_array_create");
    ref *aptr = parray->value.refs;
    int i;
    str_enum senum;

    if (code < 0)
	return code;
    init_str_enum(&senum, i_ctx_p, ksa);
    for (i = 0; i < size; i++, aptr++) {
	ref nref;
	int code = cfont_next_string(&senum);

	if (code != 1)
	    return (code < 0 ? code : gs_note_error(e_Fatal));
	code = name_ref(imemory, senum.next.value.const_bytes,
			r_size(&senum.next), &nref, 0);
	if (code < 0)
	    return code;
	ref_assign_new(aptr, &nref);
    }
    return 0;
}

/* Create an array with string/null values. */
private int
cfont_string_array_create(i_ctx_t *i_ctx_p, ref * parray,
			  cfont_string_array ksa, int size, uint attrs)
{
    int code = ialloc_ref_array(parray, a_readonly, size,
				"cfont_string_array_create");
    ref *aptr = parray->value.refs;
    int i;
    str_enum senum;

    if (code < 0)
	return code;
    init_str_enum(&senum, i_ctx_p, ksa);
    for (i = 0; i < size; i++, aptr++) {
	int code = cfont_next_string(&senum);

	switch (code) {
	    default:		/* error */
		return code;
	    case 1:		/* string */
		r_set_attrs(&senum.next, attrs);
	    case 0:		/* other */
		;
	}
	ref_mark_new(&senum.next);
	*aptr = senum.next;
    }
    return 0;
}

/* Create an array with scalar values. */
private int
cfont_scalar_array_create(i_ctx_t *i_ctx_p, ref * parray,
			  const ref *va, int size, uint attrs)
{
    int code = ialloc_ref_array(parray, attrs, size,
				"cfont_scalar_array_create");
    ref *aptr = parray->value.refs;
    uint elt_attrs = attrs | ialloc_new_mask;
    int i;

    if (code < 0)
	return code;
    memcpy(aptr, va, size * sizeof(ref));
    for (i = 0; i < size; i++, aptr++)
	r_set_attrs(aptr, elt_attrs);
    return 0;
}

/* Create a name. */
private int
cfont_name_create(i_ctx_t *i_ctx_p, ref * pnref, const char *str)
{
    return name_ref(imemory, (const byte *)str, strlen(str), pnref, 0);
}

/* Create an object by parsing a string. */
private int
cfont_ref_from_string(i_ctx_t *i_ctx_p, ref * pref, const char *str, uint len)
{
    scanner_state sstate;
    stream s;
    int code;

    scanner_state_init(&sstate, false);
    s_init(&s, imemory);
    sread_string(&s, (const byte *)str, len);
    code = scan_token(i_ctx_p, &s, pref, &sstate);
    return (code <= 0 ? code : gs_note_error(e_Fatal));
}

/* ------ Initialization ------ */

/* Procedure vector passed to font initialization procedures. */
private const cfont_procs ccfont_procs = {
    cfont_ref_dict_create,
    cfont_string_dict_create,
    cfont_num_dict_create,
    cfont_name_array_create,
    cfont_string_array_create,
    cfont_scalar_array_create,
    cfont_name_create,
    cfont_ref_from_string
};

/* null    .getccfont    <number-of-fonts> */
/* <int>   .getccfont    <font-object> */
private int
zgetccfont(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;
    const ccfont_fproc *fprocs;
    int nfonts;
    int index;

    code = ccfont_fprocs(&nfonts, &fprocs);
    if (code != ccfont_version)
	return_error(e_invalidfont);

    if (r_has_type(op, t_null)) {
	make_int(op, nfonts);
	return 0;
    }
    check_type(*op, t_integer);
    index = op->value.intval;
    if (index < 0 || index >= nfonts)
	return_error(e_rangecheck);

    return (*fprocs[index]) (i_ctx_p, &ccfont_procs, op);
}

/* Operator table initialization */

const op_def ccfonts_op_defs[] =
{
    {"0.getccfont", zgetccfont},
    op_def_end(0)
};
