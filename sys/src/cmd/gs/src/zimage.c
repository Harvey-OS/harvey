/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zimage.c,v 1.2 2000/03/10 04:37:02 lpd Exp $ */
/* Image operators */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "estack.h"		/* for image[mask] */
#include "gsstruct.h"
#include "ialloc.h"
#include "igstate.h"
#include "ilevel.h"
#include "store.h"
#include "gscspace.h"
#include "gscssub.h"
#include "gsmatrix.h"
#include "gsimage.h"
#include "gxiparam.h"
#include "stream.h"
#include "ifilter.h"		/* for stream exception handling */
#include "iimage.h"

/* Forward references */
private int image_setup(P5(i_ctx_t *i_ctx_p, os_ptr op, gs_image_t * pim,
			   const gs_color_space * pcs, int npop));
private int image_proc_process(P1(i_ctx_t *));
private int image_file_continue(P1(i_ctx_t *));
private int image_string_continue(P1(i_ctx_t *));
private int image_cleanup(P1(i_ctx_t *));

/* <width> <height> <bits/sample> <matrix> <datasrc> image - */
int
zimage(i_ctx_t *i_ctx_p)
{
    return zimage_opaque_setup(i_ctx_p, osp, false, gs_image_alpha_none,
			       gs_current_DeviceGray_space(igs), 5);
}

/* <width> <height> <paint_1s> <matrix> <datasrc> imagemask - */
int
zimagemask(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_image_t image;

    check_type(op[-2], t_boolean);
    gs_image_t_init_mask_adjust(&image, op[-2].value.boolval,
				gs_incachedevice(igs) != CACHE_DEVICE_NONE);
    return image_setup(i_ctx_p, op, &image, NULL, 5);
}

/* Setup for [color|alpha]image.  This code isn't used for Level 1, */
/* but it's simpler to include it here. */
int
zimage_multiple(i_ctx_t *i_ctx_p, bool has_alpha)
{
    os_ptr op = osp;
    int spp;			/* samples per pixel */
    int npop = 7;
    os_ptr procp = op - 2;
    const gs_color_space *pcs;
    bool multi = false;

    check_int_leu(*op, 4);	/* ncolors */
    check_type(op[-1], t_boolean);	/* multiproc */
    switch ((spp = (int)(op->value.intval))) {
	case 1:
	    pcs = gs_current_DeviceGray_space(igs);
	    break;
	case 3:
	    pcs = gs_current_DeviceRGB_space(igs);
	    goto color;
	case 4:
	    pcs = gs_current_DeviceCMYK_space(igs);
color:
	    if (op[-1].value.boolval) {	/* planar format */
		if (has_alpha)
		    ++spp;
		npop += spp - 1;
		procp -= spp - 1;
		multi = true;
	    }
	    break;
	default:
	    return_error(e_rangecheck);
    }
    return zimage_opaque_setup(i_ctx_p, procp, multi,
		    (has_alpha ? gs_image_alpha_last : gs_image_alpha_none),
			       pcs, npop);
}

/* Common setup for [color|alpha]image. */
/* Fills in format, BitsPerComponent, Alpha. */
int
zimage_opaque_setup(i_ctx_t *i_ctx_p, os_ptr op, bool multi,
		    gs_image_alpha_t alpha, const gs_color_space * pcs,
		    int npop)
{
    gs_image_t image;

    check_int_leu(op[-2], (level2_enabled ? 12 : 8));	/* bits/sample */
    gs_image_t_init(&image, pcs);
    image.BitsPerComponent = (int)op[-2].value.intval;
    image.Alpha = alpha;
    image.format =
	(multi ? gs_image_format_component_planar : gs_image_format_chunky);
    return image_setup(i_ctx_p, op, &image, pcs, npop);
}

/* Common setup for [color|alpha]image and imagemask. */
/* Fills in Width, Height, ImageMatrix, ColorSpace. */
private int
image_setup(i_ctx_t *i_ctx_p, os_ptr op, gs_image_t * pim,
	    const gs_color_space * pcs, int npop)
{
    int code;

    check_type(op[-4], t_integer);	/* width */
    check_type(op[-3], t_integer);	/* height */
    if (op[-4].value.intval < 0 || op[-3].value.intval < 0)
	return_error(e_rangecheck);
    if ((code = read_matrix(op - 1, &pim->ImageMatrix)) < 0)
	return code;
    pim->ColorSpace = pcs;
    pim->Width = (int)op[-4].value.intval;
    pim->Height = (int)op[-3].value.intval;
    return zimage_setup(i_ctx_p, (gs_pixel_image_t *) pim, op,
			pim->ImageMask | pim->CombineWithColor, npop);
}

/* Common setup for all Level 1 and 2 images, and ImageType 4 images. */
int
zimage_setup(i_ctx_t *i_ctx_p, const gs_pixel_image_t * pim,
	     const ref * sources, bool uses_color, int npop)
{
    gx_image_enum_common_t *pie;
    int code =
	gs_image_begin_typed((const gs_image_common_t *)pim, igs,
			     uses_color, &pie);

    if (code < 0)
	return code;
    return zimage_data_setup(i_ctx_p, (const gs_pixel_image_t *)pim, pie,
			     sources, npop);
}

/* Common setup for all Level 1 and 2 images, and ImageType 3 and 4 images. */
/*
 * We push the following on the estack.
 *      control mark,
 *	num_sources,
 *      for I = num_sources-1 ... 0:
 *          data source I,
 *          aliasing information:
 *              if source is not file, 1, except that the topmost value
 *		  is used for bookkeeping in the procedure case (see below);
 *              if file is referenced by a total of M different sources and
 *                this is the occurrence with the lowest I, M;
 *              otherwise, -J, where J is the lowest I of the same file as
 *                this one;
 *      current plane index,
 *      num_sources,
 *      enumeration structure.
 */
#define NUM_PUSH(nsource) ((nsource) * 2 + 5)
/*
 * We can access these values either from the bottom (esp at control mark - 1,
 * EBOT macros) or the top (esp = enumeration structure, ETOP macros).
 * Note that all macros return pointers.
 */
#define EBOT_NUM_SOURCES(ep) ((ep) + 2)
#define EBOT_SOURCE(ep, i)\
  ((ep) + 3 + (EBOT_NUM_SOURCES(ep)->value.intval - 1 - (i)) * 2)
#define ETOP_SOURCE(ep, i)\
  ((ep) - 4 - (i) * 2)
#define ETOP_PLANE_INDEX(ep) ((ep) - 2)
#define ETOP_NUM_SOURCES(ep) ((ep) - 1)
int
zimage_data_setup(i_ctx_t *i_ctx_p, const gs_pixel_image_t * pim,
		  gx_image_enum_common_t * pie, const ref * sources, int npop)
{
    int num_sources = pie->num_planes;
    int inumpush = NUM_PUSH(num_sources);
    int code;
    gs_image_enum *penum;
    int px;
    const ref *pp;

    check_estack(inumpush + 2);	/* stuff above, + continuation + proc */
    make_int(EBOT_NUM_SOURCES(esp), num_sources);
    /*
     * Note that the data sources may be procedures, strings, or (Level
     * 2 only) files.  (The Level 1 reference manual says that Level 1
     * requires procedures, but Adobe Level 1 interpreters also accept
     * strings.)  The sources must all be of the same type.
     *
     * The Adobe documentation explicitly says that if two or more of the
     * data sources are the same or inter-dependent files, the result is not
     * defined.  We don't have a problem with the bookkeeping for
     * inter-dependent files, since each one has its own buffer, but we do
     * have to be careful if two or more sources are actually the same file.
     * That is the reason for the aliasing information described above.
     */
    for (px = 0, pp = sources; px < num_sources; px++, pp++) {
	es_ptr ep = EBOT_SOURCE(esp, px);

	make_int(ep + 1, 1);	/* default is no aliasing */
	switch (r_type(pp)) {
	    case t_file:
		if (!level2_enabled)
		    return_error(e_typecheck);
		/* Check for aliasing. */
		{
		    int pi;

		    for (pi = 0; pi < px; ++pi)
			if (sources[pi].value.pfile == pp->value.pfile) {
			    /* Record aliasing */
			    make_int(ep + 1, -pi);
			    EBOT_SOURCE(esp, pi)->value.intval++;
			    break;
			}
		}
		/* falls through */
	    case t_string:
		if (r_type(pp) != r_type(sources))
		    return_error(e_typecheck);
		check_read(*pp);
		break;
	    default:
		if (!r_is_proc(sources))
		    return_error(e_typecheck);
		check_proc(*pp);
	}
	*ep = *pp;
    }
    if ((penum = gs_image_enum_alloc(imemory, "image_setup")) == 0)
	return_error(e_VMerror);
    code = gs_image_enum_init(penum, pie, (const gs_data_image_t *)pim, igs);
    if (code != 0) {		/* error, or empty image */
	gs_image_cleanup(penum);
	ifree_object(penum, "image_setup");
	if (code >= 0)		/* empty image */
	    pop(npop);
	return code;
    }
    push_mark_estack(es_other, image_cleanup);
    esp += inumpush - 1;
    make_int(ETOP_PLANE_INDEX(esp), 0);
    make_int(ETOP_NUM_SOURCES(esp), num_sources);
    make_istruct(esp, 0, penum);
    switch (r_type(sources)) {
	case t_file:
	    push_op_estack(image_file_continue);
	    break;
	case t_string:
	    push_op_estack(image_string_continue);
	    break;
	default:		/* procedure */
	    push_op_estack(image_proc_process);
	    break;
    }
    pop(npop);
    return o_push_estack;
}
/* Pop all the control information off the e-stack. */
private es_ptr
zimage_pop_estack(es_ptr tep)
{
    return tep - NUM_PUSH(ETOP_NUM_SOURCES(tep)->value.intval);
}

/*
 * Continuation for procedure data source.  We use the topmost aliasing slot
 * to remember whether we've just called the procedure (1) or whether we're
 * returning from a RemapColor callout (0).
 */
private int
image_proc_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_image_enum *penum = r_ptr(esp, gs_image_enum);
    int px = ETOP_PLANE_INDEX(esp)->value.intval;
    int num_sources = ETOP_NUM_SOURCES(esp)->value.intval;
    uint size, used[gs_image_max_planes];
    gs_const_string plane_data[gs_image_max_planes];
    const byte *wanted;
    int i, code;

    if (!r_has_type_attrs(op, t_string, a_read)) {
	check_op(1);
	/* Procedure didn't return a (readable) string.  Quit. */
	esp = zimage_pop_estack(esp);
	image_cleanup(i_ctx_p);
	return_error(!r_has_type(op, t_string) ? e_typecheck : e_invalidaccess);
    }
    size = r_size(op);
    if (size == 0 && ETOP_SOURCE(esp, 0)[1].value.intval == 0)
	code = 1;
    else {
	for (i = 0; i < num_sources; i++)
	    plane_data[i].size = 0;
	plane_data[px].data = op->value.bytes;
	plane_data[px].size = size;
	code = gs_image_next_planes(penum, plane_data, used);
	if (code == e_RemapColor) {
	    op->value.bytes += used[px]; /* skip used data */
	    r_dec_size(op, used[px]);
	    ETOP_SOURCE(esp, 0)[1].value.intval = 0; /* RemapColor callout */
	    return code;
	}
    }
    if (code) {			/* Stop now. */
	esp = zimage_pop_estack(esp);
	pop(1);
	image_cleanup(i_ctx_p);
	return (code < 0 ? code : o_pop_estack);
    }
    pop(1);
    wanted = gs_image_planes_wanted(penum);
    do {
	if (++px == num_sources)
	    px = 0;
    } while (!wanted[px]);
    ETOP_PLANE_INDEX(esp)->value.intval = px;
    return image_proc_process(i_ctx_p);
}
private int
image_proc_process(i_ctx_t *i_ctx_p)
{
    int px = ETOP_PLANE_INDEX(esp)->value.intval;
    gs_image_enum *penum = r_ptr(esp, gs_image_enum);
    const byte *wanted = gs_image_planes_wanted(penum);
    int num_sources = ETOP_NUM_SOURCES(esp)->value.intval;
    const ref *pp;

    ETOP_SOURCE(esp, 0)[1].value.intval = 0; /* procedure callout */
    while (!wanted[px]) {
	if (++px == num_sources)
	    px = 0;
	ETOP_PLANE_INDEX(esp)->value.intval = px;
    }
    pp = ETOP_SOURCE(esp, px);
    push_op_estack(image_proc_continue);
    *++esp = *pp;
    return o_push_estack;
}

/* Continue processing data from an image with file data sources. */
private int
image_file_continue(i_ctx_t *i_ctx_p)
{
    gs_image_enum *penum = r_ptr(esp, gs_image_enum);
    int num_sources = ETOP_NUM_SOURCES(esp)->value.intval;

    for (;;) {
	uint min_avail = max_int;
	gs_const_string plane_data[gs_image_max_planes];
	int code;
	int px;
	const ref *pp;
	bool at_eof = false;

	/*
	 * Do a first pass through the files to ensure that at least
	 * one has data available in its buffer.
	 */

	for (px = 0, pp = ETOP_SOURCE(esp, 0); px < num_sources;
	     ++px, pp -= 2
	    ) {
	    int num_aliases = pp[1].value.intval;
	    stream *s = pp->value.pfile;
	    int min_left;
	    uint avail;

	    if (num_aliases <= 0)
		continue;	/* this is an alias for an earlier file */
	    while ((avail = sbufavailable(s)) <=
		   (min_left = sbuf_min_left(s)) + num_aliases - 1) {
		int next = s->end_status;

		switch (next) {
		case 0:
		    s_process_read_buf(s);
		    continue;
		case EOFC:
		    at_eof = true;
		    break;	/* with no data available */
		case INTC:
		case CALLC:
		    return
			s_handle_read_exception(i_ctx_p, next, pp,
						NULL, 0, image_file_continue);
		default:
		    /* case ERRC: */
		    return_error(e_ioerror);
		}
		break;		/* for EOFC */
	    }
	    /*
	     * Note that in the EOF case, we can get here with no data
	     * available.
	     */
	    if (avail >= min_left)
		avail = (avail - min_left) / num_aliases; /* may be 0 */
	    if (avail < min_avail)
		min_avail = avail;
	    plane_data[px].data = sbufptr(s);
	    plane_data[px].size = avail;
	}

	/*
	 * Now pass the available buffered data to the image processor.
	 * Even if there is no available data, we must call
	 * gs_image_next_planes one more time to finish processing any
	 * retained data.
	 */

	{
	    int pi;
	    uint used[gs_image_max_planes];

	    code = gs_image_next_planes(penum, plane_data, used);
	    /* Now that used has been set, update the streams. */
	    for (pi = 0, pp = ETOP_SOURCE(esp, 0); pi < num_sources;
		 ++pi, pp -= 2
		 )
		sbufskip(pp->value.pfile, used[pi]);
	    if (code == e_RemapColor)
		return code;
	}
	if (at_eof)
	    code = 1;
	if (code) {
	    esp = zimage_pop_estack(esp);
	    image_cleanup(i_ctx_p);
	    return (code < 0 ? code : o_pop_estack);
	}
    }
}

/* Process data from an image with string data sources. */
/* This may still encounter a RemapColor callback. */
private int
image_string_continue(i_ctx_t *i_ctx_p)
{
    gs_image_enum *penum = r_ptr(esp, gs_image_enum);
    int num_sources = ETOP_NUM_SOURCES(esp)->value.intval;
    gs_const_string sources[gs_image_max_planes];
    uint used[gs_image_max_planes];

    /* Pass no data initially, to find out how much is retained. */
    memset(sources, 0, sizeof(sources[0]) * num_sources);
    for (;;) {
	int px;
	int code = gs_image_next_planes(penum, sources, used);

	if (code == e_RemapColor)
	    return code;
	if (code) {		/* Stop now. */
	    esp -= NUM_PUSH(num_sources);
	    image_cleanup(i_ctx_p);
	    return (code < 0 ? code : o_pop_estack);
	}
	for (px = 0; px < num_sources; ++px)
	    if (sources[px].size == 0) {
		const ref *psrc = ETOP_SOURCE(esp, px);

		sources[px].data = psrc->value.bytes;
		sources[px].size = r_size(psrc);
	    }
    }
}

/* Clean up after enumerating an image */
private int
image_cleanup(i_ctx_t *i_ctx_p)
{
    es_ptr ep_top = esp + NUM_PUSH(EBOT_NUM_SOURCES(esp)->value.intval);
    gs_image_enum *penum = r_ptr(ep_top, gs_image_enum);

    gs_image_cleanup(penum);
    ifree_object(penum, "image_cleanup");
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zimage_op_defs[] =
{
    {"5image", zimage},
    {"5imagemask", zimagemask},
		/* Internal operators */
    {"1%image_proc_continue", image_proc_continue},
    {"0%image_file_continue", image_file_continue},
    {"0%image_string_continue", image_string_continue},
    op_def_end(0)
};
