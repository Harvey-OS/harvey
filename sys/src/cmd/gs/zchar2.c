/* Copyright (C) 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zchar2.c */
/* Level 2 character operators */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gschar.h"
#include "gsmatrix.h"		/* for gxfont.h */
#include "gsstruct.h"		/* for st_stream */
#include "gxfixed.h"		/* for gxfont.h */
#include "gxfont.h"
#include "estack.h"
#include "ialloc.h"
#include "ichar.h"
#include "ifont.h"
#include "igstate.h"
#include "iname.h"
#include "store.h"
#include "stream.h"
#include "ibnum.h"
#include "gspath.h"             /* gs_rmoveto prototype */

/* Table of continuation procedures. */
private int xshow_continue(P1(os_ptr));
private int yshow_continue(P1(os_ptr));
private int xyshow_continue(P1(os_ptr));
static const op_proc_p xyshow_continues[4] = {
	0, xshow_continue, yshow_continue, xyshow_continue
};

/* Forward references */
private int cshow_continue(P1(os_ptr));
private int moveshow(P2(os_ptr, int));
private int moveshow_continue(P2(os_ptr, int));

/* <proc> <string> cshow - */
private int
zcshow(os_ptr op)
{	gs_show_enum *penum;
	int code;
	check_proc(op[-1]);
	if ( (code = op_show_setup(op, &penum)) < 0 ) return code;
	if ( (code = gs_cshow_n_init(penum, igs, (char *)op->value.bytes, r_size(op))) < 0 )
	   {	ifree_object(penum, "op_show_enum_setup");
		return code;
	   }
	op_show_finish_setup(penum, 2, NULL);
	sslot = op[-1];			/* save kerning proc */
	return cshow_continue(op - 2);
}
private int
cshow_continue(os_ptr op)
{	gs_show_enum *penum = senum;
	int code = gs_show_next(penum);
	if ( code != gs_show_move )
	{	code = op_show_continue_dispatch(op, code);
		if ( code == o_push_estack )	/* must be gs_show_render */
		{	make_op_estack(esp - 1, cshow_continue);
		}
		else if ( code < 0 )
			goto errx;
		return code;
	}
	/* Push the character code and width, and call the procedure. */
	{	gs_show_enum *penum = senum;
		ref *pslot = &sslot;
		gs_point wpt;
		gs_show_current_width(penum, &wpt);
		push(3);
		make_int(op - 2, gs_show_current_char(penum));
		make_real(op - 1, wpt.x);
		make_real(op, wpt.y);
		push_op_estack(cshow_continue);
		*++esp = *pslot;	/* user procedure */
	}
	return o_push_estack;
errx:	op_show_free();
	return code;
}

/* <charname> glyphshow - */
private int
zglyphshow(os_ptr op)
{	gs_show_enum *penum;
	int code;
	check_type(*op, t_name);
	if ( (code = op_show_enum_setup(&penum)) < 0 )
		return code;
	if ( (code = gs_glyphshow_init(penum, igs, (gs_glyph)name_index(op))) < 0
	   )
	{	ifree_object(penum, "op_show_enum_setup");
		return code;
	}
	op_show_finish_setup(penum, 1, NULL);
	return op_show_continue(op - 1);
}

/* - rootfont <font> */
private int
zrootfont(os_ptr op)
{	push(1);
	*op = *pfont_dict(gs_rootfont(igs));
	return 0;
}

/* <string> <numarray|numstring> xshow - */
private int
zxshow(os_ptr op)
{	return moveshow(op, 1);
}

/* <string> <numarray|numstring> yshow - */
private int
zyshow(os_ptr op)
{	return moveshow(op, 2);
}

/* <string> <numarray|numstring> xyshow - */
private int
zxyshow(os_ptr op)
{	return moveshow(op, 3);
}

/* Common code for {x,y,xy}show */
private int
moveshow(os_ptr op, int xymask)
{	gs_show_enum *penum;
	int code = op_show_setup(op - 1, &penum);
	stream *s;
	if ( code < 0 ) return code;
	if ( (code = gs_xyshow_n_init(penum, igs, (char *)op[-1].value.bytes, r_size(op - 1)) < 0) )
	{	ifree_object(penum, "op_show_enum_setup");
		return code;
	}
	s = s_alloc(imemory, "moveshow(stream)");
	if ( s == 0 )
		return_error(e_VMerror);
	code = sread_num_array(s, op);
	if ( code < 0 )
	{	ifree_object(penum, "op_show_enum_setup");
		return code;
	}
	/* Save the number format in the position member of the stream. */
	/* This is a real hack, but it's by far the simplest approach. */
	s->position = code;
#define s_num_format(s) (int)(s->position)
	op_show_finish_setup(penum, 2, NULL);
	make_istruct(&sslot, 0, s);
	return moveshow_continue(op - 2, xymask);
}

/* Continuation procedures */

private int
xshow_continue(os_ptr op)
{	return moveshow_continue(op, 1);
}

private int
yshow_continue(os_ptr op)
{	return moveshow_continue(op, 2);
}

private int
xyshow_continue(os_ptr op)
{	return moveshow_continue(op, 3);
}

/* Get one value from the encoded number string or array. */
/* Sets pvalue->value.realval. */
private int
sget_real(stream *s, int format, ref *pvalue, int read)
{	if ( read )
	{	int code;
		switch ( code = sget_encoded_number(s, format, pvalue) )
		{
		case t_integer: pvalue->value.realval = pvalue->value.intval;
		case t_real: break;
		case t_null: code = e_rangecheck;
		default: return code;
		}
	}
	else
		pvalue->value.realval = 0;
	return 0;
}

private int
moveshow_continue(os_ptr op, int xymask)
{	stream *s = r_ptr(&sslot, stream);
	int format = s_num_format(s);
	int code;
	gs_show_enum *penum = senum;
next:	code = gs_show_next(penum);
	if ( code != gs_show_move )
	{	code = op_show_continue_dispatch(op, code);
		if ( code == o_push_estack )	/* must be gs_show_render */
		{	make_op_estack(esp - 1, xyshow_continues[xymask]);
		}
		else if ( code < 0 )
			goto errx;
		return code;
	}
	{	/* Move according to the next value(s) from the stream. */
		ref rwx, rwy;
		code = sget_real(s, format, &rwx, xymask & 1);
		if ( code < 0 ) goto errx;
		code = sget_real(s, format, &rwy, xymask & 2);
		if ( code < 0 ) goto errx;
		code = gs_rmoveto(igs, rwx.value.realval, rwy.value.realval);
		if ( code < 0 ) goto errx;
	}
	goto next;
errx:	op_show_free();
	return code;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zchar2_op_defs) {
	{"2cshow", zcshow},
	{"0rootfont", zrootfont},
		/* Internal operators */
	{"0%cshow_continue", cshow_continue},
END_OP_DEFS(0) }
BEGIN_OP_DEFS(zchar2_l2_op_defs) {
		op_def_begin_level2(),
	{"1glyphshow", zglyphshow},
	{"2xshow", zxshow},
	{"2xyshow", zxyshow},
	{"2yshow", zyshow},
		/* Internal operators */
	{"0%xshow_continue", xshow_continue},
	{"0%yshow_continue", yshow_continue},
	{"0%xyshow_continue", xyshow_continue},
END_OP_DEFS(0) }
