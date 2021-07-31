/* Copyright (C) 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* zcspace2.c */
/* Level 2 color space operators */
#include "string_.h"			/* for strlen */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gscolor.h"
#include "gscspace.h"
#include "gscolor2.h"
#include "dstack.h"		/* for systemdict */
#include "estack.h"
#include "idict.h"
#include "idparam.h"
#include "igstate.h"
#include "iname.h"		/* for name_eq */
#include "store.h"

/* Imported from gscolor2.c */
extern const gs_color_space_type
	gs_color_space_type_Separation,
	gs_color_space_type_Pattern;

/* Imported from zcie.c */
extern int
	zcolorspace_CIEBasedABC(P3(const ref *, gs_color_space *, ref_cie_procs *)),
	zcolorspace_CIEBasedA(P3(const ref *, gs_color_space *, ref_cie_procs *));
/* Imported from zcsindex.c */
extern int zcolorspace_Indexed(P3(const ref *, gs_color_space *, ref *));
extern int zcolorspace_Separation(P3(const ref *, gs_color_space *,
                                     ref_separation_params *));

/* Forward references */
typedef enum { cs_allow_base, cs_allow_paint, cs_allow_all } cs_allowed;
private int cspace_param(P4(const ref *, gs_color_space *, ref_color_procs *, cs_allowed));

/* Initialization */
private void
zcspace2_init(void)
{	/* Create the names of the color spaces. */
	ref csna;
	typedef struct { const char _ds *s; int i; } csni;
	static const csni nia[] = {
		{"DeviceGray", (int)gs_color_space_index_DeviceGray},
		{"DeviceRGB", (int)gs_color_space_index_DeviceRGB},
		{"DeviceCMYK", (int)gs_color_space_index_DeviceCMYK},
		{"CIEBasedABC", (int)gs_color_space_index_CIEBasedABC},
		{"CIEBasedA", (int)gs_color_space_index_CIEBasedA},
		{"Separation", (int)gs_color_space_index_Separation},
		{"Indexed", (int)gs_color_space_index_Indexed},
		{"Pattern", (int)gs_color_space_index_Pattern}
	};
	int i;
	ialloc_ref_array(&csna, a_readonly, countof(nia), "ColorSpaceNames");
	for ( i = 0; i < countof(nia); i++ )
	{	const char _ds *str = nia[i].s;
		name_ref((const byte *)str, strlen(str), csna.value.refs + nia[i].i, 0);
	}
	dict_put_string(systemdict, "ColorSpaceNames", &csna);
}


/* - currentcolorspace <cspace> */
int
zcurrentcolorspace(register os_ptr op)
{	push(1);
	if ( r_has_type(&istate->colorspace.array, t_null) )
	{	/* Create the 1-element array on the fly. */
		const gs_color_space *pcs = gs_currentcolorspace(igs);
		ref *pcsna;
		dict_find_string(systemdict, "ColorSpaceNames", &pcsna);
		make_tasv(op, t_array, a_readonly, 1, refs,
			  pcsna->value.refs + (int)pcs->type->index);
	}
	else
		*op = istate->colorspace.array;
	return 0;
}

/* <cspace> setcolorspace - */
int
zsetcolorspace(register os_ptr op)
{	gs_color_space cs;
	ref_color_procs procs;
	ref_colorspace cspace_old;
	es_ptr ep = esp;
	int code;
	procs = istate->colorspace.procs;
	check_op(1);
	code = cspace_param((const ref *)op, &cs, &procs, cs_allow_all);
	if ( code < 0 )
	{	esp = ep;
		return code;
	}
	/* The color space installation procedure may refer to */
	/* istate->colorspace.procs. */
	cspace_old = istate->colorspace;
	if ( r_has_type(op, t_name) )
		make_null(&istate->colorspace.array);	/* no params */
	else
		istate->colorspace.array = *op;
	istate->colorspace.procs = procs;
	code = gs_setcolorspace(igs, &cs);
	if ( code < 0 )
	{	istate->colorspace = cspace_old;
		esp = ep;
		return code;
	}
	pop(1);
	return (esp == ep ? 0 : o_push_estack);
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zcspace2_l2_op_defs) {
		op_def_begin_level2(),
	{"0currentcolorspace", zcurrentcolorspace},
	{"1setcolorspace", zsetcolorspace},
END_OP_DEFS(zcspace2_init) }

/* ------ Internal procedures ------ */

/* Extract the parameters for a color space. */
private int
cspace_param(const ref *pcsref, gs_color_space *pcs,
  ref_color_procs *pcprocs, cs_allowed allow)
{	const ref *pcsa;
	uint asize;
	ref *pcsna;
	int csi;
	int code;
	if ( r_has_type(pcsref, t_array) )
	{	check_read(*pcsref);
		pcsa = pcsref->value.const_refs;
		asize = r_size(pcsref);
		if ( asize == 0 )
			return_error(e_rangecheck);
	}
	else
	{	pcsa = pcsref;
		asize = 1;
	}
	check_type_only(*pcsa, t_name);
	dict_find_string(systemdict, "ColorSpaceNames", &pcsna);
	for ( csi = 0; !name_eq(pcsa, pcsna->value.refs + csi); )
	{	if ( ++csi == r_size(pcsna) )
			return_error(e_rangecheck);
	}
	pcsa++;
	asize--;
	/* Note: to avoid having to undo allocations, we should make all */
	/* checks before any recursive calls of cspace_params. */
	/* Unfortunately, we can't do this in the case of Indexed spaces. */
	switch ( (gs_color_space_index)csi )
	{
	case gs_color_space_index_DeviceGray:
		if ( asize != 0 )
			return_error(e_rangecheck);
		pcs->type = &gs_color_space_type_DeviceGray;
		break;
	case gs_color_space_index_DeviceRGB:
		if ( asize != 0 )
			return_error(e_rangecheck);
		pcs->type = &gs_color_space_type_DeviceRGB;
		break;
	case gs_color_space_index_DeviceCMYK:
		if ( asize != 0 )
			return_error(e_rangecheck);
		pcs->type = &gs_color_space_type_DeviceCMYK;
		break;
	case gs_color_space_index_CIEBasedABC:
		if ( asize != 1 )
			return_error(e_rangecheck);
		code = zcolorspace_CIEBasedABC(pcsa, pcs, &pcprocs->cie);
		if ( code < 0 )
			return code;
		/*pcs->type = &gs_color_space_type_CIEBasedABC;*/	/* set by zcolorspace... */
		break;
	case gs_color_space_index_CIEBasedA:
		if ( asize != 1 )
			return_error(e_rangecheck);
		code = zcolorspace_CIEBasedA(pcsa, pcs, &pcprocs->cie);
		if ( code < 0 )
			return code;
		/*pcs->type = &gs_color_space_type_CIEBasedA;*/	/* set by zcolorspace... */
		break;
	case gs_color_space_index_Separation:
		if ( allow == cs_allow_base )
			return_error(e_rangecheck);
		if ( asize != 3 )
			return_error(e_rangecheck);
		switch ( r_type(pcsa) )
		{
		default:
			return_error(e_typecheck);
		case t_string:
		case t_name:
			;
		}
		check_proc(pcsa[2]);
		code = cspace_param(pcsa + 1, (gs_color_space *)&pcs->params.separation.alt_space, pcprocs, cs_allow_base);
		if ( code < 0 )
			return code;
		code = zcolorspace_Separation(pcsa, pcs, &pcprocs->special.separation);
		if ( code < 0 )
			return code;
		/*pcs->type = &gs_color_space_type_Separation;*/	/* set by zcolorspace... */
		break;
	case gs_color_space_index_Indexed:
		if ( allow == cs_allow_base )
			return_error(e_rangecheck);
		if ( asize != 3 )
			return_error(e_rangecheck);
		check_int_leu_only(pcsa[1], 4095);
		/* We must get the base space now, rather than later, */
		/* so we can check the length of the table against */
		/* the num_components of the base space. */
		code = cspace_param(pcsa, (gs_color_space *)&pcs->params.indexed.base_space, pcprocs, cs_allow_base);
		if ( code < 0 )
			return code;
		code = zcolorspace_Indexed(pcsa, pcs, &pcprocs->special.index_proc);
		if ( code < 0 )
			return code;
		/*pcs->type = &gs_color_space_type_Indexed;*/	/* set by zcolorspace... */
		break;
	case gs_color_space_index_Pattern:
		if ( allow != cs_allow_all )
			return_error(e_rangecheck);
		switch ( asize )
		{
		case 0:		/* no base space */
			pcs->params.pattern.has_base_space = false;
			break;
		default:
			return_error(e_rangecheck);
		case 1:
			pcs->params.pattern.has_base_space = true;
			code = cspace_param(pcsa,
			  (gs_color_space *)&pcs->params.pattern.base_space,
			  pcprocs, cs_allow_paint);
			if ( code < 0 )
				return code;
		}
		pcs->type = &gs_color_space_type_Pattern;
		break;
	default:
		return_error(e_typecheck);
	}
	return 0;
}
