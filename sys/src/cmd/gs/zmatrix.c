/* Copyright (C) 1989, 1991, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zmatrix.c */
/* Matrix operators */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "igstate.h"
#include "gsmatrix.h"
#include "gscoord.h"
#include "store.h"

/* Forward references */
private int near common_transform(P3(os_ptr,
  int (*)(P4(gs_state *, floatp, floatp, gs_point *)),
  int (*)(P4(floatp, floatp, const gs_matrix *, gs_point *))));

/* - initmatrix - */
int
zinitmatrix(os_ptr op)
{	return gs_initmatrix(igs);
}

/* <matrix> defaultmatrix <matrix> */
int
zdefaultmatrix(register os_ptr op)
{	gs_matrix mat;
	gs_defaultmatrix(igs, &mat);
	return write_matrix(op, &mat);
}

/* <matrix> currentmatrix <matrix> */
int
zcurrentmatrix(register os_ptr op)
{	gs_matrix mat;
	gs_currentmatrix(igs, &mat);
	return write_matrix(op, &mat);
}

/* <matrix> setmatrix - */
int
zsetmatrix(register os_ptr op)
{	gs_matrix mat;
	int code = read_matrix(op, &mat);
	if ( code < 0 )
		return code;
	if ( (code = gs_setmatrix(igs, &mat)) < 0 )
		return code;
	pop(1);
	return 0;
}

/* <tx> <ty> translate - */
/* <tx> <ty> <matrix> translate <matrix> */
int
ztranslate(register os_ptr op)
{	int code;
	float trans[2];
	if ( (code = num_params(op, 2, trans)) >= 0 )
	{	code = gs_translate(igs, trans[0], trans[1]);
		if ( code < 0 )
			return code;
	}
	else				/* matrix operand */
	{	gs_matrix mat;
		/* The num_params failure might be a stack underflow. */
		check_op(2);
		if ( (code = num_params(op - 1, 2, trans)) < 0 ||
		     (code = gs_make_translation(trans[0], trans[1], &mat)) < 0 ||
		     (code = write_matrix(op, &mat)) < 0
		   )
		  {	/* Might be a stack underflow. */
			check_op(3);
			return code;
		  }
		op[-2] = *op;
	}
	pop(2);
	return code;
}

/* <sx> <sy> scale - */
/* <sx> <sy> <matrix> scale <matrix> */
int
zscale(register os_ptr op)
{	int code;
	float scale[2];
	if ( (code = num_params(op, 2, scale)) >= 0 )
	{	code = gs_scale(igs, scale[0], scale[1]);
		if ( code < 0 )
			return code;
	}
	else				/* matrix operand */
	{	gs_matrix mat;
		/* The num_params failure might be a stack underflow. */
		check_op(2);
		if ( (code = num_params(op - 1, 2, scale)) < 0 ||
		     (code = gs_make_scaling(scale[0], scale[1], &mat)) < 0 ||
		     (code = write_matrix(op, &mat)) < 0
		   )
		  {	/* Might be a stack underflow. */
			check_op(3);
			return code;
		  }
		op[-2] = *op;
	}
	pop(2);
	return code;
}

/* <angle> rotate - */
/* <angle> <matrix> rotate <matrix> */
int
zrotate(register os_ptr op)
{	int code;
	float ang;
	if ( (code = num_params(op, 1, &ang)) >= 0 )
	{	code = gs_rotate(igs, ang);
		if ( code < 0 )
			return code;
	}
	else				/* matrix operand */
	{	gs_matrix mat;
		/* The num_params failure might be a stack underflow. */
		check_op(1);
		if ( (code = num_params(op - 1, 1, &ang)) < 0 ||
		     (code = gs_make_rotation(ang, &mat)) < 0 ||
		     (code = write_matrix(op, &mat)) < 0
		   )
		  {	/* Might be a stack underflow. */
			check_op(2);
			return code;
		  }
		op[-1] = *op;
	}
	pop(1);
	return code;
}

/* <matrix> concat - */
int
zconcat(register os_ptr op)
{	gs_matrix mat;
	int code = read_matrix(op, &mat);
	if ( code < 0 ) return code;
	code = gs_concat(igs, &mat);
	if ( code < 0 ) return code;
	pop(1);
	return 0;
}

/* <matrix1> <matrix2> <matrix> concatmatrix <matrix> */
int
zconcatmatrix(register os_ptr op)
{	gs_matrix m1, m2, mp;
	int code;
	if (	(code = read_matrix(op - 2, &m1)) < 0 ||
		(code = read_matrix(op - 1, &m2)) < 0 ||
		(code = gs_matrix_multiply(&m1, &m2, &mp)) < 0 ||
		(code = write_matrix(op, &mp)) < 0
	   ) return code;
	op[-2] = *op;
	pop(2);
	return code;
}

/* <x> <y> transform <xt> <yt> */
/* <x> <y> <matrix> transform <xt> <yt> */
int
ztransform(register os_ptr op)
{	return common_transform(op, gs_transform, gs_point_transform);
}

/* <dx> <dy> dtransform <dxt> <dyt> */
/* <dx> <dy> <matrix> dtransform <dxt> <dyt> */
int
zdtransform(register os_ptr op)
{	return common_transform(op, gs_dtransform, gs_distance_transform);
}

/* <xt> <yt> itransform <x> <y> */
/* <xt> <yt> <matrix> itransform <x> <y> */
int
zitransform(register os_ptr op)
{	return common_transform(op, gs_itransform, gs_point_transform_inverse);
}

/* <dxt> <dyt> idtransform <dx> <dy> */
/* <dxt> <dyt> <matrix> idtransform <dx> <dy> */
int
zidtransform(register os_ptr op)
{	return common_transform(op, gs_idtransform, gs_distance_transform_inverse);
}

/* Common logic for [i][d]transform */
private int near
common_transform(register os_ptr op,
  int (*ptproc)(P4(gs_state *, floatp, floatp, gs_point *)),
  int (*matproc)(P4(floatp, floatp, const gs_matrix *, gs_point *)))
{	float opxy[2];
	gs_point pt;
	int code;
	/* Optimize for the non-matrix case */
	switch ( r_type(op) )
	   {
	case t_real:
		opxy[1] = op->value.realval;
		break;
	case t_integer:
		opxy[1] = op->value.intval;
		break;
	case t_array:				/* might be a matrix */
	   {	gs_matrix mat;
		gs_matrix *pmat = &mat;
		if (	(code = read_matrix(op, pmat)) < 0 ||
			(code = num_params(op - 1, 2, opxy)) < 0 ||
			(code = (*matproc)(opxy[0], opxy[1], pmat, &pt)) < 0
		   )
		  {	/* Might be a stack underflow. */
			check_op(3);
			return code;
		  }
		op--;
		pop(1);
		goto out;
	   }
	default:
		return_op_typecheck(op);
	   }
	switch ( r_type(op - 1) )
	   {
	case t_real:
		opxy[0] = (op - 1)->value.realval;
		break;
	case t_integer:
		opxy[0] = (op - 1)->value.intval;
		break;
	default:
		return_op_typecheck(op - 1);
	   }
	if ( (code = (*ptproc)(igs, opxy[0], opxy[1], &pt)) < 0 )
		return code;
out:	make_real(op - 1, pt.x);
	make_real(op, pt.y);
	return 0;
}

/* <matrix> <inv_matrix> invertmatrix <inv_matrix> */
int
zinvertmatrix(register os_ptr op)
{	gs_matrix m;
	int code;
	if (	(code = read_matrix(op - 1, &m)) < 0 ||
		(code = gs_matrix_invert(&m, &m)) < 0 ||
		(code = write_matrix(op, &m)) < 0
	   ) return code;
	op[-1] = *op;
	pop(1);
	return code;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zmatrix_op_defs) {
	{"1concat", zconcat},
	{"2dtransform", zdtransform},
	{"3concatmatrix", zconcatmatrix},
	{"1currentmatrix", zcurrentmatrix},
	{"1defaultmatrix", zdefaultmatrix},
	{"2idtransform", zidtransform},
	{"0initmatrix", zinitmatrix},
	{"2invertmatrix", zinvertmatrix},
	{"2itransform", zitransform},
	{"1rotate", zrotate},
	{"2scale", zscale},
	{"1setmatrix", zsetmatrix},
	{"2transform", ztransform},
	{"2translate", ztranslate},
END_OP_DEFS(0) }
