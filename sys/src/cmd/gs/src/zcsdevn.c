/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zcsdevn.c,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* DeviceN color space support */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gxcspace.h"		/* must precede gscolor2.h */
#include "gscolor2.h"
#include "gxcdevn.h"
#include "estack.h"
#include "ialloc.h"
#include "icremap.h"
#include "igstate.h"
#include "iname.h"
#include "ostack.h"
#include "store.h"

/* Imported from gscdevn.c */
extern const gs_color_space_type gs_color_space_type_DeviceN;

/* Forward references */
private int ztransform_DeviceN(P5(const gs_device_n_params * params,
				  const float *in, float *out,
				  const gs_imager_state *pis, void *data));

/* <array> .setdevicenspace - */
/* The current color space is the alternate space for the DeviceN space. */
private int
zsetdevicenspace(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const ref *pcsa;
    gs_separation_name *names;
    gs_device_n_map *pmap;
    uint num_components;
    gs_color_space cs;
    ref_colorspace cspace_old;
    int code;

    check_read_type(*op, t_array);
    if (r_size(op) != 4)
	return_error(e_rangecheck);
    pcsa = op->value.const_refs + 1;
    if (!r_is_array(pcsa))
	return_error(e_typecheck);
    num_components = r_size(pcsa);
    if (num_components == 0)
	return_error(e_rangecheck);
    if (num_components > GS_CLIENT_COLOR_MAX_COMPONENTS)
	return_error(e_limitcheck);
    check_proc(pcsa[2]);
    cs = *gs_currentcolorspace(igs);
    if (!cs.type->can_be_alt_space)
	return_error(e_rangecheck);
    code = alloc_device_n_map(&pmap, imemory, ".setdevicenspace(map)");
    if (code < 0)
	return code;
    names = (gs_separation_name *)
	ialloc_byte_array(num_components, sizeof(gs_separation_name),
			  ".setdevicenspace(names)");
    if (names == 0) {
	ifree_object(pmap, ".setdevicenspace(map)");
	return_error(e_VMerror);
    }
    {
	uint i;
	ref sname;

	for (i = 0; i < num_components; ++i) {
	    array_get(pcsa, (long)i, &sname);
	    switch (r_type(&sname)) {
		case t_string:
		    code = name_from_string(&sname, &sname);
		    if (code < 0) {
			ifree_object(names, ".setdevicenspace(names)");
			ifree_object(pmap, ".setdevicenspace(map)");
			return code;
		    }
		    /* falls through */
		case t_name:
		    names[i] = name_index(&sname);
		    break;
		default:
		    ifree_object(names, ".setdevicenspace(names)");
		    ifree_object(pmap, ".setdevicenspace(map)");
		    return_error(e_typecheck);
	    }
	}
    }
    /* See zcsindex.c for why we use memmove here. */
    memmove(&cs.params.device_n.alt_space, &cs,
	    sizeof(cs.params.device_n.alt_space));
    gs_cspace_init(&cs, &gs_color_space_type_DeviceN, NULL);
    cspace_old = istate->colorspace;
    istate->colorspace.procs.special.device_n.layer_names = pcsa[0];
    istate->colorspace.procs.special.device_n.tint_transform = pcsa[2];
    cs.params.device_n.names = names;
    cs.params.device_n.num_components = num_components;
    pmap->tint_transform = ztransform_DeviceN;
    cs.params.device_n.map = pmap;
    code = gs_setcolorspace(igs, &cs);
    if (code < 0) {
	istate->colorspace = cspace_old;
	ifree_object(names, ".setdevicenspace(names)");
	ifree_object(pmap, ".setdevicenspace(map)");
	return code;
    }
    rc_decrement(pmap, ".setdevicenspace(map)");  /* build sets rc = 1 */
    pop(1);
    return 0;
}

/* ------ Internal procedures ------ */

/* Forward references */
private int devicen_remap_transform(P5(const gs_device_n_params * params,
				       const float *in, float *out,
				       const gs_imager_state *pis,
				       void *data));
private int devicen_remap_prepare(P1(i_ctx_t *));
private int devicen_remap_finish(P1(i_ctx_t *));
private int devicen_remap_cleanup(P1(i_ctx_t *));

/* Map to a concrete color by calling the tint_transform procedure. */
private int
ztransform_DeviceN(const gs_device_n_params * params, const float *in,
		   float *out, const gs_imager_state *pis, void *data)
{
    /* Just schedule a call on the real tint_transform. */
    int_remap_color_info_t *prci =
	r_ptr(&gs_int_gstate((const gs_state *)pis)->remap_color_info,
	      int_remap_color_info_t);

    prci->proc = devicen_remap_prepare;
    memcpy(prci->tint, in, params->num_components * sizeof(float));
    return_error(e_RemapColor);
}

/* Prepare to run tint_transform. */
private int
devicen_remap_prepare(i_ctx_t *i_ctx_p)
{
    gs_state *pgs = igs;
    const gs_color_space *pcs = gs_currentcolorspace(pgs);
    const gs_color_space *pbcs;
    gs_client_color cc;
    frac ignore_conc[GX_DEVICE_COLOR_MAX_COMPONENTS];

    /*
     * Find the DeviceN space.  The worst case is an uncolored Pattern over
     * an Indexed space over a DeviceN space.
     */
    if (gs_color_space_get_index(pcs) == gs_color_space_index_Pattern) {
	pcs = gs_cspace_base_space(pcs);
    }
    pbcs = pcs;
    if (gs_color_space_get_index(pbcs) != gs_color_space_index_DeviceN) {
	pbcs = gs_cspace_base_space(pbcs);
    }
    /*
     * Temporarily set the tint_transform procedure in the color space to
     * the one that calls the PostScript code, and its data to the context
     * pointer.  This is a hack, but a localized one, to get an Indexed
     * color space to do the table lookup.
     */
    memcpy(cc.paint.values,
	   r_ptr(&istate->remap_color_info, int_remap_color_info_t)->tint,
	   pbcs->params.device_n.num_components * sizeof(float));
    pbcs->params.device_n.map->tint_transform = devicen_remap_transform;
    pbcs->params.device_n.map->tint_transform_data = i_ctx_p;
    return pcs->type->concretize_color(&cc, pcs, ignore_conc,
				       (gs_imager_state *)pgs);
}

/* Run the tint_transform procedure on the tint values. */
private int
devicen_remap_transform(const gs_device_n_params * params, const float *in,
			float *out, const gs_imager_state *pis, void *data)
{
    i_ctx_t *i_ctx_p = data;
    int num_in = params->num_components;
    int num_out =
	gs_color_space_num_components((const gs_color_space *)
				      &params->alt_space);
    int i;

    check_estack(num_in + 4);
    check_ostack(num_in);
    for (i = 0; i < num_in; ++i) {
	++osp;
	make_real(osp, in[i]);
	*++esp = *osp;
    }
    ++esp;
    make_int(esp, num_in);
    push_mark_estack(es_other, devicen_remap_cleanup);
    push_op_estack(devicen_remap_finish);
    *++esp = istate->colorspace.procs.special.device_n.tint_transform;
    /*
     * Initialize the output values to nominal legal ones so that the
     * concretize_color procedure doesn't get arithmetic exceptions.
     */
    for (i = 0; i < num_out; ++i)
	out[i] = 0;
    /* Restore the tint_transform in the color space. */
    params->map->tint_transform = ztransform_DeviceN;
    params->map->tint_transform_data = 0;
    return o_push_estack;
}

/* Save the transformed color value. */
private int
devicen_remap_finish(i_ctx_t *i_ctx_p)
{
    gs_state *pgs = igs;
    const gs_color_space *pcs = gs_currentcolorspace(pgs);
    const gs_device_n_params *params;
    const gs_color_space *pacs;
    gs_device_n_map *map;
    int num_in, num_out;
    float conc[GX_DEVICE_COLOR_MAX_COMPONENTS];
    int code;
    int i;

    while (gs_color_space_get_index(pcs) != gs_color_space_index_DeviceN) {
	pcs = gs_cspace_base_space(pcs);
    }
    params = &pcs->params.device_n;
    num_in = params->num_components; /* also on e-stack */
    pacs = (const gs_color_space *)&params->alt_space;
    map = params->map;
    num_out = gs_color_space_num_components(pacs);
    code = float_params(osp, num_out, conc);
    if (code < 0)
	return code;
    esp -= num_in + 2;		/* mark, count, tint values */
    for (i = 0; i < num_in; ++i)
	map->tint[i] = esp[i + 1].value.realval;
    for (i = 0; i < num_out; ++i)
	map->conc[i] = float2frac(conc[i]);
    map->cache_valid = true;
    osp -= num_out;
    return o_pop_estack;
}

/* Clean up by removing the tint values from the e-stack. */
private int
devicen_remap_cleanup(i_ctx_t *i_ctx_p)
{
    int num_in = esp->value.intval;

    esp -= num_in + 1;
    return o_pop_estack;
}

/* ------ Initialization procedure ------ */

const op_def zcsdevn_op_defs[] =
{
    op_def_begin_ll3(),
    {"1.setdevicenspace", zsetdevicenspace},
    {"0%devicen_remap_prepare", devicen_remap_prepare},
    op_def_end(0)
};
