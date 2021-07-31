/* Copyright (C) 1989, 1990, 1991, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* zpath2.c */
/* Non-constructor path operators */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "errors.h"
#include "estack.h"	/* for pathforall */
#include "ialloc.h"
#include "igstate.h"
#include "gsstruct.h"
#include "gspath.h"
#include "store.h"

/* - flattenpath - */
int
zflattenpath(register os_ptr op)
{	return gs_flattenpath(igs);
}

/* - reversepath - */
int
zreversepath(register os_ptr op)
{	return gs_reversepath(igs);
}

/* - strokepath - */
int
zstrokepath(register os_ptr op)
{	return gs_strokepath(igs);
}

/* - clippath - */
int
zclippath(register os_ptr op)
{	return gs_clippath(igs);
}

/* - pathbbox <llx> <lly> <urx> <ury> */
int
zpathbbox(register os_ptr op)
{	gs_rect box;
	int code = gs_pathbbox(igs, &box);
	if ( code < 0 ) return code;
	push(4);
	make_real(op - 3, box.p.x);
	make_real(op - 2, box.p.y);
	make_real(op - 1, box.q.x);
	make_real(op, box.q.y);
	return 0;
}

/* <moveproc> <lineproc> <curveproc> <closeproc> pathforall - */
private int path_continue(P1(os_ptr));
private int path_cleanup(P1(os_ptr));
int
zpathforall(register os_ptr op)
{	gs_path_enum *penum;
	int code;
	check_proc(op[-3]);
	check_proc(op[-2]);
	check_proc(op[-1]);
	check_proc(*op);
	check_estack(8);
	if ( (penum = gs_path_enum_alloc(imemory, "pathforall")) == 0 )
		return_error(e_VMerror);
	code = gs_path_enum_init(penum, igs);
	if ( code < 0 )
	{	ifree_object(penum, "path_cleanup");
		return code;
	}
	/* Push a mark, the four procedures, and the path enumerator, */
	/* and then call the continuation procedure. */
	push_mark_estack(es_for, path_cleanup);	/* iterator */
	memcpy(esp + 1, op - 3, 4 * sizeof(ref));	/* 4 procs */
	esp += 5;
	make_istruct(esp, 0, penum);
	pop(4);  op -= 4;
	return path_continue(op);
}
/* Continuation procedure for pathforall */
private void pf_push(P3(gs_point *, int, os_ptr));
private int
path_continue(register os_ptr op)
{	gs_path_enum *penum = r_ptr(esp, gs_path_enum);
	gs_point ppts[3];
	int code;
	/* Make sure we have room on the o-stack for the worst case */
	/* before we enumerate the next path element. */
	check_ostack(6);	/* 3 points for curveto */
	code = gs_path_enum_next(penum, ppts);
	switch ( code )
	  {
	case 0:			/* all done */
	    { esp -= 6;
	      path_cleanup(op);
	      return o_pop_estack;
	    }
	default:		/* error */
	    return code;
	case gs_pe_moveto:
	    esp[2] = esp[-4];			/* moveto proc */
	    pf_push(ppts, 1, op);
	    break;
	case gs_pe_lineto:
	    esp[2] = esp[-3];			/* lineto proc */
	    pf_push(ppts, 1, op);
	    break;
	case gs_pe_curveto:
	    esp[2] = esp[-2];			/* curveto proc */
	    pf_push(ppts, 3, op);
	    break;
	case gs_pe_closepath:
	    esp[2] = esp[-1];			/* closepath proc */
	    break;
	  }
	push_op_estack(path_continue);
	++esp;		/* include pushed procedure */
	return o_push_estack;
}
/* Internal procedure to push one or more points */
private void
pf_push(gs_point *ppts, int n, os_ptr op)
{	while ( n-- )
	  { op += 2;
	    make_real(op - 1, ppts->x);
	    make_real(op, ppts->y);
	    ppts++;
	  }
	osp = op;
}
/* Clean up after a pathforall */
private int
path_cleanup(os_ptr op)
{	gs_path_enum *penum = r_ptr(esp + 6, gs_path_enum);
	gs_path_enum_cleanup(penum);
	ifree_object(penum, "path_cleanup");
	return 0;
}

/* - initclip - */
int
zinitclip(register os_ptr op)
{	return gs_initclip(igs);
}

/* - clip - */
int
zclip(register os_ptr op)
{	return gs_clip(igs);
}

/* - eoclip - */
int
zeoclip(register os_ptr op)
{	return gs_eoclip(igs);
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zpath2_op_defs) {
	{"0clip", zclip},
	{"0clippath", zclippath},
	{"0eoclip", zeoclip},
	{"0flattenpath", zflattenpath},
	{"0initclip", zinitclip},
	{"0pathbbox", zpathbbox},
	{"4pathforall", zpathforall},
	{"0reversepath", zreversepath},
	{"0strokepath", zstrokepath},
		/* Internal operators */
	{"0%path_continue", path_continue},
END_OP_DEFS(0) }
