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

/* zfont2.c */
/* Font creation utilities */
#include "memory_.h"
#include "string_.h"
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gxfixed.h"
#include "gsmatrix.h"
#include "gxdevice.h"
#include "gschar.h"
#include "gxfont.h"
#include "bfont.h"
#include "ialloc.h"
#include "idict.h"
#include "idparam.h"
#include "ilevel.h"
#include "iname.h"
#include "ipacked.h"
#include "istruct.h"
#include "store.h"

/* Registered encodings.  See ifont.h for documentation. */
ref registered_Encodings;
private ref *registered_Encodings_p = &registered_Encodings;
private gs_gc_root_t registered_Encodings_root;

/* Structure descriptor */
public_st_font_data();

/* Initialize the font building operators */
private void
zfont2_init(void)
{	/* Initialize the registered Encodings. */
	{	int i;
		ialloc_ref_array(&registered_Encodings, a_readonly,
				 registered_Encodings_countof,
				 "registered_Encodings");
		for ( i = 0; i < registered_Encodings_countof; i++ )
		  make_empty_array(&registered_Encoding(i), 0);
	}
	gs_register_ref_root(imemory, &registered_Encodings_root,
			     (void **)&registered_Encodings_p,
			     "registered_Encodings");
}

/* <string|name> <font_dict> .buildfont3 <string|name> <font> */
/* Build a type 3 (user-defined) font. */
int
zbuildfont3(os_ptr op)
{	int ccode, gcode, code;
	ref *pBuildChar;
	ref *pBuildGlyph;
	build_proc_refs build;
	gs_font_base *pfont;
	check_type(*op, t_dictionary);
	ccode = dict_find_string(op, "BuildChar", &pBuildChar);
	gcode = dict_find_string(op, "BuildGlyph", &pBuildGlyph);
	if ( ccode <= 0 )
	{	if ( gcode <= 0 )
			return_error(e_invalidfont);
		make_null(&build.BuildChar);
	}
	else
	{	build.BuildChar = *pBuildChar;
		check_proc(build.BuildChar);
	}
	if ( gcode <= 0 )
		make_null(&build.BuildGlyph);
	else
	{	build.BuildGlyph = *pBuildGlyph;
		check_proc(build.BuildGlyph);
	}
	code = build_gs_simple_font(op, &pfont, ft_user_defined,
				    &st_gs_font_base, &build);
	if ( code < 0 )
		return code;
	return define_gs_font((gs_font *)pfont);
}

/* <int> <array|shortarray> .registerencoding - */
private int
zregisterencoding(register os_ptr op)
{	long i;
	switch ( r_type(op) )
	  {
	  case t_array:
	  case t_shortarray:
	    break;
	  default:
	    return_op_typecheck(op);
	  }
	check_read(*op);
	check_type(op[-1], t_integer);
	for ( i = r_size(op); i > 0; )
	  {	ref cname;
		array_get(op, --i, &cname);
		check_type_only(cname, t_name);
	  }
	i = op[-1].value.intval;
	if ( i >= 0 && i < registered_Encodings_countof )
	{	ref *penc = &registered_Encoding(i);
		ref_assign_old(NULL, penc, op, ".registerencoding");
	}
	pop(2);
	return 0;
}

/* Encode a character. */
/* (This is very inefficient right now; we can speed it up later.) */
private gs_glyph
zfont_encode_char(gs_show_enum *penum, gs_font *pfont, gs_char *pchr)
{	const ref *pencoding = &pfont_data(pfont)->Encoding;
	ulong index = *pchr;	/* work around VAX widening bug */
	ref cname;
	int code = array_get(pencoding, (long)index, &cname);
	if ( code < 0 || !r_has_type(&cname, t_name) )
		return gs_no_glyph;
	return (gs_glyph)name_index(&cname);
}

/* Get the name of a glyph. */
/* The following typedef is needed to work around a bug in */
/* some AIX C compiler. */
typedef const char *const_chars;
private const_chars
zfont_glyph_name(gs_glyph index, uint *plen)
{	ref nref, sref;
	name_index_ref(index, &nref);
	name_string_ref(&nref, &sref);
	*plen = r_size(&sref);
	return (const char *)sref.value.const_bytes;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zfont2_op_defs) {
	{"2.buildfont3", zbuildfont3},
	{"2.registerencoding", zregisterencoding},
END_OP_DEFS(zfont2_init) }

/* ------ Subroutines ------ */

/* Convert strings to executable names for build_proc_refs. */
int
build_proc_name_refs(build_proc_refs *pbuild,
  const char _ds *bcstr, const char _ds *bgstr)
{	int code;
	if ( (code = name_ref((const byte *)bcstr, strlen(bcstr), &pbuild->BuildChar, 0)) < 0 ||
	     (code = name_ref((const byte *)bgstr, strlen(bgstr), &pbuild->BuildGlyph, 0)) < 0
	   )
		return code;
	r_set_attrs(&pbuild->BuildChar, a_executable);
	r_set_attrs(&pbuild->BuildGlyph, a_executable);
	return 0;
}

/* Do the common work for building a font of any non-composite FontType. */
/* The caller guarantees that *op is a dictionary. */
int
build_gs_simple_font(os_ptr op, gs_font_base **ppfont, font_type ftype,
  gs_memory_type_ptr_t pstype, const build_proc_refs *pbuild)
{	ref *pbbox;
	float bbox[4];
	gs_uid uid;
	int code;
	gs_font_base *pfont;
	/* Pre-clear the bbox in case it's invalid. */
	/* The Red Books say that FontBBox is required, */
	/* but the Adobe interpreters don't require it, */
	/* and a few user-written fonts don't supply it, */
	/* or supply one of the wrong size (!). */
	bbox[0] = bbox[1] = bbox[2] = bbox[3] = 0.0;
	if ( dict_find_string(op, "FontBBox", &pbbox) > 0 )
	{	if ( !r_is_array(pbbox) )
			return_error(e_invalidfont);
		if ( r_size(pbbox) == 4 )
		{	const ref_packed *pbe = pbbox->value.packed;
			ref rbe[4];
			int i;
			for ( i = 0; i < 4; i++ )
			{	packed_get(pbe, rbe + i);
				pbe = packed_next(pbe);
			}
			if ( num_params(rbe + 3, 4, bbox) < 0 )
				return_error(e_invalidfont);
		}
	}
	code = dict_uid_param(op, &uid, 0, imemory);
	if ( code < 0 ) return code;
	code = build_gs_font(op, (gs_font **)ppfont, ftype, pstype, pbuild);
	if ( code != 0 ) return code;	/* invalid or scaled font */
	pfont = *ppfont;
	pfont->procs.init_fstack = gs_default_init_fstack;
	pfont->procs.next_char = gs_default_next_char;
	pfont->procs.define_font = gs_no_define_font;
	pfont->procs.make_font = zdefault_make_font;
	pfont->FontBBox.p.x = bbox[0];
	pfont->FontBBox.p.y = bbox[1];
	pfont->FontBBox.q.x = bbox[2];
	pfont->FontBBox.q.y = bbox[3];
	pfont->UID = uid;
	lookup_gs_simple_font_encoding(pfont);
	return 0;
}

/* Compare the encoding of a simple font with the registered encodings. */
void
lookup_gs_simple_font_encoding(gs_font_base *pfont)
{	const ref *pfe = &pfont_data(pfont)->Encoding;
	int index;
	for ( index = registered_Encodings_countof; --index >= 0; )
	  if ( obj_eq(pfe, &registered_Encoding(index)) )
	    break;
	pfont->encoding_index = index;
	if ( index < 0 )
	{	/* Look for an encoding that's "close". */
		int near_index = -1;
		uint esize = r_size(pfe);
		uint best = esize / 3;	/* must match at least this many */
		for ( index = registered_Encodings_countof; --index >= 0; )
		{	const ref *pre = &registered_Encoding(index);
			bool r_packed = r_has_type(pre, t_shortarray);
			bool f_packed = !r_has_type(pfe, t_array);
			uint match = esize;
			int i;
			ref fchar;
			const ref *pfchar = &fchar;
			if ( r_size(pre) != esize )
			  continue;
			for ( i = esize; --i >= 0; )
			{	uint rnidx =
				  (r_packed ?
				   name_index(pre->value.refs + i) :
				   packed_name_index(pre->value.packed + i));
				if ( f_packed )
				  array_get(pfe, (long)i, &fchar);
				else
				  pfchar = pfe->value.const_refs + i;
				if ( !r_has_type(pfchar, t_name) ||
				     name_index(pfchar) != rnidx
				   )
				  if ( --match <= best )
				    break;
			}
			if ( match > best )
			  best = match,
			  near_index = index;
		}
		index = near_index;
	}
	pfont->nearest_encoding_index = index;
}

/* Do the common work for building a font of any FontType. */
/* The caller guarantees that *op is a dictionary. */
/* op[-1] must be the key under which the font is being registered */
/* in FontDirectory, normally a name or string. */
/* Return 0 for a new font, 1 for a font made by makefont or scalefont, */
/* or a negative error code. */
private void get_font_name(P2(ref *, const ref *));
private void copy_font_name(P2(gs_font_name *, const ref *));
int
build_gs_font(os_ptr op, gs_font **ppfont, font_type ftype,
  gs_memory_type_ptr_t pstype, const build_proc_refs *pbuild)
{	ref kname, fname;		/* t_string */
	ref *pftype;
	ref *pfontname;
	ref *pmatrix;
	gs_matrix mat;
	ref *pencoding;
	bool bitmapwidths;
	int exactsize, inbetweensize, transformedchar;
	int wmode;
	int code;
	gs_font *pfont;
	ref *pfid;
	ref *aop = dict_access_ref(op);
	get_font_name(&kname, op - 1);
	if ( dict_find_string(op, "FontType", &pftype) <= 0 ||
	    !r_has_type(pftype, t_integer) ||
	    pftype->value.intval != (int)ftype ||
	    dict_find_string(op, "FontMatrix", &pmatrix) <= 0 ||
	    read_matrix(pmatrix, &mat) < 0 ||
	    dict_find_string(op, "Encoding", &pencoding) <= 0 ||
	    !r_is_array(pencoding)
	   )
		return_error(e_invalidfont);
	if ( dict_find_string(op, "FontName", &pfontname) > 0 )
		get_font_name(&fname, pfontname);
	else
		make_empty_string(&fname, a_readonly);
	if ( (code = dict_int_param(op, "WMode", 0, 1, 0, &wmode)) < 0 ||
	     (code = dict_bool_param(op, "BitmapWidths", false, &bitmapwidths)) < 0 ||
	     (code = dict_int_param(op, "ExactSize", 0, 2, fbit_use_bitmaps, &exactsize)) < 0 ||
	     (code = dict_int_param(op, "InBetweenSize", 0, 2, fbit_use_outlines, &inbetweensize)) < 0 ||
	     (code = dict_int_param(op, "TransformedChar", 0, 2, fbit_use_outlines, &transformedchar)) < 0
	   )
		return code;
	code = dict_find_string(op, "FID", &pfid);
	if ( code > 0 )
	{	if ( !r_has_type(pfid, t_fontID) )
		  return_error(e_invalidfont);
		/*
		 * If this font has a FID entry already, it might be
		 * a scaled font made by makefont or scalefont,
		 * or it might just be an existing font being registered
		 * under a second name.  The latter is only allowed
		 * in Level 2 environments.
		 */
		pfont = r_ptr(pfid, gs_font);
		if ( pfont->base == pfont )	/* original font */
		{	if ( !level2_enabled )
			  return_error(e_invalidfont);
			*ppfont = pfont;
			return 0;
		}
		/* This was made by makefont or scalefont. */
		/* Just insert the new name. */
		code = 1;
		goto set_name;
	}
	/* This is a new font. */
	if ( !r_has_attr(aop, a_write) )
		return_error(e_invalidaccess);
	{	font_data *pdata;
		pfont = ialloc_struct(gs_font, pstype,
				      "buildfont(font)");
		pdata = ialloc_struct(font_data, &st_font_data,
				      "buildfont(data)");
		if ( pfont == 0 || pdata == 0 )
			code = gs_note_error(e_VMerror);
		else
			code = add_FID(op, pfont);
		if ( code < 0 )
		  {	ifree_object(pdata, "buildfont(data)");
			ifree_object(pfont, "buildfont(font)");
			return code;
		  }
		refset_null((ref *)pdata, sizeof(font_data) / sizeof(ref));
		ref_assign_new(&pdata->dict, op);
		ref_assign_new(&pdata->BuildChar, &pbuild->BuildChar);
		ref_assign_new(&pdata->BuildGlyph, &pbuild->BuildGlyph);
		ref_assign_new(&pdata->Encoding, pencoding);
		pfont->base = pfont;
		pfont->memory = imemory;
		pfont->dir = 0;
		pfont->client_data = pdata;
		pfont->FontType = ftype;
		pfont->FontMatrix = mat;
		pfont->BitmapWidths = bitmapwidths;
		pfont->ExactSize = exactsize;
		pfont->InBetweenSize = inbetweensize;
		pfont->TransformedChar = transformedchar;
		pfont->WMode = wmode;
		pfont->PaintType = 0;
		pfont->procs.build_char = gs_no_build_char;
		pfont->procs.encode_char = zfont_encode_char;
		pfont->procs.glyph_name = zfont_glyph_name;
	}
	code = 0;
set_name:
	copy_font_name(&pfont->key_name, &kname);
	copy_font_name(&pfont->font_name, &fname);
	*ppfont = pfont;
	return code;
}

/* Get the string corresponding to a font name. */
/* If the font name isn't a name or a string, return an empty string. */
private void
get_font_name(ref *pfname, const ref *op)
{	switch ( r_type(op) )
	{
	case t_string:
		*pfname = *op;
		break;
	case t_name:
		name_string_ref(op, pfname);
		break;
	default:
		/* This is weird, but legal.... */
		make_empty_string(pfname, a_readonly);
	}
}

/* Copy a font name into the gs_font structure. */
private void
copy_font_name(gs_font_name *pfstr, const ref *pfname)
{	uint size = r_size(pfname);
	if ( size > gs_font_name_max )
		size = gs_font_name_max;
	memcpy(&pfstr->chars[0], pfname->value.const_bytes, size);
	/* Following is only for debugging printout. */
	pfstr->chars[size] = 0;
	pfstr->size = size;
}

/* Finish building a font, by calling gs_definefont if needed. */
int
define_gs_font(gs_font *pfont)
{	return (pfont->base == pfont && pfont->dir == 0 ?	/* i.e., unregistered original font */
		gs_definefont(ifont_dir, pfont) :
		0);
}
