/* Copyright (C) 1989, 1990, 1991, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zpaint.c */
/* Painting operators */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "estack.h"			/* for image[mask] */
#include "gsstruct.h"
#include "ialloc.h"
#include "igstate.h"
#include "ilevel.h"
#include "store.h"
#include "gscspace.h"
#include "gsmatrix.h"
#include "gsimage.h"
#include "gspaint.h"
#include "stream.h"
#include "ifilter.h"		/* for stream exception handling */

/* Forward references */
/* zimage_setup is used by zimage2.c */
int zimage_setup(P11(int width, int height, gs_matrix *pmat,
  ref *sources, int bits_per_component,
  bool multi, const gs_color_space *pcs, int masked,
  const float *decode, bool interpolate, int npop));
/* zimage_opaque_setup is used by zcolor1.c */
int zimage_opaque_setup(P4(os_ptr, bool, const gs_color_space_type _ds *, int));
private int image_setup(P7(os_ptr, int, bool, const gs_color_space_type _ds *, int, const float *, int));
private int image_process(P2(os_ptr, ref *));
private int image_continue(P1(os_ptr));
private int image_process_continue(P1(os_ptr));
private int image_cleanup(P1(os_ptr));

/* - fill - */
int
zfill(register os_ptr op)
{	return gs_fill(igs);
}

/* - .fillpage - */
int
zfillpage(register os_ptr op)
{	return gs_fillpage(igs);
}

/* - eofill - */
int
zeofill(register os_ptr op)
{	return gs_eofill(igs);
}

/* - stroke - */
int
zstroke(register os_ptr op)
{	return gs_stroke(igs);
}

/* Standard decoding maps for images. */
static const float decode_01[8] = { 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0 };
static const float decode_10[8] = { 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0 };

/* <width> <height> <bits/sample> <matrix> <datasrc> image - */
int
zimage(register os_ptr op)
{	return zimage_opaque_setup(op, false, &gs_color_space_type_DeviceGray, 5);
}

/* <width> <height> <paint_1s> <matrix> <datasrc> imagemask - */
int
zimagemask(register os_ptr op)
{	check_type(op[-2], t_boolean);
	return image_setup(op, 1, false, &gs_color_space_type_DeviceGray, 1,
			   (op[-2].value.boolval ? decode_10 : decode_01), 5);
}

/* Common setup for image and colorimage. */
int
zimage_opaque_setup(register os_ptr op, bool multi,
  const gs_color_space_type _ds *pcst, int npop)
{	check_int_leu(op[-2], 8);	/* bits/sample */
	return image_setup(op, (int)op[-2].value.intval, multi, pcst, 0, decode_01, npop);
}

/* Common setup for [color]image and imagemask. */
private int
image_setup(register os_ptr op, int bps, bool multi,
  const gs_color_space_type _ds *pcst, int masked, const float *decode,
  int npop)
{	gs_matrix mat;
	int code;
	gs_color_space cs;
	check_type(op[-4], t_integer);	/* width */
	check_type(op[-3], t_integer);	/* height */
	if ( op[-4].value.intval < 0 || op[-3].value.intval < 0 )
		return_error(e_rangecheck);
	if ( (code = read_matrix(op - 1, &mat)) < 0 )
		return code;
	cs.type = pcst;
	return zimage_setup((int)op[-4].value.intval,
			    (int)op[-3].value.intval,
			    &mat, op, bps, multi, &cs, masked, decode,
			    false, npop);
}

/* Common setup for Level 1 image/imagemask/colorimage and */
/* the Level 2 dictionary form of image/imagemask. */
int
zimage_setup(int width, int height, gs_matrix *pmat,
  ref *sources, int bits_per_component,
  bool multi, const gs_color_space *pcs,
  int masked, const float *decode, bool interpolate, int npop)
{	int code;
	gs_image_enum *penum;
	int px;
	ref *pp;
	int num_sources = (multi ? pcs->type->num_components : 1);
	/* We push on the estack: */
	/*	Control mark, 4 procs, last plane index, */
	/*	enumeration structure. */
#define inumpush 7
	check_estack(inumpush + 2);	/* stuff above, + continuation + proc */
	/* Note that the "procedures" might not be procedures, */
	/* but might be strings or files (Level 2 only). */
	for ( px = 0, pp = sources; px < num_sources; px++, pp++ )
	{	switch ( r_type(pp) )
		{
		case t_file:
			if ( !level2_enabled )
				return_error(e_typecheck);
			/* falls through */
		case t_string:
			check_read(*pp);
			break;
		default:
			check_proc(*pp);
		}
	}
	if ( masked != 0 && decode != 0 )
	{	/* Make sure decode is 0..1 or 1..0 */
		if ( decode[0] == 0.0 && decode[1] == 1.0 )
			masked = -1;
		else if ( decode[0] == 1.0 && decode[1] == 0.0 )
			masked = 1;
		else
			return_error(e_rangecheck);
	}
	if ( width == 0 || height == 0 )	/* empty image */
	{	pop(npop);
		return 0;
	}
	if ( (penum = gs_image_enum_alloc(imemory, "image_setup")) == 0 )
		return_error(e_VMerror);
	code = (masked != 0 ?
		gs_imagemask_init(penum, igs, width, height,
				  masked < 0, interpolate, pmat, 1) :
		gs_image_init(penum, igs, width, height, bits_per_component,
			      multi, pcs, decode, interpolate, pmat) );
	if ( code < 0 )
	{	ifree_object(penum, "image_setup");
		return code;
	}
	push_mark_estack(es_other, image_cleanup);
	++esp;
	for ( px = 0, pp = sources; px < 4; esp++, px++, pp++ )
	  if ( px < num_sources )
		*esp = *pp;
	  else
		make_null(esp);
	make_int(esp, 0);		/* current plane */
	++esp;
	make_istruct(esp, 0, penum);
	pop(npop);
	push_op_estack(image_process_continue);
	return o_push_estack;
}
/* Continuation procedure.  Hand the string to the enumerator. */
private int
image_continue(register os_ptr op)
{	ref sref;
	if ( !r_has_type_attrs(op, t_string, a_read) )
	{	check_op(1);
		/* Procedure didn't return a (readable) string.  Quit. */
		esp -= inumpush;
		image_cleanup(op);
		return_error(!r_has_type(op, t_string) ? e_typecheck : e_invalidaccess);
	}
	sref = *op;
	pop(1);
	return image_process(osp, &sref); /* osp because we did a pop */
}
/* Continue after an interrupt or a callout. */
private int
image_process_continue(os_ptr op)
{	return image_process(op, NULL);
}
/* Process data from an image data source. */
private int
image_process(os_ptr op, ref *psrc)
{	gs_image_enum *penum = r_ptr(esp, gs_image_enum);
	ref *psref = psrc;
	uint size, used;
	int code;
	int px;
	es_ptr pproc;
	if ( psref != NULL )
	  goto sw;
rd:	px = (int)(esp[-1].value.intval);
	pproc = esp - 5;
	if ( px == 4 || r_has_type(pproc + px, t_null) )
		esp[-1].value.intval = px = 0;
	psref = pproc + px;
sw:	switch ( r_type(psref) )
	{
	case t_string:
		size = r_size(psref);
		code = gs_image_next(penum, psref->value.bytes, size, &used);
		break;
	case t_file:
	{	stream *s = psref->value.pfile;
		while ( (size = sbufavailable(s)) == 0 )
		{	int next = sgetc(s);
			if ( next >= 0 )
			{	sputback(s);
				continue;
			}
			switch ( next )
			{
			case EOFC:
				break;		/* with size = 0 */
			case INTC:
			case CALLC:
				return s_handle_read_exception(next, psref,
						NULL, image_process_continue);
			default:
			/* case ERRC: */
				return_error(e_ioerror);
			}
			break;			/* for EOFC */
		}
		used = 0;		/* in case of failure */
		code = gs_image_next(penum, sbufptr(s), size, &used);
		sskip(s, used);
	}	break;
	default:			/* procedure */
		push_op_estack(image_continue);
		*++esp = *psref;
		return o_push_estack;
	}
	if ( size == 0 || code != 0 )	/* stop now */
	{	esp -= inumpush;
		image_cleanup(op);
		if ( code < 0 ) return code;
		return o_pop_estack;
	}
	++(esp[-1].value.intval);
	goto rd;
}
/* Clean up after enumerating an image */
private int
image_cleanup(os_ptr op)
{	gs_image_enum *penum = r_ptr(esp + inumpush, gs_image_enum);
	gs_image_cleanup(penum);
	ifree_object(penum, "image_cleanup");
	return 0;
}

/* ------ Non-standard operators ------ */

/* <width> <height> <data> .imagepath - */
int
zimagepath(register os_ptr op)
{	int code;
	check_type(op[-2], t_integer);
	check_type(op[-1], t_integer);
	check_read_type(*op, t_string);
	if ( r_size(op) < ((op[-2].value.intval + 7) >> 3) * op[-1].value.intval )
		return_error(e_rangecheck);
	code = gs_imagepath(igs,
		(int)op[-2].value.intval, (int)op[-1].value.intval,
		op->value.const_bytes);
	if ( code == 0 ) pop(3);
	return code;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zpaint_op_defs) {
	{"0eofill", zeofill},
	{"0fill", zfill},
	{"0.fillpage", zfillpage},
	{"5image", zimage},
	{"5imagemask", zimagemask},
	{"3.imagepath", zimagepath},
	{"0stroke", zstroke},
		/* Internal operators */
	{"1%image_continue", image_continue},
	{"0%image_process_continue", image_process_continue},
END_OP_DEFS(0) }
