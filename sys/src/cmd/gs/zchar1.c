/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zchar1.c */
/* Type 1 character display operator */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gsstruct.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gschar.h"
#include "gxdevice.h"		/* for gxfont.h */
#include "gxfont.h"
#include "gxfont1.h"
#include "gxtype1.h"
#include "estack.h"
#include "ialloc.h"
#include "ichar.h"
#include "idict.h"
#include "ifont.h"
#include "igstate.h"
#include "store.h"

/* Forward references */
private void op_type1_free(P1(os_ptr));
private int type1_call_OtherSubr(P3(gs_type1_state *, int (*)(P1(os_ptr)), const ref *));
private int type1_continue_dispatch(P3(gs_type1_state *, const ref *, ref *));

/* <string> .type1getsbw <lsbx> <lsby> <wx> <wy> */
private int type1getsbw_continue(P1(os_ptr));
private int type1getsbw_push(P2(os_ptr, gs_type1_state *));
int
ztype1getsbw(register os_ptr op)
{	gs_show_enum *penum = op_show_find();
	gs_font_type1 *pfont = (gs_font_type1 *)gs_currentfont(igs);
	gs_type1_data *pdata;
	gs_type1_state is;		/* stack allocate to avoid sandbars */
	gs_type1_state *pis = &is;
	int code;
	ref other_subr;
	if ( penum == 0 || pfont->FontType != ft_encrypted )
	  return_error(e_undefined);
	check_type(*op, t_string);
	pdata = &pfont->data;
	if ( r_size(op) <= pdata->lenIV )
	  return_error(e_invalidfont);
	check_estack(4);	/* in case we need to do a callback */
	check_ostack(3);	/* for returning the result */
	code = gs_type1_init(pis, penum, NULL,
			     gs_show_in_charpath(penum), pfont->PaintType,
			     pdata);
	if ( code < 0 )
	  return code;
	code = type1_continue_dispatch(pis, op, &other_subr);
	switch ( code )
	{
	default:		/* code < 0 or done, error */
		return((code < 0 ? code : gs_note_error(e_invalidfont)));
	case type1_result_callothersubr:	/* unknown OtherSubr */
		return type1_call_OtherSubr(pis, type1getsbw_continue,
					    &other_subr);
	case type1_result_sbw:			/* [h]sbw, done */
		break;
	}
	return type1getsbw_push(op, pis);
}

/* Push the results on the o-stack.  The caller must have done */
/* a check_ostack. */
private int
type1getsbw_push(register os_ptr op, gs_type1_state *pis)
{	push(3);
	make_real(op - 3, fixed2float(pis->lsb.x));
	make_real(op - 2, fixed2float(pis->lsb.y));
	make_real(op - 1, fixed2float(pis->width.x));
	make_real(op, fixed2float(pis->width.y));
	return 0;
}

/* Continue from an OtherSubr callback. */
private int
type1getsbw_continue(os_ptr op)
{	ref other_subr;
	gs_type1_state *pis = r_ptr(esp, gs_type1_state);
	int code;
	check_ostack(3);	/* for returning the result */
	code = type1_continue_dispatch(pis, (const ref *)0, &other_subr);
	op = osp;		/* in case z1_push/pop_proc was called */
	switch ( code )
	{
	default:		/* code < 0 or done, error */
		op_type1_free(op);
		return((code < 0 ? code : gs_note_error(e_invalidfont)));
	case type1_result_callothersubr:	/* unknown OtherSubr */
		push_op_estack(type1getsbw_continue);
		++esp;
		*esp = other_subr;
		return o_push_estack;
	case type1_result_sbw:			/* [h]sbw, done */
		type1getsbw_push(op, pis);	/* must do this before free */
		op = osp;			/* push changed osp */
		op_type1_free(op);
		return o_pop_estack;
	}
}

/* <string> .type1addpath - */
/* <string> <lsbx> <lsby> .type1addpath - */
private int type1addpath_continue(P1(os_ptr));
private int op_type1_cleanup(P1(os_ptr));
int
ztype1addpath(register os_ptr op)
{	int code = 0;
	ref other_subr;
	gs_show_enum *penum = op_show_find();
	gs_font_type1 *pfont = (gs_font_type1 *)gs_currentfont(igs);
	gs_type1_data *pdata;
	gs_type1_state is;		/* stack allocate to avoid sandbars */
	gs_type1_state *pis = &is;
	float sbxy[2];
	gs_point sbpt;
	gs_point *psbpt = 0;
	os_ptr opc = op;
	const ref *opstr;
	if ( penum == 0 || pfont->FontType != ft_encrypted )
	  return_error(e_undefined);
	if ( !r_has_type(opc, t_string) )
	  {	check_op(3);
		code = num_params(op, 2, sbxy);
		if ( code < 0 )
		  return code;
		sbpt.x = sbxy[0];
		sbpt.y = sbxy[1];
		psbpt = &sbpt;
		opc -= 2;
		check_type(*opc, t_string);
	  }
	pdata = &pfont->data;
	if ( r_size(opc) <= pdata->lenIV )
	   {	/* String is empty, or too short.  Just ignore it. */
		goto ret;
	   }
	check_estack(4);	/* in case we need to do a callback */
	code = gs_type1_init(pis, penum, psbpt,
			     gs_show_in_charpath(penum), pfont->PaintType,
			     pdata);
	if ( code < 0 )
	  return code;
	opstr = opc;
cont:	code = type1_continue_dispatch(pis, opstr, &other_subr);
	switch ( code )
	{
	case 0:			/* all done */
		break;
	default:		/* code < 0, error */
		return code;
	case type1_result_callothersubr:	/* unknown OtherSubr */
		return type1_call_OtherSubr(pis, type1addpath_continue,
					    &other_subr);
	case type1_result_sbw:			/* [h]sbw, just continue */
		opstr = 0;
		goto cont;
	}
ret:	pop((psbpt == 0 ? 1 : 3));
	return code;
}

/* Continue from an OtherSubr callback. */
private int
type1addpath_continue(os_ptr op)
{	ref other_subr;
	gs_type1_state *pis = r_ptr(esp, gs_type1_state);
	int code;
cont:	code = type1_continue_dispatch(pis, (const ref *)0, &other_subr);
	op = osp;		/* in case z1_push/pop_proc was called */
	switch ( code )
	{
	case 0:			/* all done */
	{	/* Assume the OtherSubrs didn't mess with the o-stack.... */
		int npop = (r_has_type(op, t_string) ? 1 : 3);
		pop(npop);  op -= npop;
		op_type1_free(op);
		return o_pop_estack;
	}
	default:		/* code < 0, error */
		op_type1_free(op);
		if ( code < 0 )
			return code;
		else
			return_error(e_invalidfont);
	case type1_result_callothersubr:	/* unknown OtherSubr */
		push_op_estack(type1addpath_continue);
		++esp;
		*esp = other_subr;
		return o_push_estack;
	case type1_result_sbw:			/* [h]sbw, just continue */
		goto cont;
	}
}

/* ----- Internal procedures ------ */

/* Do a callback to an OtherSubr implemented in PostScript. */
/* The caller must have done a check_estack. */
private int
type1_call_OtherSubr(gs_type1_state *pis, int (*cont)(P1(os_ptr)),
  const ref *pos)
{	/* Move the Type 1 interpreter state to the heap. */
	gs_type1_state *hpis = ialloc_struct(gs_type1_state,
					     &st_gs_type1_state,
					     ".type1addpath");
	if ( hpis == 0 )
	  return_error(e_VMerror);
	*hpis = *pis;
	push_mark_estack(es_show, op_type1_cleanup);
	++esp;
	make_istruct(esp, 0, hpis);
	push_op_estack(cont);
	++esp;
	*esp = *pos;
	return o_push_estack;
}

/* Handle the results of gs_type1_interpret. */
/* pcref points to a t_string ref. */
private int
type1_continue_dispatch(gs_type1_state *pis, const ref *pcref, ref *pos)
{	int value;
	int code;
	gs_const_string charstring;
	gs_const_string *pchars;
	if ( pcref == 0 )
	  {	pchars = 0;
	  }
	else
	  {	charstring.data = pcref->value.const_bytes;
		charstring.size = r_size(pcref);
		pchars = &charstring;
	  }
	code = gs_type1_interpret(pis, pchars, &value);
	switch ( code )
	{
	case type1_result_callothersubr:
	{	/* The Type 1 interpreter handles all known othersubrs, */
		/* so this must be an unknown one. */
		font_data *pfdata = pfont_data(gs_currentfont(igs));
		code = array_get(&pfdata->OtherSubrs, (long)value, pos);
		return (code < 0 ? code : type1_result_callothersubr);
	}
	}
	return code;
}
/* Clean up after a Type 1 callback. */
private int
op_type1_cleanup(os_ptr op)
{	ifree_object(r_ptr(esp + 2, void), "op_type1_cleanup");
	return 0;
}
private void
op_type1_free(os_ptr op)
{	esp -= 2;		/* hpis, cleanup op */
	op_type1_cleanup(op);
}

/* ------ Auxiliary procedures for type 1 fonts ------ */

int
z1_subr_proc(gs_type1_data *pdata, int index, gs_const_string *pstr)
{	/* Get the enclosing font by working backwards. */
	gs_font_type1 *pfont = (gs_font_type1 *)
		((char *)pdata - offset_of(gs_font_type1, data));
	font_data *pfdata = pfont_data(pfont);
	ref *psubr;

	if ( index < 0 || index >= r_size(&pfdata->Subrs) )
	  return_error(e_rangecheck);
	psubr = pfdata->Subrs.value.refs + index;
	check_type_only(*psubr, t_string);
	pstr->data = psubr->value.const_bytes;
	pstr->size = r_size(psubr);
	return 0;
}

int
z1_seac_proc(gs_type1_data *pdata, int index, gs_const_string *pstr)
{	/* Get the enclosing font by working backwards. */
	gs_font_type1 *pfont = (gs_font_type1 *)
		((char *)pdata - offset_of(gs_font_type1, data));
	font_data *pfdata = pfont_data(pfont);
	ref *pcstr;
	ref enc_entry;
	int code = array_get(&StandardEncoding, (long)index, &enc_entry);
	if ( code < 0 )
	  return code;
	if ( dict_find(&pfdata->CharStrings, &enc_entry, &pcstr) <= 0 )
	  return_error(e_undefined);
	check_type_only(*pcstr, t_string);
	pstr->data = pcstr->value.const_bytes;
	pstr->size = r_size(pcstr);
	return 0;
}

int
z1_push_proc(gs_type1_data *pdata, const fixed *pf, int count)
{	const fixed *p = pf + count - 1;
	int i;
	check_ostack(count);
	for ( i = 0; i < count; i++, p-- )
	  {	osp++;
		make_real(osp, fixed2float(*p));
	  }
	return 0;
}

int
z1_pop_proc(gs_type1_data *pdata, fixed *pf)
{	float val;
	int code = num_params(osp, 1, &val);
	if ( code < 0 )
		return code;
	*pf = float2fixed(val);
	osp--;
	return 0;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zchar1_op_defs) {
	{"1.type1getsbw", ztype1getsbw},
	{"1.type1addpath", ztype1addpath},
		/* Internal operators */
	{"0%type1getsbw_continue", type1getsbw_continue},
	{"0%type1addpath_continue", type1addpath_continue},
END_OP_DEFS(0) }
