/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: zcsdevn.c,v 1.5.2.1 2002/01/17 06:57:55 dancoby Exp $ */
/* DeviceN color space support */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gxcspace.h"		/* must precede gscolor2.h */
#include "gscolor2.h"
#include "gscdevn.h"
#include "gxcdevn.h"
#include "estack.h"
#include "ialloc.h"
#include "icremap.h"
#include "ifunc.h"
#include "igstate.h"
#include "iname.h"

/* Imported from gscdevn.c */
extern const gs_color_space_type gs_color_space_type_DeviceN;

/*
 * This routine is used as an interpeter callback function for the
 * graphics library.  This routine translates a colorname_index value,
 * (which is how the separation and DeviceN colorant names are passed
 * to the graphics library) into a character string pointer and a
 * string length.
 */
private int
gs_get_colorname_string(gs_separation_name colorname_index,
			unsigned char **ppstr, unsigned int *pname_size)
{
    ref nref;

    name_index_ref(colorname_index, &nref);
    name_string_ref(&nref, &nref);
    return obj_string_data(&nref, (const unsigned char**) ppstr, pname_size);
}

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
    gs_function_t *pfn;
    int code;

    /* Verify that we have an array as our input parameter */
    check_read_type(*op, t_array);
    if (r_size(op) != 4)
	return_error(e_rangecheck);

    /* pcsa is a pointer to the color names array (element 1 in input array) */
    pcsa = op->value.const_refs + 1;
    if (!r_is_array(pcsa))
	return_error(e_typecheck);
    num_components = r_size(pcsa);
    if (num_components == 0)
	return_error(e_rangecheck);
    if (num_components > GS_CLIENT_COLOR_MAX_COMPONENTS)
	return_error(e_limitcheck);

    /* Check tint transform procedure.  Note: Cheap trick to get pointer to it.
       The tint transform procedure is element 3 in the input array */
    check_proc(pcsa[2]);
    
    /* The alternate color space has been selected as the current color space */
    cs = *gs_currentcolorspace(igs);
    if (!cs.type->can_be_alt_space)
	return_error(e_rangecheck);
    /*
     * Allocate space for DeviceN color map function.  This can be either
     * a type 4 function or we can collecte data for a color cube (type 0
     * function).
     */
    code = alloc_device_n_map(&pmap, imemory, ".setdevicenspace(map)");
    if (code < 0)
	return code;

    /* Allocate space for color names list and copy names from input array. */
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

    /* Now set the current color space as DeviceN */

    /* See zcsindex.c for why we use memmove here. */
    memmove(&cs.params.device_n.alt_space, &cs,
	    sizeof(cs.params.device_n.alt_space));
    gs_cspace_init(&cs, &gs_color_space_type_DeviceN, NULL);
    cspace_old = istate->colorspace;
    /*
     * pcsa is a pointer to element 1 (2nd element)  in the DeviceN
     * description array.  Thus pcsa[2] is element #3 (4th element)
     * which is the tint transform.
     */
    istate->colorspace.procs.special.device_n.layer_names = pcsa[0];
    istate->colorspace.procs.special.device_n.tint_transform = pcsa[2];
    cs.params.device_n.names = names;
    cs.params.device_n.num_components = num_components;
    cs.params.device_n.map = pmap;
    cs.params.device_n.get_colorname_string = gs_get_colorname_string;
    pfn = ref_function(pcsa + 2);	/* See comment above */
    if (!pfn)
	code = gs_note_error(e_rangecheck);

    if (code < 0) {
	istate->colorspace = cspace_old;
	ifree_object(names, ".setdevicenspace(names)");
	ifree_object(pmap, ".setdevicenspace(map)");
	return code;
    }
    gs_cspace_set_devn_function(&cs, pfn);
    code = gs_setcolorspace(igs, &cs);
    rc_decrement(pmap, ".setdevicenspace(map)");  /* build sets rc = 1 */
    pop(1);
    return code;
}


/* ------ Initialization procedure ------ */

const op_def zcsdevn_op_defs[] =
{
    op_def_begin_ll3(),
    {"1.setdevicenspace", zsetdevicenspace},
    op_def_end(0)
};

