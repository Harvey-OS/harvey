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

/* zchar.c */
/* Character operators */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gsstruct.h"
#include "gxarith.h"
#include "gxfixed.h"
#include "gxmatrix.h"			/* for ifont.h */
#include "gschar.h"
#include "gxdevice.h"			/* for gxfont.h */
#include "gxfont.h"
#include "gzpath.h"
#include "gzstate.h"
#include "dstack.h"			/* for stack depth */
#include "estack.h"
#include "ialloc.h"
#include "ichar.h"
#include "idict.h"
#include "ifont.h"
#include "igstate.h"
#include "ilevel.h"
#include "iname.h"
#include "store.h"

/* Imported operators */
int zend(P1(os_ptr));

/* Forward references */
private bool map_glyph_to_char(P3(const ref *, const ref *, ref *));
private int finish_show(P1(os_ptr));
private int finish_stringwidth(P1(os_ptr));
private int op_show_cleanup(P1(os_ptr));

/* <string> show - */
int
zshow(register os_ptr op)
{	gs_show_enum *penum;
	int code = op_show_setup(op, &penum);
	if ( code < 0 ) return code;
	if ( (code = gs_show_n_init(penum, igs, (char *)op->value.bytes, r_size(op))) < 0 )
	   {	ifree_object(penum, "op_show_enum_setup");
		return code;
	   }
	op_show_finish_setup(penum, 1, finish_show);
	return op_show_continue(op - 1);
}

/* <ax> <ay> <string> ashow - */
int
zashow(register os_ptr op)
{	gs_show_enum *penum;
	int code;
	float axy[2];
	if (	(code = num_params(op - 1, 2, axy)) < 0 ||
		(code = op_show_setup(op, &penum)) < 0
	   )
		return code;
	if ( (code = gs_ashow_n_init(penum, igs, axy[0], axy[1], (char *)op->value.bytes, r_size(op))) < 0 )
	   {	ifree_object(penum, "op_show_enum_setup");
		return code;
	   }
	op_show_finish_setup(penum, 3, finish_show);
	return op_show_continue(op - 3);
}

/* <cx> <cy> <char> <string> widthshow - */
int
zwidthshow(register os_ptr op)
{	gs_show_enum *penum;
	int code;
	float cxy[2];
	check_type(op[-1], t_integer);
	if ( (gs_char)(op[-1].value.intval) != op[-1].value.intval )
		return_error(e_rangecheck);
	if (	(code = num_params(op - 2, 2, cxy)) < 0 ||
		(code = op_show_setup(op, &penum)) < 0
	   )
		return code;
	if ( (code = gs_widthshow_n_init(penum, igs, cxy[0], cxy[1],
					 (gs_char)op[-1].value.intval,
					 (char *)op->value.bytes,
					 r_size(op))) < 0 )
	   {	ifree_object(penum, "op_show_enum_setup");
		return code;
	   }
	op_show_finish_setup(penum, 4, finish_show);
	return op_show_continue(op - 4);
}

/* <cx> <cy> <char> <ax> <ay> <string> awidthshow - */
int
zawidthshow(register os_ptr op)
{	gs_show_enum *penum;
	int code;
	float cxy[2], axy[2];
	check_type(op[-3], t_integer);
	if ( (gs_char)(op[-3].value.intval) != op[-3].value.intval )
		return_error(e_rangecheck);
	if (	(code = num_params(op - 4, 2, cxy)) < 0 ||
		(code = num_params(op - 1, 2, axy)) < 0 ||
		(code = op_show_setup(op, &penum)) < 0
	   )
		return code;
	if ( (code = gs_awidthshow_n_init(penum, igs, cxy[0], cxy[1],
					  (gs_char)op[-3].value.intval,
					  axy[0], axy[1],
					  (char *)op->value.bytes,
					  r_size(op))) < 0 )
	   {	ifree_object(penum, "op_show_enum_setup");
		return code;
	   }
	op_show_finish_setup(penum, 6, finish_show);
	return op_show_continue(op - 6);
}

/* <proc> <string> kshow - */
int
zkshow(register os_ptr op)
{	gs_show_enum *penum;
	int code;
	check_proc(op[-1]);
	if ( (code = op_show_setup(op, &penum)) < 0 ) return code;
	if ( (code = gs_kshow_n_init(penum, igs, (char *)op->value.bytes, r_size(op))) < 0 )
	   {	ifree_object(penum, "op_show_enum_setup");
		return code;
	   }
	op_show_finish_setup(penum, 2, finish_show);
	sslot = op[-1];		/* save kerning proc */
	return op_show_continue(op - 2);
}

/* Common finish procedure for all show operations. */
/* Doesn't have to do anything. */
private int
finish_show(os_ptr op)
{	return 0;
}

/* <string> stringwidth <wx> <wy> */
int
zstringwidth(register os_ptr op)
{	gs_show_enum *penum;
	int code = op_show_setup(op, &penum);
	if ( code < 0 ) return code;
	if ( (code = gs_stringwidth_n_init(penum, igs, (char *)op->value.bytes, r_size(op))) < 0 )
	   {	ifree_object(penum, "op_show_enum_setup");
		return code;
	   }
	op_show_finish_setup(penum, 1, finish_stringwidth);
	return op_show_continue(op - 1);
}
/* Finishing procedure for stringwidth. */
/* Pushes the accumulated width. */
private int
finish_stringwidth(register os_ptr op)
{	gs_point width;
	gs_show_width(senum, &width);
	push(2);
	make_real(op - 1, width.x);
	make_real(op, width.y);
	return 0;
}

/* <string> <outline_bool> charpath - */
int
zcharpath(register os_ptr op)
{	gs_show_enum *penum;
	int code;
	check_type(*op, t_boolean);
	code = op_show_setup(op - 1, &penum);
	if ( code < 0 ) return code;
	if ( (code = gs_charpath_n_init(penum, igs, (char *)op[-1].value.bytes, r_size(op - 1), op->value.boolval)) < 0 )
	   {	ifree_object(penum, "op_show_enum_setup");
		return code;
	   }
	op_show_finish_setup(penum, 2, finish_show);
	return op_show_continue(op - 2);
}

/* <wx> <wy> <llx> <lly> <urx> <ury> setcachedevice - */
int
zsetcachedevice(register os_ptr op)
{	float wbox[6];
	gs_show_enum *penum = op_show_find();
	int code = num_params(op, 6, wbox);
	if ( penum == 0 )
	  return_error(e_undefined);
	if ( code < 0 )
	  return code;
	if ( gs_show_width_only(penum) )
	  return op_show_return_width(op, 6, &wbox[0]);
	code = gs_setcachedevice(penum, igs, wbox);
	if ( code < 0 )
	  return code;
	pop(6);
	make_null(&istate->pagedevice);
	return 0;
}

/* <w0x> <w0y> <llx> <lly> <urx> <ury> <w1x> <w1y> <vx> <vy> setcachedevice2 - */
private int
zsetcachedevice2(os_ptr op)
{	float wbox[10];
	gs_show_enum *penum = op_show_find();
	int code = num_params(op, 10, wbox);
	if ( penum == 0 )
	  return_error(e_undefined);
	if ( code < 0 )
	  return code;
	if ( gs_show_width_only(penum) )
	  return op_show_return_width(op, 10,
				      (gs_rootfont(igs)->WMode ?
				       &wbox[6] : &wbox[0]));
	code = gs_setcachedevice2(penum, igs, wbox);
	if ( code < 0 )
	  return code;
	pop(10);
	make_null(&istate->pagedevice);
	return 0;
}

/* <wx> <wy> setcharwidth - */
int
zsetcharwidth(register os_ptr op)
{	float width[2];
	gs_show_enum *penum = op_show_find();
	int code = num_params(op, 2, width);
	if ( penum == 0 )
	  return_error(e_undefined);
	if ( code < 0 )
	  return code;
	if ( gs_show_width_only(penum) )
	  return op_show_return_width(op, 2, &width[0]);
	code = gs_setcharwidth(penum, igs, width[0], width[1]);
	if ( code < 0 )
	  return code;
	pop(2);
	return 0;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zchar_op_defs) {
	{"3ashow", zashow},
	{"6awidthshow", zawidthshow},
	{"2charpath", zcharpath},
	{"2kshow", zkshow},
	{"6setcachedevice", zsetcachedevice},
	{":setcachedevice2", zsetcachedevice2},
	{"2setcharwidth", zsetcharwidth},
	{"1show", zshow},
	{"1stringwidth", zstringwidth},
	{"4widthshow", zwidthshow},
		/* Internal operators */
	{"0%finish_show", finish_show},
	{"0%finish_stringwidth", finish_stringwidth},
	{"0%op_show_continue", op_show_continue},
END_OP_DEFS(0) }

/* ------ Subroutines ------ */

/* Most of these are exported for zchar2.c. */ 

/* Prepare to set up for a show operator. */
/* Don't change any state yet. */
int
op_show_setup(os_ptr op, gs_show_enum **ppenum)
{	check_read_type(*op, t_string);
	return op_show_enum_setup(ppenum);
}
int
op_show_enum_setup(gs_show_enum **ppenum)
{	check_estack(snumpush + 2);
	if ( (*ppenum = gs_show_enum_alloc((gs_memory_t *)imemory, igs, "op_show_enum_setup")) == 0 )
	  return_error(e_VMerror);
	return 0;
}

/* Finish setting up a show operator.  This can't fail, since */
/* op_show_enum_setup did the check_estack. */
void
op_show_finish_setup(gs_show_enum *penum, int npop, op_proc_p endproc /* end procedure */)
{	register es_ptr ep = esp + snumpush;
	esp = ep;
	make_mark_estack(ep - (snumpush - 1), es_show, op_show_cleanup);
	if ( endproc == NULL )
	  endproc = finish_show;
	make_null(&esslot(ep));
	make_int(&esodepth(ep), 0);	/* see gs_show_render case in */
					/* op_show_continue_dispatch */
	make_int(&esddepth(ep), 0);	/* ditto */
	make_op_estack(&eseproc(ep), endproc);
	make_istruct(ep, 0, penum);
}

/* Continuation operator for character rendering. */
int
op_show_continue(os_ptr op)
{	return op_show_continue_dispatch(op, gs_show_next(senum));
}
/* Note that this sets osp = op explicitly iff the dispatch succeeds. */
/* This is so that the show operators don't pop anything from the o-stack */
/* if they don't succeed. */
int
op_show_continue_dispatch(register os_ptr op, int code)
{	gs_show_enum *penum = senum;
	switch ( code )
	{
	case 0:				/* all done */
	{	os_ptr save_osp = osp;
		osp = op;
		code = (*real_opproc(&seproc))(op);
		op_show_free();
		if ( code < 0 )
		{	osp = save_osp;
			return code;
		}
		return o_pop_estack;
	}
	case gs_show_kern:
	{	ref *pslot = &sslot;
		push(2);
		make_int(op - 1, gs_kshow_previous_char(penum));
		make_int(op, gs_kshow_next_char(penum));
		push_op_estack(op_show_continue);		/* continue after kerning */
		*++esp = *pslot;	/* kerning procedure */
	}
		return o_push_estack;
	case gs_show_render:
	{	gs_font *pfont = gs_currentfont(igs);
		font_data *pfdata = pfont_data(pfont);
		gs_char chr = gs_show_current_char(penum);
		gs_glyph glyph = gs_show_current_glyph(penum);
		push(2);
		op[-1] = pfdata->dict;	/* push the font */
		/*
		 * For Type 1 fonts, prefer BuildChar to BuildGlyph,
		 * so that PostScript procedures appearing in the
		 * CharStrings dictionary will receive the character code
		 * rather than the character name; for Type 3 fonts,
		 * prefer BuildGlyph to BuildChar.
		 */
		if ( pfont->FontType == ft_user_defined )
		{	/* Type 3 font, prefer BuildGlyph. */
			if ( level2_enabled &&
			     !r_has_type(&pfdata->BuildGlyph, t_null)
			   )
			{	name_index_ref(glyph, op);
				esp[2] = pfdata->BuildGlyph;
			}
			else if ( r_has_type(&pfdata->BuildChar, t_null) )
				goto err;
			else if ( chr == gs_no_char )
			  {	/* glyphshow, reverse map the character */
				/* through the Encoding */
				ref gref;
				const ref *pencoding = &pfdata->Encoding;
				name_index_ref(glyph, &gref);
				if ( !map_glyph_to_char(&gref, pencoding,
							(ref *)op)
				   )
				  {	/* Not found, try .notdef */
					name_enter_string(".notdef", &gref);
					if ( !map_glyph_to_char(&gref,
								pencoding,
								(ref *)op)
					   )
					  goto err;
				  }
				esp[2] = pfdata->BuildChar;
			  }
			else
			{	make_int(op, chr);
				esp[2] = pfdata->BuildChar;
			}
		}
		else
		{	/* Type 1 font, prefer BuildChar. */
			/* We know that both BuildChar and BuildGlyph */
			/* are present. */
			if ( chr != gs_no_char )
			{	make_int(op, chr);
				esp[2] = pfdata->BuildChar;
			}
			else
			{	name_index_ref(glyph, op);
				esp[2] = pfdata->BuildGlyph;
			}
		}
		/* Save the stack depths in case we bail out. */
		sodepth.value.intval = ref_stack_count(&o_stack) - 2;
		sddepth.value.intval = ref_stack_count(&d_stack);
		push_op_estack(op_show_continue);
		++esp;		/* skip BuildChar or BuildGlyph proc */
	}
		return o_push_estack;
	default:			/* error */
err:		op_show_free();
		if ( code < 0 )
			return code;
		else
			return_error(e_invalidfont);
	}
}
/* Reverse-map a glyph name to a character code for glyphshow. */
private bool
map_glyph_to_char(const ref *pgref, const ref *pencoding, ref *pch)
{	uint esize = r_size(pencoding);
	uint ch;
	ref eref;
	for ( ch = 0; ch < esize; ch++ )
	  {	array_get(pencoding, (long)ch, &eref);
		if ( obj_eq(pgref, &eref) )
		  { make_int(pch, ch);
		    return true;
		  }
	  }
	return false;
}

/* Find the index of the e-stack mark for the current show enumerator. */
/* Return 0 if we can't find the mark. */
uint
op_show_find_index(void)
{	uint count = 0;
	STACK_LOOP_BEGIN(&e_stack, ep, size)
	  for ( ep += size - 1; size != 0; size--, ep--, count++ )
	    if ( r_is_estack_mark(ep) && estack_mark_index(ep) == es_show )
	      return count;
	STACK_LOOP_END(ep, size)
	return 0;			/* no mark */
}

/* Find the current show enumerator on the e-stack. */
gs_show_enum *
op_show_find(void)
{	uint index = op_show_find_index();
	if ( index == 0 )
	  return 0;			/* no mark */
	return r_ptr(ref_stack_index(&e_stack, index - (snumpush - 1)),
		     gs_show_enum);
}

/* Shortcut the BuildChar or BuildGlyph procedure at the point */
/* of the setcharwidth or the setcachedevice[2] if we are in */
/* a stringwidth or cshow, or if we are only collecting the scalable */
/* width for an xfont character. */
int
op_show_return_width(os_ptr op, uint npop, float *pwidth)
{	uint index = op_show_find_index();
	es_ptr ep = (es_ptr)ref_stack_index(&e_stack, index - (snumpush - 1));
	int code = gs_setcharwidth(esenum(ep), igs, pwidth[0], pwidth[1]);
	uint ocount, dsaved, dcount;

	if ( code < 0 )
	  return code;
	/* Restore the operand and dictionary stacks. */
	ocount = ref_stack_count(&o_stack) - (uint)esodepth(ep).value.intval;
	if ( ocount < npop )
	  return_error(e_stackunderflow);
	dsaved = (uint)esddepth(ep).value.intval;
	dcount = ref_stack_count(&d_stack);
	if ( dcount < dsaved )
	  return_error(e_dictstackunderflow);
	while ( dcount > dsaved )
	  {	code = zend(op);
		if ( code < 0 )
		  return code;
		dcount--;
	  }
	ref_stack_pop(&o_stack, ocount);
	/* We don't want to pop the mark or the continuation */
	/* procedure (op_show_continue or cshow_continue). */
	pop_estack(index - snumpush);
	return o_pop_estack;
}

/* Discard the show record (after an error, or at the end). */
private int
op_show_cleanup(os_ptr op)
{	register es_ptr ep = esp + snumpush;
	gs_show_enum *penum = esenum(ep);
	if ( r_is_struct(&esslot(ep)) )
	  ifree_object(esslot(ep).value.pstruct, "free_show(stream)");
	gs_show_enum_release(penum, (gs_memory_t *)imemory);
	return 0;
}
void
op_show_free(void)
{	esp -= snumpush;
	op_show_cleanup(osp);
}
