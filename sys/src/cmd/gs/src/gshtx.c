/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gshtx.c,v 1.6 2004/08/04 19:36:12 stefan Exp $ */
/* Stand-alone halftone/transfer function related code */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsutil.h"		/* for gs_next_ids */
#include "gxfmap.h"
#include "gzstate.h"
#include "gzht.h"
#include "gshtx.h"		/* must come after g*ht.h */

/*
 * Procedure to free the set of components when a halftone is released.
 */
private void
free_comps(
	      gs_memory_t * pmem,
	      void *pvht,
	      client_name_t cname
)
{
    gs_ht *pht = (gs_ht *) pvht;

    gs_free_object(pmem, pht->params.ht_multiple.components, cname);
    gs_free_object(pmem, pvht, cname);
}

/*
 * Stub transfer function, to be applied to components that are not provided
 * with a transfer function.
 */
private float
null_closure_transfer(
			 floatp val,
			 const gx_transfer_map * pmap_dummy,	/* NOTUSED */
			 const void *dummy	/* NOTUSED */
)
{
    return val;
}


/*
 * Build a gs_ht halftone structure.
 */
int
gs_ht_build(
	       gs_ht ** ppht,
	       uint num_comps,
	       gs_memory_t * pmem
)
{
    gs_ht *pht;
    gs_ht_component *phtc;
    int i;

    /* must have at least one component */
    *ppht = 0;
    if (num_comps == 0)
	return_error(gs_error_rangecheck);

    /* allocate the halftone and the array of components */
    rc_alloc_struct_1(pht,
		      gs_ht,
		      &st_gs_ht,
		      pmem,
		      return_error(gs_error_VMerror),
		      "gs_ht_build"
	);
    phtc = gs_alloc_struct_array(pmem,
				 num_comps,
				 gs_ht_component,
				 &st_ht_comp_element,
				 "gs_ht_build"
	);
    if (phtc == 0) {
	gs_free_object(pmem, pht, "gs_ht_build");
	return_error(gs_error_VMerror);
    }
    /* initialize the halftone */
    pht->type = ht_type_multiple;
    pht->rc.free = free_comps;
    pht->params.ht_multiple.components = phtc;
    pht->params.ht_multiple.num_comp = num_comps;

    for (i = 0; i < num_comps; i++) {
        phtc[i].comp_number = i;
	phtc[i].cname = 0;
	phtc[i].type = ht_type_none;
    }

    *ppht = pht;

    return 0;
}

/*
 * Set a spot-function halftone component in a gs_ht halftone.
 */
int
gs_ht_set_spot_comp(
		       gs_ht * pht,
		       int comp,
		       floatp freq,
		       floatp angle,
		       float (*spot_func) (floatp, floatp),
		       bool accurate,
		       gs_ht_transfer_proc transfer,
		       const void *client_data
)
{
    gs_ht_component *phtc = &(pht->params.ht_multiple.components[comp]);

    if (comp >= pht->params.ht_multiple.num_comp)
	return_error(gs_error_rangecheck);
    if (phtc->type != ht_type_none)
	return_error(gs_error_invalidaccess);

    phtc->type = ht_type_spot;
    phtc->params.ht_spot.screen.frequency = freq;
    phtc->params.ht_spot.screen.angle = angle;
    phtc->params.ht_spot.screen.spot_function = spot_func;
    phtc->params.ht_spot.accurate_screens = accurate;
    phtc->params.ht_spot.transfer = gs_mapped_transfer;

    phtc->params.ht_spot.transfer_closure.proc =
	(transfer == 0 ? null_closure_transfer : transfer);
    phtc->params.ht_spot.transfer_closure.data = client_data;

    return 0;
}

/*
 * Set a threshold halftone component in a gs_ht halftone. Note that the
 * caller is responsible for releasing the threshold data.
 */
int
gs_ht_set_threshold_comp(
			    gs_ht * pht,
			    int comp,
			    int width,
			    int height,
			    const gs_const_string * thresholds,
			    gs_ht_transfer_proc transfer,
			    const void *client_data
)
{
    gs_ht_component *phtc = &(pht->params.ht_multiple.components[comp]);

    if (comp >= pht->params.ht_multiple.num_comp)
	return_error(gs_error_rangecheck);
    if (phtc->type != ht_type_none)
	return_error(gs_error_invalidaccess);

    phtc->type = ht_type_threshold;
    phtc->params.ht_threshold.width = width;
    phtc->params.ht_threshold.height = height;
    phtc->params.ht_threshold.thresholds = *thresholds;
    phtc->params.ht_threshold.transfer = gs_mapped_transfer;

    phtc->params.ht_threshold.transfer_closure.proc =
	(transfer == 0 ? null_closure_transfer : transfer);
    phtc->params.ht_threshold.transfer_closure.data = client_data;

    return 0;
}

/*
 * Increase the reference count of a gs_ht structure by 1.
 */
void
gs_ht_reference(
		   gs_ht * pht
)
{
    rc_increment(pht);
}

/*
 * Decrement the reference count of a gs_ht structure by 1. Free the
 * structure if the reference count reaches 0.
 */
void
gs_ht_release(
		 gs_ht * pht
)
{
    rc_decrement_only(pht, "gs_ht_release");
}


/*
 *  Verify that a gs_ht halftone is legitimate.
 */
private int
check_ht(
	    gs_ht * pht
)
{
    int i;
    int num_comps = pht->params.ht_multiple.num_comp;

    if (pht->type != ht_type_multiple)
	return_error(gs_error_unregistered);
    for (i = 0; i < num_comps; i++) {
	gs_ht_component *phtc = &(pht->params.ht_multiple.components[i]);
	if ((phtc->type != ht_type_spot) && (phtc->type != ht_type_threshold))
	    return_error(gs_error_unregistered);
    }
    return 0;
}

/*
 *  Load a transfer map from a gs_ht_transfer_proc function.
 */
private void
build_transfer_map(
		      gs_ht_component * phtc,
		      gx_transfer_map * pmap
)
{
    gs_ht_transfer_proc proc;
    const void *client_info;
    int i;
    frac *values = pmap->values;

    if (phtc->type == ht_type_spot) {
	proc = phtc->params.ht_spot.transfer_closure.proc;
	client_info = phtc->params.ht_spot.transfer_closure.data;
    } else {
	proc = phtc->params.ht_threshold.transfer_closure.proc;
	client_info = phtc->params.ht_threshold.transfer_closure.data;
    }

    for (i = 0; i < transfer_map_size; i++) {
	float fval =
	    proc(i * (1 / (double)(transfer_map_size - 1)), pmap, client_info);

	values[i] =
	    (fval <= 0.0 ? frac_0 : fval >= 1.0 ? frac_1 :
	     float2frac(fval));
    }
}

/*
 *  Allocate the order and transfer maps required by a halftone, and perform
 *  some elementary initialization. This will also build the component index
 *  to order index map.
 */
private gx_ht_order_component *
alloc_ht_order(
		  const gs_ht * pht,
		  gs_memory_t * pmem,
		  byte * comp2order
)
{
    int num_comps = pht->params.ht_multiple.num_comp;
    gx_ht_order_component *pocs = gs_alloc_struct_array(
							   pmem,
					   pht->params.ht_multiple.num_comp,
						      gx_ht_order_component,
					     &st_ht_order_component_element,
							   "alloc_ht_order"
    );
    int inext = 0;
    int i;

    if (pocs == 0)
	return 0;
    pocs->corder.transfer = 0;

    for (i = 0; i < num_comps; i++) {
	gs_ht_component *phtc = &(pht->params.ht_multiple.components[i]);
	gx_transfer_map *pmap = gs_alloc_struct(pmem,
						gx_transfer_map,
						&st_transfer_map,
						"alloc_ht_order"
	);

	if (pmap == 0) {
	    int j;

	    for (j = 0; j < inext; j++)
		gs_free_object(pmem, pocs[j].corder.transfer, "alloc_ht_order");
	    gs_free_object(pmem, pocs, "alloc_ht_order");
	    return 0;
	}
	pmap->proc = gs_mapped_transfer;
	pmap->id = gs_next_ids(pmem, 1);
	pocs[inext].corder.levels = 0;
	pocs[inext].corder.bit_data = 0;
	pocs[inext].corder.cache = 0;
	pocs[inext].corder.transfer = pmap;
	pocs[inext].cname = phtc->cname;
        pocs[inext].comp_number = phtc->comp_number;
	comp2order[i] = inext++;
    }

    return pocs;
}

/*
 *  Build the halftone order for one component.
 */
private int
build_component(
		   gs_ht_component * phtc,
		   gx_ht_order * porder,
		   gs_state * pgs,
		   gs_memory_t * pmem
)
{
    if (phtc->type == ht_type_spot) {
	gs_screen_enum senum;
	int code;

	code = gx_ht_process_screen_memory(&senum,
					   pgs,
					   &phtc->params.ht_spot.screen,
				      phtc->params.ht_spot.accurate_screens,
					   pmem
	    );
	if (code < 0)
	    return code;

	/* avoid wiping out the transfer structure pointer */
	senum.order.transfer = porder->transfer;
	*porder = senum.order;

    } else {			/* ht_type_threshold */
	int code;
	gx_transfer_map *transfer = porder->transfer;

	porder->params.M = phtc->params.ht_threshold.width;
	porder->params.N = 0;
	porder->params.R = 1;
	porder->params.M1 = phtc->params.ht_threshold.height;
	porder->params.N1 = 0;
	porder->params.R1 = 1;
	code = gx_ht_alloc_threshold_order(porder,
					   phtc->params.ht_threshold.width,
					   phtc->params.ht_threshold.height,
					   256,
					   pmem
	    );
	if (code < 0)
	    return code;
	gx_ht_construct_threshold_order(
				porder,
				phtc->params.ht_threshold.thresholds.data
	    );
	/*
	 * gx_ht_construct_threshold_order wipes out transfer map pointer,
	 * restore it here.
	 */
	porder->transfer = transfer;
    }

    build_transfer_map(phtc, porder->transfer);
    return 0;
}

/*
 * Free an order array and all elements it points to.
 */
private void
free_order_array(
		    gx_ht_order_component * pocs,
		    int num_comps,
		    gs_memory_t * pmem
)
{
    int i;

    for (i = 0; i < num_comps; i++)
	gx_ht_order_release(&(pocs[i].corder), pmem, true);
    gs_free_object(pmem, pocs, "gs_ht_install");
}


/*
 *  Install a gs_ht halftone as the current halftone in the graphic state.
 */
int
gs_ht_install(
		 gs_state * pgs,
		 gs_ht * pht
)
{
    int code = 0;
    gs_memory_t *pmem = pht->rc.memory;
    gx_device_halftone dev_ht;
    gx_ht_order_component *pocs;
    byte comp2order[32];	/* ample component to order map */
    int num_comps = pht->params.ht_multiple.num_comp;
    int i;

    /* perform so sanity checks (must have one default component) */
    if ((code = check_ht(pht)) != 0)
	return code;

    /* allocate the halftone order structure and transfer maps */
    if ((pocs = alloc_ht_order(pht, pmem, comp2order)) == 0)
	return_error(gs_error_VMerror);

    /* build all of the order for each component */
    for (i = 0; i < num_comps; i++) {
	int j = comp2order[i];

	code = build_component(&(pht->params.ht_multiple.components[i]),
			       &(pocs[j].corder),
			       pgs,
			       pmem
	    );

	if ((code >= 0) && (j != 0)) {
	    gx_ht_cache *pcache;

	    pcache = gx_ht_alloc_cache(pmem,
				       4,
				       pocs[j].corder.raster *
				       (pocs[j].corder.num_bits /
					pocs[j].corder.width) * 4
		);

	    if (pcache == 0)
		code = gs_note_error(gs_error_VMerror);
	    else {
		pocs[j].corder.cache = pcache;
		gx_ht_init_cache(pmem, pcache, &(pocs[j].corder));
	    }
	}
	if (code < 0)
	    break;
    }

    if (code < 0) {
	free_order_array(pocs, num_comps, pmem);
	return code;
    }
    /* initialize the device halftone structure */
    dev_ht.rc.memory = pmem;
    dev_ht.order = pocs[0].corder;	/* Default */
    if (num_comps == 1) {
	/* we have only a Default; we don't need components. */
	gs_free_object(pmem, pocs, "gs_ht_install");
	dev_ht.components = 0;
    } else {
	dev_ht.components = pocs;
	dev_ht.num_comp = num_comps;
    }

    /* at last, actually install the halftone in the graphic state */
    if ((code = gx_ht_install(pgs, (gs_halftone *) pht, &dev_ht)) < 0)
        gx_device_halftone_release(&dev_ht, pmem);
    return code;
}

/* ---------------- Mask-defined halftones ---------------- */

/*
 * Create a halftone order from an array of explicit masks.  This is
 * silly, because the rendering machinery actually wants masks, but doing
 * it right seems to require too many changes in existing code.
 */
private int
create_mask_bits(const byte * mask1, const byte * mask2,
		 int width, int height, gx_ht_bit * bits)
{
    /*
     * We do this with the slowest, simplest possible algorithm....
     */
    int width_bytes = (width + 7) >> 3;
    int x, y;
    int count = 0;

    for (y = 0; y < height; ++y)
	for (x = 0; x < width; ++x) {
	    int offset = y * width_bytes + (x >> 3);
	    byte bit_mask = 0x80 >> (x & 7);

	    if ((mask1[offset] ^ mask2[offset]) & bit_mask) {
		if (bits)
		    gx_ht_construct_bit(&bits[count], width, y * width + x);
		++count;
	    }
	}
    return count;
}
private int
create_mask_order(gx_ht_order * porder, gs_state * pgs,
		  const gs_client_order_halftone * phcop,
		  gs_memory_t * mem)
{
    int width_bytes = (phcop->width + 7) >> 3;
    const byte *masks = (const byte *)phcop->client_data;
    int bytes_per_mask = width_bytes * phcop->height;
    const byte *prev_mask;
    int num_levels = phcop->num_levels;
    int num_bits = 0;
    int i;
    int code;

    /* Do a first pass to compute how many bits entries will be needed. */
    for (prev_mask = masks, num_bits = 0, i = 0;
	 i < num_levels - 1;
	 ++i, prev_mask += bytes_per_mask
	)
	num_bits += create_mask_bits(prev_mask, prev_mask + bytes_per_mask,
				     phcop->width, phcop->height, NULL);
    code = gx_ht_alloc_client_order(porder, phcop->width, phcop->height,
				    num_levels, num_bits, mem);
    if (code < 0)
	return code;
    /* Fill in the bits and levels entries. */
    for (prev_mask = masks, num_bits = 0, i = 0;
	 i < num_levels - 1;
	 ++i, prev_mask += bytes_per_mask
	) {
	porder->levels[i] = num_bits;
	num_bits += create_mask_bits(prev_mask, prev_mask + bytes_per_mask,
				     phcop->width, phcop->height,
				     ((gx_ht_bit *)porder->bit_data) +
				      num_bits);
    }
    porder->levels[num_levels - 1] = num_bits;
    return 0;
}

/* Define the client-order halftone procedure structure. */
private const gs_client_order_ht_procs_t mask_order_procs =
{
    create_mask_order
};

/*
 * Define a halftone by an explicit set of masks.  We translate these
 * internally into a threshold array, since that's what the halftone
 * rendering machinery knows how to deal with.
 */
int
gs_ht_set_mask_comp(gs_ht * pht,
		    int component_index,
		    int width, int height, int num_levels,
		    const byte * masks,		/* width x height x num_levels bits */
		    gs_ht_transfer_proc transfer,
		    const void *client_data)
{
    gs_ht_component *phtc =
    &(pht->params.ht_multiple.components[component_index]);

    if (component_index >= pht->params.ht_multiple.num_comp)
	return_error(gs_error_rangecheck);
    if (phtc->type != ht_type_none)
	return_error(gs_error_invalidaccess);

    phtc->type = ht_type_client_order;
    phtc->params.client_order.width = width;
    phtc->params.client_order.height = height;
    phtc->params.client_order.num_levels = num_levels;
    phtc->params.client_order.procs = &mask_order_procs;
    phtc->params.client_order.client_data = masks;
    phtc->params.client_order.transfer_closure.proc =
	(transfer == 0 ? null_closure_transfer : transfer);
    phtc->params.client_order.transfer_closure.data = client_data;

    return 0;

}
