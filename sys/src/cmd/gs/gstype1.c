/* Copyright (C) 1990, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gstype1.c */
/* Adobe Type 1 font routines for Ghostscript library */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxarith.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxcoord.h"
#include "gzstate.h"
#include "gxdevmem.h"			/* ditto */
#include "gzpath.h"
#include "gxchar.h"
#include "gxfont.h"
#include "gxfont1.h"
#include "gxtype1.h"
#include "gxop1.h"

/* Define whether to always do Flex segments as curves. */
/* This is only an issue because some old Adobe DPS fonts */
/* seem to violate the Flex specification in a way that requires this. */
#define ALWAYS_DO_FLEX_AS_CURVE 1

/* Structure descriptor */
public_st_gs_font_type1();

/* Encrypt a string. */
int
gs_type1_encrypt(byte *dest, const byte *src, uint len, crypt_state *pstate)
{	register crypt_state state = *pstate;
	register const byte *from = src;
	register byte *to = dest;
	register uint count = len;
	while ( count )
	   {	encrypt_next(*from, state, *to);
		from++, to++, count--;
	   }
	*pstate = state;
	return 0;
}
/* Decrypt a string. */
int
gs_type1_decrypt(byte *dest, const byte *src, uint len, crypt_state *pstate)
{	register crypt_state state = *pstate;
	register const byte *from = src;
	register byte *to = dest;
	register uint count = len;
	while ( count )
	   {	/* If from == to, we can't use the obvious */
		/*	decrypt_next(*from, state, *to);	*/
		register byte ch = *from++;
		decrypt_next(ch, state, *to);
		to++, count--;
	   }
	*pstate = state;
	return 0;
}

/* Define the structure type for a Type 1 interpreter state. */
public_st_gs_type1_state();
/* GC procedures */
#define pis ((gs_type1_state *)vptr)
private ENUM_PTRS_BEGIN(gs_type1_state_enum_ptrs) {
	if ( index < pis->ips_count + 3 )
	  {	ENUM_RETURN_CONST_STRING_PTR(gs_type1_state,
					     ipstack[index - 3].char_string);
	  }
	return 0;
	}
	ENUM_PTR(0, gs_type1_state, penum);
	ENUM_PTR(1, gs_type1_state, pgs);
	ENUM_PTR(2, gs_type1_state, pdata);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(gs_type1_state_reloc_ptrs) {
	int i;
	RELOC_PTR(gs_type1_state, penum);
	RELOC_PTR(gs_type1_state, pgs);
	RELOC_PTR(gs_type1_state, pdata);
	for ( i = 0; i < pis->ips_count; i++ )
	  {	ip_state *ipsp = &pis->ipstack[i];
		int diff = ipsp->ip - ipsp->char_string.data;
		gs_reloc_const_string(&ipsp->char_string, gcst);
		ipsp->ip = ipsp->char_string.data + diff;
	  }
} RELOC_PTRS_END
#undef pis

/* Initialize a Type 1 interpreter. */
/* The caller must supply a string to the first call of gs_type1_interpret. */
int
gs_type1_init(register gs_type1_state *pis, gs_show_enum *penum,
  const gs_point *psbpt, int charpath_flag, int paint_type,
  gs_type1_data *pdata)
{	gs_state *pgs = penum->pgs;
	pis->penum = penum;
	pis->pgs = pgs;
	pis->pdata = pdata;
	pis->charpath_flag = charpath_flag;
	pis->paint_type = paint_type;
	pis->os_count = 0;
	pis->ips_count = 1;
	pis->ipstack[0].ip = 0;
	pis->ipstack[0].char_string.data = 0;
	pis->ipstack[0].char_string.size = 0;
	pis->seac_base = -1;
	pis->adxy.x = pis->adxy.y = 0;
	pis->flex_count = flex_max;		/* not in Flex */
	pis->ignore_pops = 0;
	pis->dotsection_flag = dotsection_out;
	pis->vstem3_set = false;
	pis->vs_offset.x = pis->vs_offset.y = 0;
	pis->hints_initial = 0;		/* probably not needed */
	pis->hint_next = 0;
	pis->hints_pending = 0;
	set_pixel_scale(&pis->scale.x, penum->log2_current_scale.x);
	set_pixel_scale(&pis->scale.y, penum->log2_current_scale.y);
	gx_matrix_to_fixed_coeff(&ctm_only(pgs), &pis->fc, max_coeff_bits);
	if ( charpath_flag )
	  reset_font_hints(&pis->fh, &penum->log2_current_scale);
	else
	  compute_font_hints(&pis->fh, &pgs->ctm, &penum->log2_current_scale,
			     pdata);
	/* Set the current point of the path to the origin, */
	/* in anticipation of the initial [h]sbw. */
	{ gx_path *ppath = pgs->path;
	  pis->position.x = pis->origin.x = ppath->position.x;
	  pis->position.y = pis->origin.y = ppath->position.y;
	}
	/* Set the flatness to a value that is likely to produce */
	/* reasonably good-looking curves, regardless of its */
	/* current value in the graphics state. */
	   {	/* If the character is very small, set the flatness */
		/* to zero, which will produce very accurate curves. */
		float cxx = fabs(pgs->ctm.xx), cyy = fabs(pgs->ctm.yy);
		if ( cyy > cxx )
		  cxx = cyy;
		if ( is_skewed(&pgs->ctm) )
		   {	float cxy = fabs(pgs->ctm.xy), cyx = fabs(pgs->ctm.yx);
			if ( cxy > cxx ) cxx = cxy;
			if ( cyx > cxx ) cxx = cyx;
		   }
		pis->flatness = (cxx >= 0.2 ? cxx : 0.0);
	   }

	/* Check for a side bearing override. */
	if ( psbpt == 0 )
		pis->sb_set = false;
	else
	{	pis->lsb.x = float2fixed(psbpt->x);
		pis->lsb.y = float2fixed(psbpt->y);
		pis->sb_set = true;
	}
	pis->width_set = false;

	reset_stem_hints(pis);
	return 0;
}

/* Tracing for type 1 interpreter */
#ifdef DEBUG
#  define dc(str) if ( gs_debug['1'] ) type1_trace(cip, c, str);
private void near
type1_trace(const byte *cip, byte c, const char _ds *str)
{	dprintf3("[1]0x%lx: %02x %s\n", (ulong)(cip - 1), c, (char *)str);
}
#else
#  define dc(str)
#endif

/* Define the state used by operator procedures. */
/* These macros refer to a current instance (s) of gs_op1_state. */
#define sppath s.ppath
#define sfc s.fc
#define ptx s.px
#define pty s.py

/* Accumulate relative coordinates */
/****** THESE ARE NOT ACCURATE FOR NON-INTEGER DELTAS. ******/
/* This probably doesn't make any difference in practice. */
#define c_fixed(d, c) m_fixed(d, c, sfc, max_coeff_bits)
#define accum_x(dx)\
    ptx += c_fixed(dx, xx);\
    if ( sfc.skewed ) pty += c_fixed(dx, xy)
#define accum_y(dy)\
    pty += c_fixed(dy, yy);\
    if ( sfc.skewed ) ptx += c_fixed(dy, yx)
#define accum_xy(dx,dy)\
    accum_xy_proc(&s, dx, dy)

#define s (*ps)

private void near
accum_xy_proc(register is_ptr ps, fixed dx, fixed dy)
{	ptx += m_fixed(dx, xx, sfc, max_coeff_bits),
	pty += m_fixed(dy, yy, sfc, max_coeff_bits);
	if ( sfc.skewed )
	  ptx += m_fixed(dy, yx, sfc, max_coeff_bits),
	  pty += m_fixed(dx, xy, sfc, max_coeff_bits);
}

/* If not in a charpath, we round all endpoints of lines or curves */
/* to the nearest quarter-pixel, and suppress null lines. */
/* (Rounding to the half-pixel causes too many dropouts.) */
/* This saves a lot of rendering work for small characters. */
#define pixel_quantum float2fixed(0.25)
#define pixel_rounded(fx)\
  (((fx) + (pixel_quantum >> 1)) & -pixel_quantum)
#define must_draw_to(lpx, lpy, px, py)\
  (pis->charpath_flag ? (lpx = (px), lpy = (py), 1) :\
   ((lpx = pixel_rounded(px)), (lpy = pixel_rounded(py)),\
    (psub = sppath->current_subpath) == 0 ||\
    (pseg = psub->last)->type == s_line_close ||\
    lpx != pseg->pt.x || lpy != pseg->pt.y))

/* ------ Operator procedures ------ */

/* We put these before the interpreter to save having to write */
/* prototypes for all of them. */

int
gs_op1_closepath(register is_ptr ps)
{	/* Note that this does NOT reset the current point! */
	gx_path *ppath = sppath;
	subpath *psub;
	segment *pseg;
	fixed ctemp;
	int code;
	/* Check for and suppress a microscopic closing line. */
	if ( (psub = ppath->current_subpath) != 0 &&
	     (pseg = psub->last) != 0 && pseg->type == s_line &&
	     (ctemp = pseg->pt.x - psub->pt.x,
	      any_abs(ctemp) < float2fixed(0.1)) &&
	     (ctemp = pseg->pt.y - psub->pt.y,
	      any_abs(ctemp) < float2fixed(0.1))
	   )
	  code = gx_path_pop_close_subpath(sppath);
	else
	  code = gx_path_close_subpath(sppath);
	if ( code < 0 )
	  return code;
	return gx_path_add_point(ppath, ptx, pty);	/* put the point where it was */
}

int
gs_op1_sbw(register is_ptr ps, fixed sbx, fixed sby, fixed wx, fixed wy)
{	register gs_type1_state *pis = ps->pis;
	if ( pis->sb_set )
	  sbx = pis->lsb.x, sby = pis->lsb.y;
	else
	  pis->lsb.x = sbx, pis->lsb.y = sby,
	    pis->sb_set = true;		/* needed for accented chars */
	if ( !pis->width_set )
	  pis->width.x = wx, pis->width.y = wy,
	    pis->width_set = true;
	if_debug4('1',"[1]sb=(%g,%g) w=(%g,%g)\n",
		  fixed2float(sbx), fixed2float(sby),
		  fixed2float(pis->width.x), fixed2float(pis->width.y));
	accum_xy(sbx, sby);
	return 0;
}

int
gs_op1_hsbw(register is_ptr ps, fixed sbx, fixed wx)
{	return gs_op1_sbw(ps, sbx, fixed_0, wx, fixed_0);
}

int
gs_op1_rrcurveto(register is_ptr ps, fixed dx1, fixed dy1,
  fixed dx2, fixed dy2, fixed dx3, fixed dy3)
{	gs_fixed_point pt1, pt2;
	fixed ax0 = sppath->position.x - ptx;
	fixed ay0 = sppath->position.y - pty;
	accum_xy(dx1, dy1);
	pt1.x = ptx + ax0, pt1.y = pty + ay0;
	accum_xy(dx2, dy2);
	pt2.x = ptx, pt2.y = pty;
	accum_xy(dx3, dy3);
	return gx_path_add_curve(sppath, pt1.x, pt1.y, pt2.x, pt2.y, ptx, pty);
}

#undef s

/* ------ Main interpreter ------ */

/* Continue interpreting a Type 1 CharString. */
/* If str != 0, it is taken as the byte string to interpret. */
/* Return 0 on successful completion, <0 on error, */
/* or >0 when client intervention is required. */
/* The int * argument is where the character is stored for seac, */
/* or the othersubr # for callothersubr. */
private int near type1_endchar(P3(gs_type1_state *, gs_state *, gx_path *));
int
gs_type1_interpret(register gs_type1_state *pis, const gs_const_string *str,
  int *pindex)
{	gs_state *pgs = pis->pgs;
	gs_type1_data *pdata = pis->pdata;
	gs_op1_state s;
	fixed cstack[ostack_size];
#define cs0 cstack[0]
#define ics0 fixed2int_var(cs0)
#define cs1 cstack[1]
#define ics1 fixed2int_var(cs1)
#define cs2 cstack[2]
#define ics2 fixed2int_var(cs2)
#define cs3 cstack[3]
#define ics3 fixed2int_var(cs3)
#define cs4 cstack[4]
#define ics4 fixed2int_var(cs4)
#define cs5 cstack[5]
#define ics5 fixed2int_var(cs5)
	register fixed _ss *csp;
#define clear csp = cstack - 1
	ip_state *ipsp = &pis->ipstack[pis->ips_count - 1];
	register const byte *cip;
	register crypt_state state;
	register int c;
	int code = 0;
	fixed ftx = pis->origin.x, fty = pis->origin.y;

	sppath = pgs->path;
	s.pis = pis;
	sfc = pis->fc;
	ptx = pis->position.x;
	pty = pis->position.y;

	/* Copy the operand stack out of the saved state. */
	if ( pis->os_count == 0 )
	   {	clear;
	   }
	else
	   {	memcpy(cstack, pis->ostack, pis->os_count * sizeof(fixed));
		csp = &cstack[pis->os_count - 1];
	   }

	if ( str == 0 )
	  goto cont;
	ipsp->char_string = *str;
	cip = str->data;
call:	state = crypt_charstring_seed;
	   {	int skip = pdata->lenIV;
		/* Skip initial random bytes */
		for ( ; skip > 0; --skip )
		   {	decrypt_skip_next(*cip, state); ++cip;
		   }
	   }
	goto top;
cont:	cip = ipsp->ip;
	state = ipsp->dstate;
top:	while ( 1 )
	 { uint c0;
	   c = decrypt_this((c0 = *cip++), state);
	   decrypt_skip_next(c0, state);
	   switch ( (char_command)c )
	   {
#define cnext clear; goto top
#define inext goto top
	case c_hstem: dc("hstem")
		apply_path_hints(pis, false);
		type1_hstem(pis, cs0, cs1);
		cnext;
	case c_vstem: dc("vstem")
		apply_path_hints(pis, false);
		type1_vstem(pis, cs0, cs1);
		cnext;
	case c_vmoveto: dc("vmoveto")
		cs1 = cs0;
		cs0 = 0;
		accum_y(cs1);
move:		/* cs0 = dx, cs1 = dy for hint checking. */
		if ( pis->hint_next != 0 || sppath->subpath_open > 0 )
		  apply_path_hints(pis, true);
		code = gx_path_add_point(sppath, ptx, pty);
		goto cc;
	case c_rlineto: dc("rlineto")
		accum_xy(cs0, cs1);
line:		/* cs0 = dx, cs1 = dy for hint checking. */
		code = gx_path_add_line(sppath, ptx, pty);
cc:		if ( code < 0 )
		  return code;
pp:		if_debug2('1', "[1]pt=(%g,%g)\n",
			  fixed2float(ptx), fixed2float(pty));
		cnext;
	case c_hlineto: dc("hlineto")
		accum_x(cs0);
		cs1 = 0;
		goto line;
	case c_vlineto: dc("vlineto")
		cs1 = cs0;
		cs0 = 0;
		accum_y(cs1);
		goto line;
	case c_rrcurveto: dc("rrcurveto")
		code = gs_op1_rrcurveto(&s, cs0, cs1, cs2, cs3, cs4, cs5);
		goto cc;
	case c_closepath: dc("closepath")
		code = gs_op1_closepath(&s);
		apply_path_hints(pis, true);
		goto cc;
	case c_callsubr: dc("callsubr")
	   {	int index = fixed2int_var(*csp);
		code = (*pdata->subr_proc)(pdata, index, &ipsp[1].char_string);
		if ( code < 0 )
		  return_error(code);
		--csp;
		ipsp->ip = cip, ipsp->dstate = state;
		++ipsp;
		cip = ipsp->char_string.data;
	   }
		goto call;
	case c_return: dc("return")
		--ipsp;
		goto cont;
	case c_escape: dc("escape:")
		decrypt_next(*cip, state, c); ++cip;
		switch ( (char_extended_command)c )
		   {
		case ce_dotsection: dc("  dotsection")
			pis->dotsection_flag ^=
			  (dotsection_in ^ dotsection_out);
			cnext;
		case ce_vstem3: dc("  vstem3")
			apply_path_hints(pis, false);
			if ( !pis->vstem3_set && pis->fh.use_x_hints )
			{	center_vstem(pis, pis->lsb.x + cs2, cs3);
				/* Adjust the current point */
				/* (center_vstem handles everything else). */
				ptx += pis->vs_offset.x;
				pty += pis->vs_offset.y;
				pis->vstem3_set = true;
			}
			type1_vstem(pis, cs0, cs1);
			type1_vstem(pis, cs2, cs3);
			type1_vstem(pis, cs4, cs5);
			cnext;
		case ce_hstem3: dc("  hstem3")
			apply_path_hints(pis, false);
			type1_hstem(pis, cs0, cs1);
			type1_hstem(pis, cs2, cs3);
			type1_hstem(pis, cs4, cs5);
			cnext;
		case ce_seac: dc("  seac")
		  {	gs_const_string acstr;
			/* Do the accent now.  When it finishes */
			/* (detected in endchar), do the base character. */
			pis->seac_base = ics3;
			/* Adjust the origin of the coordinate system */
			/* for the accent (endchar puts it back). */
			ptx = ftx, pty = fty;
			pis->adxy.x = cs1;
			pis->adxy.y = cs2;
			accum_xy(cs1, cs2);
			sppath->position.x = pis->position.x = ptx;
			sppath->position.y = pis->position.y = pty;
			pis->os_count = 0;	/* clear */
			/* Ask the caller to provide a new string. */
			code = (*pdata->seac_proc)(pdata, ics4, &acstr);
			if ( code != 0 )
			  {	*pindex = ics4;
				return code;
			  }
			/* Continue with the supplied string. */
			clear;
			ipsp->char_string = acstr;
			cip = acstr.data;
			goto call;
		  }
		case ce_sbw: dc("  sbw")
			code = gs_op1_sbw(&s, cs0, cs1, cs2, cs3);
			goto rsbw;
		case ce_div: dc("  div")
			csp[-1] = float2fixed((float)csp[-1] / (float)*csp);
			--csp; goto pushed;
		case ce_undoc15: dc("  undoc15")
			/* See gstype1.h for information on this opcode. */
			cnext;
		case ce_callothersubr: dc("  callothersubr")
#define fpts pis->flex_points
			/* We must remember to pop both the othersubr # */
			/* and the argument count off the stack. */
			switch ( *pindex = fixed2int_var(*csp) )
			{
			case 0:
			{	/* We have to do something really sleazy */
				/* here, namely, make it look as though */
				/* the rmovetos never really happened, */
				/* because we don't want to interrupt */
				/* the current subpath. */
				gs_fixed_point ept;
				fixed fheight = csp[-4];
				gs_fixed_point hpt;
				if ( pis->flex_count != 8 )
				  return_error(gs_error_invalidfont);
				/* Assume the next two opcodes */
				/* are `pop' `pop'.  Unfortunately, some */
				/* Monotype fonts put these in a Subr, */
				/* so we can't just look ahead in the */
				/* opcode stream. */
				pis->ignore_pops = 2;
				csp[-4] = csp[-3];
				csp[-3] = csp[-2];
				csp -= 3;
				gx_path_current_point(sppath, &ept);
				gx_path_add_point(sppath, fpts[0].x, fpts[0].y);
				sppath->subpath_open =	/* <--- sleaze */
				  pis->flex_path_was_open;
				/* Decide whether to do the flex as a curve. */
				hpt.x = fpts[1].x - fpts[4].x;
				hpt.y = fpts[1].y - fpts[4].y;
				if_debug3('1',
					  "[1]flex: d=(%g,%g), height=%g\n",
					  fixed2float(hpt.x), fixed2float(hpt.y),
					  fixed2float(fheight) / 100);
#if !ALWAYS_DO_FLEX_AS_CURVE			/* See beginning of file. */
				if ( any_abs(hpt.x) + any_abs(hpt.y) <
				     fheight / 100
				   )
				{	/* Do the flex as a line. */
					code = gx_path_add_line(sppath,
								ept.x, ept.y);
				}
				else
#endif
				{	/* Do the flex as a curve. */
					code = gx_path_add_curve(sppath,
						fpts[2].x, fpts[2].y,
						fpts[3].x, fpts[3].y,
						fpts[4].x, fpts[4].y);
					if ( code < 0 )
					  return code;
					code = gx_path_add_curve(sppath,
						fpts[5].x, fpts[5].y,
						fpts[6].x, fpts[6].y,
						fpts[7].x, fpts[7].y);
				}
			}
				if ( code < 0 )
				  return code;
				pis->flex_count = flex_max;	/* not inside flex */
				inext;
			case 1:
				gx_path_current_point(sppath, &fpts[0]);
				pis->flex_path_was_open = /* <--- more sleaze */
				  sppath->subpath_open;
				pis->flex_count = 1;
				csp -= 2;
				inext;
			case 2:
				if ( pis->flex_count >= flex_max )
				  return_error(gs_error_invalidfont);
				gx_path_current_point(sppath,
						&fpts[pis->flex_count++]);
				csp -= 2;
				inext;
			case 3:
				/* Assume the next opcode is a `pop'. */
				/* See above as to why we don't just */
				/* look ahead in the opcode stream. */
				pis->ignore_pops = 1;
				reset_stem_hints(pis);
				csp -= 2;
				inext;
			}
#undef fpts
			/* Not a recognized othersubr, */
			/* let the client handle it. */
		{	int scount = csp - cstack;
			int n;
			/* Copy the arguments to the caller's stack. */
			if ( scount < 1 || csp[-1] < 0 ||
			     csp[-1] > int2fixed(scount - 1)
			   )
			  return_error(gs_error_invalidfont);
			n = fixed2int_var(csp[-1]);
			code = (*pdata->push_proc)(pdata, csp - (n + 1), n);
			if ( code < 0 )
			  return_error(code);
			scount -= n + 1;
			pis->position.x = ptx;
			pis->position.y = pty;
			apply_path_hints(pis, false);
			/* Exit to caller */
			ipsp->ip = cip, ipsp->dstate = state;
			pis->os_count = scount;
			pis->ips_count = ipsp - &pis->ipstack[0] + 1;
			if ( scount )
			  memcpy(pis->ostack, cstack, scount * sizeof(fixed));
			return type1_result_callothersubr;
		}
		case ce_pop: dc("  pop")
			/* Check whether we're ignoring the pops after */
			/* a known othersubr. */
			if ( pis->ignore_pops != 0 )
			  {	pis->ignore_pops--;
				inext;
			  }
			++csp;
			code = (*pdata->pop_proc)(pdata, csp);
			if ( code < 0 )
			  return_error(code);
			goto pushed;
		case ce_setcurrentpoint: dc("  setcurrentpoint")
			ptx = ftx, pty = fty;
			cs0 += pis->adxy.x;
			cs1 += pis->adxy.y;
			accum_xy(cs0, cs1);
			goto pp;
		default:
			return_error(gs_error_invalidfont);
		   }
		break;
	case c_hsbw: dc("hsbw")
		code = gs_op1_hsbw(&s, cs0, cs1);
rsbw:		/* Give the caller the opportunity to intervene. */
		if ( code < 0 )
		  return code;
		pis->position.x = ptx;
		pis->position.y = pty;
		pis->os_count = 0;	/* clear */
		ipsp->ip = cip, ipsp->dstate = state;
		pis->ips_count = ipsp - &pis->ipstack[0] + 1;
		return type1_result_sbw;
	case c_endchar: dc("endchar")
		if ( pis->seac_base >= 0 )
		   {	/* We just finished the accent of a seac. */
			/* Do the base character. */
			gs_const_string bstr;
			int bchar = pis->seac_base;
			pis->seac_base = -1;
			/* Restore the coordinate system origin */
			pis->adxy.x = pis->adxy.y = 0;
			sppath->position.x = pis->position.x = ftx;
			sppath->position.y = pis->position.y = fty;
			pis->os_count = 0;	/* clear */
			/* Clear the ipstack, in case the accent ended */
			/* inside a subroutine. */
			pis->ips_count = 1;
			/* Remove any accent hints. */
			reset_stem_hints(pis);
			/* Ask the caller to provide a new string. */
			code = (*pdata->seac_proc)(pdata, bchar, &bstr);
			if ( code != 0 )
			  {	*pindex = bchar;
				return code;
			  }
			/* Continue with the supplied string. */
			clear;
			ptx = ftx, pty = fty;
			ipsp = &pis->ipstack[0];
			ipsp->char_string = bstr;
			cip = bstr.data;
			goto call;
		  }
		/* This is a real endchar.  Handle it below. */
		return type1_endchar(pis, pgs, sppath);
	case c_undoc15: dc("  undoc15")
		/* See gstype1.h for information on this opcode. */
		cnext;
	case c_rmoveto: dc("rmoveto")
		accum_xy(cs0, cs1);
		goto move;
	case c_hmoveto: dc("hmoveto")
		accum_x(cs0);
		cs1 = 0;
		goto move;
	case c_vhcurveto: dc("vhcurveto")
		code = gs_op1_rrcurveto(&s, fixed_0, cs0, cs1, cs2, cs3, fixed_0);
		goto cc;
	case c_hvcurveto: dc("hvcurveto")
		code = gs_op1_rrcurveto(&s, cs0, fixed_0, cs1, cs2, fixed_0, cs3);
		goto cc;

	/* Fill up the dispatch up to 32. */

	case c_undef0: case c_undef2:
	case c_undef16: case c_undef17: case c_undef18: case c_undef19:
	case c_undef20: case c_undef23:
	case c_undef24: case c_undef25: case c_undef26: case c_undef27:
	case c_undef28: case c_undef29:
		return_error(gs_error_invalidfont);

	/* Fill up the dispatch for 1-byte numbers. */

#define icase(n) case n:
#define ncase(n) case n: *++csp = int2fixed(c_value_num1(n)); goto pushed;
#define icase10(n)\
  icase(n) icase(n+1) icase(n+2) icase(n+3) icase(n+4)\
  icase(n+5) icase(n+6) icase(n+7) icase(n+8) icase(n+9)
#define ncase10(n)\
  ncase(n) ncase(n+1) ncase(n+2) ncase(n+3) ncase(n+4)\
  ncase(n+5) ncase(n+6) ncase(n+7) ncase(n+8) ncase(n+9)
	icase(32) icase(33) icase(34)
	icase(35) icase(36) icase(37) icase(38) icase(39)
	icase10(40)
	icase10(50) icase10(60) icase10(70) icase10(80) icase10(90)
	icase10(100) icase10(110) goto pi; ncase10(120) ncase10(130) ncase10(140)
	ncase10(150) icase10(160) icase10(170) icase10(180) icase10(190)
	icase10(200) icase10(210) icase10(220) icase10(230)
	icase(240) icase(241) icase(242) icase(243) icase(244)
	icase(245) icase(246)
pi:		*++csp = int2fixed(c_value_num1(c));
pushed:		if_debug3('1', "[1]%d: (%d) %f\n",
			  (int)(csp - cstack), c, fixed2float(*csp));
		break;

	/* Handle 2-byte positive numbers. */

	case c_pos2_0: c = c_value_pos2(c_pos2_0, 0);
pos2:	   {	c0 = *cip++;
		if_debug2('1', "[1] (%d)+%d\n",
			  c, decrypt_this(c0, state));
		*++csp = int2fixed((int)decrypt_this(c0, state) + c);
		decrypt_skip_next(c0, state);
	   }	goto pushed;
	case c_pos2_1: c = c_value_pos2(c_pos2_1, 0); goto pos2;
	case c_pos2_2: c = c_value_pos2(c_pos2_2, 0); goto pos2;
	case c_pos2_3: c = c_value_pos2(c_pos2_3, 0); goto pos2;

	/* Handle 2-byte negative numbers. */

	case c_neg2_0: c = c_value_neg2(c_neg2_0, 0);
neg2:	   {	c0 = *cip++;
		if_debug2('1', "[1] (%d)-%d\n",
			  c, decrypt_this(c0, state));
		*++csp = int2fixed(c - (int)decrypt_this(c0, state));
		decrypt_skip_next(c0, state);
	   }	goto pushed;
	case c_neg2_1: c = c_value_neg2(c_neg2_1, 0); goto neg2;
	case c_neg2_2: c = c_value_neg2(c_neg2_2, 0); goto neg2;
	case c_neg2_3: c = c_value_neg2(c_neg2_3, 0); goto neg2;

	/* Handle 5-byte numbers. */

	case c_num4:
	   {	long lw = 0;
		int i;
		for ( i = 4; --i >= 0; )
		{	decrypt_next(*cip, state, c0);
			lw = (lw << 8) + c0;
			cip++;
		}
		*++csp = int2fixed(lw);
		if ( lw != fixed2long(*csp) )
		  return_error(gs_error_rangecheck);
	   }	goto pushed;
	   }
	 }
}

/* ------ Termination ------ */

/* Handle the end of a character. */
private int near
type1_endchar(gs_type1_state *pis, gs_state *pgs, gx_path *ppath)
{	if ( pis->hint_next != 0 || ppath->subpath_open > 0 )
	  apply_path_hints(pis, true);
	/* Set the current point to the character origin */
	/* plus the width. */
	gs_moveto(pgs, fixed2float(pis->width.x), fixed2float(pis->width.y));
	if ( pis->scale.x.log2_unit + pis->scale.y.log2_unit == 0 )
	{	/*
		 * Tweak up the fill adjustment.  This is a hack for when
		 * we can't oversample.  The values here are based entirely
		 * on experience, not theory, and are designed primarily
		 * for displays and low-resolution fax.
		 */
		gs_fixed_rect bbox;
		int dx, dy, dmax;
		gx_path_bbox(pgs->path, &bbox);
		dx = fixed2int_ceiling(bbox.q.x - bbox.p.x);
		dy = fixed2int_ceiling(bbox.q.y - bbox.p.y);
		dmax = max(dx, dy);
		if ( pis->fh.snap_h.count || pis->fh.snap_v.count ||
		     pis->fh.a_zone_count
		   )
		{	/* We have hints.  Only tweak up a little at */
			/* very small sizes, to help nearly-vertical */
			/* or nearly-horizontal diagonals. */
			pgs->fill_adjust =
				(dmax < 15 ? float2fixed(0.15) :
				 dmax < 25 ? float2fixed(0.1) :
				 fixed_0);
		}
		else
		{	/* No hints.  Tweak a little more to compensate */
			/* for lack of snapping to pixel grid. */
			pgs->fill_adjust =
				(dmax < 10 ? float2fixed(0.2) :
				 dmax < 25 ? float2fixed(0.1) :
				 float2fixed(0.05));
		}
	}
	else
	{	/* Don't do any adjusting. */
		pgs->fill_adjust = fixed_0;
	}
	/* Set the flatness for curve rendering. */
	if ( !pis->charpath_flag )
	  gs_setflat(pgs, pis->flatness);
	return 0;
}
