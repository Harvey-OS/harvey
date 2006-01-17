/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: zcsdevn.c,v 1.12 2004/08/04 19:36:13 stefan Exp $ */
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
#include "zht2.h"

/* Imported from gscdevn.c */
extern const gs_color_space_type gs_color_space_type_DeviceN;

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
    const gs_color_space * pacs;
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
    pacs = gs_currentcolorspace(igs);
    cs = *pacs;
    /* See zcsindex.c for why we use memmove here. */
    memmove(&cs.params.device_n.alt_space, &cs,
	    sizeof(cs.params.device_n.alt_space));
    gs_cspace_init(&cs, &gs_color_space_type_DeviceN, imemory, false);
    code = gs_build_DeviceN(&cs, num_components, pacs, imemory);
    if (code < 0)
	return code;
    names = cs.params.device_n.names;
    pmap = cs.params.device_n.map;
    cs.params.device_n.get_colorname_string = gs_get_colorname_string;

    /* Pick up the names of the components */
    {
	uint i;
	ref sname;

	for (i = 0; i < num_components; ++i) {
	    array_get(imemory, pcsa, (long)i, &sname);
	    switch (r_type(&sname)) {
		case t_string:
		    code = name_from_string(imemory, &sname, &sname);
		    if (code < 0) {
			ifree_object(names, ".setdevicenspace(names)");
			ifree_object(pmap, ".setdevicenspace(map)");
			return code;
		    }
		    /* falls through */
		case t_name:
		    names[i] = name_index(imemory, &sname);
		    break;
		default:
		    ifree_object(names, ".setdevicenspace(names)");
		    ifree_object(pmap, ".setdevicenspace(map)");
		    return_error(e_typecheck);
	    }
	}
    }

    /* Now set the current color space as DeviceN */

    cspace_old = istate->colorspace;
    /*
     * pcsa is a pointer to element 1 (2nd element)  in the DeviceN
     * description array.  Thus pcsa[2] is element #3 (4th element)
     * which is the tint transform.
     */
    istate->colorspace.procs.special.device_n.layer_names = pcsa[0];
    istate->colorspace.procs.special.device_n.tint_transform = pcsa[2];    
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
    if (code < 0) {
	istate->colorspace = cspace_old;
	return code;
    }
    rc_decrement(pmap, ".setdevicenspace(map)");  /* build sets rc = 1 */
    pop(1);
    return 0;
}


/* ------ Initialization procedure ------ */

const op_def zcsdevn_op_defs[] =
{
    op_def_begin_ll3(),
    {"1.setdevicenspace", zsetdevicenspace},
    op_def_end(0)
};
