/* Copyright (C) 2002 artofcode LLC.  All rights reserved.
  
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

/* $Id: gxdhtserial.c,v 1.8 2005/03/14 18:08:37 dan Exp $ */
/* Serialization and de-serialization for (traditional) halftones */

#include "memory_.h"
#include <assert.h>
#include "gx.h"
#include "gscdefs.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsutil.h"             /* for gs_next_ids */
#include "gzstate.h"
#include "gxdevice.h"           /* for gzht.h */
#include "gzht.h"
#include "gswts.h"
#include "gxdhtres.h"
#include "gsserial.h"
#include "gxdhtserial.h"


/*
 * Declare the set of procedures that return resident halftones. This
 * declares both the array of procedures and their type. It is used
 * only to check if a transmitted halftone order matches one in ROM.
 */
extern_gx_device_halftone_list();


/*
 * An enumeration of halftone transfer functions. These must distinguish
 * between cases in which no transfer function is present, and when one
 * is present but provides the identity transformation (an empty
 * PostScript array).
 */
typedef enum {
    gx_ht_tf_none = 0,
    gx_ht_tf_identity,
    gx_ht_tf_complete
} gx_ht_tf_type_t;

/* enumeration to distinguish well-tempered screening orders from others */
typedef enum {
    gx_ht_traditional,
    gx_ht_wts
} gx_ht_order_type_t;


/*
 * Serialize a transfer function. These will occupy one byte if they are
 * not present or provide an identity mapping,
 * 1 + transfer_map_size * sizeof(frac) otherwise.
 *
 * Returns:
 *
 *    0, with *psize set the the amount of space required, if successful
 *
 *    gs_error_rangecheck, with *psize set to the size required, if the
 *        original *psize was not large enough
 */
private int
gx_ht_write_tf(
    const gx_transfer_map * pmap,
    byte *                  data,    
    uint *                  psize )
{
    int                     req_size = 1;   /* minimum of one byte */

    /* check for sufficient space */
    if ( pmap != 0 && pmap->proc != gs_identity_transfer)
        req_size += sizeof(pmap->values);
    if (req_size > *psize) {
        *psize = req_size;
        return gs_error_rangecheck;
    }

    if (req_size == 1)
        *data = (byte)(pmap == 0 ? gx_ht_tf_none : gx_ht_tf_identity);
    else {
        *data++ = (byte)gx_ht_tf_complete;
        memcpy(data, pmap->values, sizeof(pmap->values));
    }

    *psize = req_size;
    return 0;
}

/*
 * Reconstruct a transfer function from its serial representation. The
 * buffer provided is expected to be large enough to hold the entire
 * transfer function.
 *
 * Returns the number of bytes read, or < 0 in the event of an error.
 */
private int
gx_ht_read_tf(
    gx_transfer_map **  ppmap,
    const byte *        data,
    uint                size,
    gs_memory_t *       mem )
{
    gx_ht_tf_type_t     tf_type;
    gx_transfer_map *   pmap;

    /* read the type byte */
    if (size == 0)
        return_error(gs_error_rangecheck);
    --size;
    tf_type = (gx_ht_tf_type_t)*data++;

    /* if no transfer function, exit now */
    if (tf_type == gx_ht_tf_none) {
        *ppmap = 0;
        return 1;
    }

    /* allocate a transfer map */
    rc_alloc_struct_1( pmap,
                       gx_transfer_map,
                       &st_transfer_map,
                       mem,
                       return_error(gs_error_VMerror),
                       "gx_ht_read_tf" );

    pmap->id = gs_next_ids(mem, 1);
    pmap->closure.proc = 0;
    pmap->closure.data = 0;
    if (tf_type == gx_ht_tf_identity) {
        gx_set_identity_transfer(pmap);
        return 1;
    } else if (tf_type == gx_ht_tf_complete && size >= sizeof(pmap->values)) {
        memcpy(pmap->values, data, sizeof(pmap->values));
        pmap->proc = gs_mapped_transfer;
        *ppmap = pmap;
        return 1 + sizeof(pmap->values);
    } else {
        rc_decrement(pmap, "gx_ht_read_tf");
        return_error(gs_error_rangecheck);
    }
}


/*
 * Serialize a halftone component. The only part that is serialized is the
 * halftone order; the other two components are only required during
 * halftone construction.
 *
 * Returns:
 *
 *    0, with *psize set the the amount of space required, if successful
 *
 *    gs_error_rangecheck, with *psize set to the size required, if the
 *        original *psize was not large enough
 *
 *    some other error code, with *psize unchange, in the event of an
 *        error other than lack of space
 */
private int
gx_ht_write_component(
    const gx_ht_order_component *   pcomp,
    byte *                          data,
    uint *                          psize )
{
    const gx_ht_order *             porder = &pcomp->corder;
    byte *                          data0 = data;
    int                             code, levels_size, bits_size;
    uint			    tmp_size = 0;
    int                             req_size;

    /*
     * There is no need to transmit the comp_number field, as this must be
     * the same as the index in the component array (see gx_ht_write).
     *
     * There is also no reason to transmit the colorant name (cname), as
     * this is only used by some high-level devices that would not be targets
     * of the command list device (and even those devices should be able to
     * get the information from their color models).
     *
     * This leaves the order itself.
     *
     * Check if we are a well-tempered-screening order. Serialization of these
     * is not yet implemented.
     */
    if (porder->wts != 0)
       return_error(gs_error_unknownerror);     /* not yet supported */

    /*
     * The following order fields are not transmitted:
     *
     *  params          Only required during halftone cell construction
     *
     *  wse, wts        Only used for well-tempered screens (see above)
     *
     *  raster          Can be re-calculated by the renderer from the width
     *
     *  orig_height,    The only potential use for these parameters is in
     *  orig_shift      this routine; they are not useful to the renderer.
     *
     *  full_height     Can be re-calculated by the renderer from the
     *                  height, width, and shift values.
     *
     *  data_memory     Must be provided by the renderer.
     *
     *  cache           Must be provided by the renderer.
     *
     *  screen_params   Ony required during halftone cell construction
     *
     * In addition, the procs parameter is passed as an index into the
     * ht_order_procs_table, as the renderer may not be in the same address
     * space as the writer.
     *
     * Calculate the size required.
     */
    levels_size = porder->num_levels * sizeof(porder->levels[0]); 
    bits_size = porder->num_bits * porder->procs->bit_data_elt_size;
    req_size =   1          /* gx_ht_type_t */
               + enc_u_sizew(porder->width)
               + enc_u_sizew(porder->height)
               + enc_u_sizew(porder->shift)
               + enc_u_sizew(porder->num_levels)
               + enc_u_sizew(porder->num_bits)
               + 1          /* order procs, as index into table */
               + levels_size
               + bits_size;
    code = gx_ht_write_tf(porder->transfer, data, &tmp_size);
    if (code < 0 && code != gs_error_rangecheck)
        return code;
    req_size += tmp_size;
    if (req_size > *psize) {
        *psize = req_size;
        return gs_error_rangecheck;
    }

    /* identify this as a traditional halftone */
    *data++ = (byte)gx_ht_traditional;

    /* write out the dimensional data */
    enc_u_putw(porder->width, data);
    enc_u_putw(porder->height, data);
    enc_u_putw(porder->shift, data);
    enc_u_putw(porder->num_levels, data);
    enc_u_putw(porder->num_bits, data);

    /* white out the procs index */
    *data++ = porder->procs - ht_order_procs_table;

    /* copy the levels array and whitening order array */
    memcpy(data, porder->levels, levels_size);
    data += levels_size;
    memcpy(data, porder->bit_data, bits_size);
    data += bits_size;

    /* write out the transfer function */
    tmp_size = *psize - (data - data0);
    if ((code = gx_ht_write_tf(porder->transfer, data, &tmp_size)) == 0)
        *psize = tmp_size + (data - data0);
    return code;
}

/*
 * Reconstruct a halftone component from its serial representation. The
 * buffer provided is expected to be large enough to hold the entire
 * halftone component.
 *
 * Because halftone components are allocated in arrays (an unfortunate
 * arrangement, as it prevents component sharing), a pointer to an
 * already allocated component structure is passed as an operand, as
 * opposed to the more normal mechanism that would have a read routine
 * allocate the component. The memory pointer is still passed, however,
 * as the levels and bit_data arrays must be allocated.
 *
 * Returns the number of bytes read, or < 0 in the event of an error.
 */
private int
gx_ht_read_component(
    gx_ht_order_component * pcomp,
    const byte *            data,
    uint                    size,
    gs_memory_t *           mem )
{
    gx_ht_order             new_order;
    const byte *            data0 = data;
    const byte *            data_lim = data + size;
    gx_ht_order_type_t      order_type;
    int                     i, code, levels_size, bits_size;
    const gx_dht_proc *     phtrp = gx_device_halftone_list;

    /* check the order type */
    if (size == 0)
        return_error(gs_error_rangecheck);
    --size;
    order_type = (gx_ht_order_type_t)*data++;

    /* currently only the traditional halftone order are supported */
    if (order_type != gx_ht_traditional)
       return_error(gs_error_unknownerror);

    /*
     * For performance reasons, the number encoding macros do not
     * support full buffer size verification. The code below verifies
     * that a minimum number of bytes is available, then converts
     * blindly and does not check again until the various integers are
     * read. Obviously this can be hazardous, but should not be a
     * problem in practice, as the calling code should have verified
     * that the data provided holds the entire halftone.
     */
    if (size < 7)
        return_error(gs_error_rangecheck);
    enc_u_getw(new_order.width, data);
    enc_u_getw(new_order.height, data);
    enc_u_getw(new_order.shift, data);
    enc_u_getw(new_order.num_levels, data);
    enc_u_getw(new_order.num_bits, data);
    if (data >= data_lim)
        return_error(gs_error_rangecheck);
    new_order.procs = &ht_order_procs_table[*data++];

    /* calculate the space required for levels and bit data */
    levels_size = new_order.num_levels * sizeof(new_order.levels[0]);
    bits_size = new_order.num_bits * new_order.procs->bit_data_elt_size;

    /* + 1 below is for the minimal transfer function */
    if (data + bits_size + levels_size + 1 > data_lim)
        return_error(gs_error_rangecheck);

    /*
     * Allocate the levels and bit data structures. The gx_ht_alloc_ht_order
     * has a name that is both strange and misleading. The routine does
     * not allocate a halftone order. Rather, it initializes the order,
     * and allocates the levels and bit data arrays. In particular, it
     * sets all of the following fields:
     *
     *    wse = 0,
     *    wts = 0,
     *    width = operand width
     *    height = operand height
     *    raster = bitmap_raster(operand width)
     *    shift = operand shift
     *    orig_height = operand height
     *    orig_shift = operand strip_shift
     *    num_levels = operand num_levels
     *    num_bits = operand num_bits
     *    procs = operand procs
     *    levels = newly allocated array
     *    bit_data = new allocated array
     *    cache = 0
     *    transfer = 0
     *
     * Since several of the list fields are already set, this call
     * effectively sets them to the values they already have. This is a
     * bit peculiar but not otherwise harmful.
     *
     * For reasons that are not known and are probably historical, the
     * procedure does not initialize the params or screen_params fields.
     * In the unlikely event that these fields are ever contain pointers,
     * we initialize them explicitly here. Wse, params, and scrren_params
     * probably should not occur in the device halftone at all; they are
     * themselves historical artifacts.
     */
    code = gx_ht_alloc_ht_order( &new_order,
                                 new_order.width,
                                 new_order.height,
                                 new_order.num_levels,
                                 new_order.num_bits,
                                 new_order.shift,
                                 new_order.procs,
                                 mem );
    if (code < 0)
        return code;
    memset(&new_order.params, 0, sizeof(new_order.params));
    memset(&new_order.screen_params, 0, sizeof(new_order.screen_params));

    /* fill in the levels and bit_data arrays */
    memcpy(new_order.levels, data, levels_size);
    data += levels_size;
    memcpy(new_order.bit_data, data, bits_size);
    data += bits_size;

    /* process the transfer function */
    code = gx_ht_read_tf(&new_order.transfer, data, data_lim - data, mem);
    if (code < 0) {
        gx_ht_order_release(&new_order, mem, false);
        return code;
    }
    data += code;

    /*
     * Check to see if the order is in ROM. Since it is possible (if not
     * particularly likely) that the command list writer and renderer do
     * not have the same set of ROM-based halftones, the full halftone
     * order is transmitted and compared against the set ROM set provided
     * by the renderer. If there is a match, the transmitted version is
     * discarded and the ROM version used.
     *
     * It is not clear which, if any or the currently used devices
     * provide a ROM-based halftone order set.
     */
    for (i = 0; phtrp[i] != 0; i++) {
        const gx_device_halftone_resource_t *const *    pphtr = phtrp[i]();
        const gx_device_halftone_resource_t *           phtr;

        while ((phtr = *pphtr++) != 0) {
            /*
             * This test does not check for strict equality of the order,
             * nor is strict equality necessary. The ROM data will replace
             * just the levels and bit_data arrays of the transmitted
             * order, so only these must be the same. We don't even care
             * if the ROM's levels and bit_data arrays are larger; we
             * will never check values beyond the range required by the
             * current order.
             */
            if ( phtr->num_levels * sizeof(phtr->levels[0]) >= levels_size &&
                 phtr->Width * phtr->Height * phtr->elt_size >= bits_size  &&
                 memcmp(phtr->levels, new_order.levels, levels_size) == 0  &&
                 memcmp(phtr->bit_data, new_order.bit_data, bits_size) == 0  ) {
                /* the casts below are required to discard const qualifiers */
                gs_free_object(mem, new_order.bit_data, "gx_ht_read_component");
                new_order.bit_data = (void *)phtr->bit_data;
                gs_free_object(mem, new_order.levels, "gx_ht_read_component");
                new_order.levels = (uint *)phtr->levels;
                goto done;
            }
        }
    }

  done:
    /* everything OK, save the order and return the # of bytes read */
    pcomp->corder = new_order;
    pcomp->cname = 0;
    return data - data0;
}


/*
 * Serialize a halftone. The essential step is the serialization of the
 * halftone orders; beyond this only the halftone type must be
 * transmitted.
 *
 * Returns:
 *
 *    0, with *psize set the the amount of space required, if successful
 *
 *    gs_error_rangecheck, with *psize set to the size required, if the
 *        original *psize was not large enough
 *
 *    some other error code, with *psize unchange, in the event of an
 *        error other than lack of space
 */
int
gx_ht_write(
    const gx_device_halftone *  pdht,
    const gx_device *           dev,
    byte *                      data,
    uint *                      psize )
{
    int                         num_dev_comps = pdht->num_dev_comp;
    int                         i, code;
    uint                        req_size = 2, used_size = 2;
                                /* 1 for halftone type, 1 for num_dev_comps */

    /*
     * With the introduction of color models, there should never be a
     * halftone that includes just one component. Enforce this
     * restriction, even though it is not present in much of the rest
     * of the code.
     *
     * NB: the pdht->order field is ignored by this code.
     */
    assert(pdht != 0 && pdht->components != 0);

    /*
     * The following fields do not need to be transmitted:
     *
     *  order       Ignored by this code (see above).
     *
     *  rc, id      Recreated by the allocation code on the renderer.
     *
     *  lcm_width,  Can be recreated by the de-serialization code on the
     *  lcm_height  the renderer. Since halftones are transmitted
     *              infrequently (for normal jobs), the time required
     *              for re-calculation is not significant.
     *
     * Hence, the only fields that must be serialized are the type,and
     * the number of components.  (The number of components for the halftone
     * may not match the device's if we are compositing with a process color
     * model which does not match the output device.
     *
     * Several halftone components may be identical, but there is
     * currently no simple way to determine this. Halftones are normally
     * transmitted only once per page, so it is not clear that use of
     * such information would significantly reduce command list size.
     */

    /* calculate the required data space */
    for ( i = 0, code = gs_error_rangecheck;
          i < num_dev_comps && code == gs_error_rangecheck;
          i++) {
        uint     tmp_size = 0;

        /* sanity check */
        assert(i == pdht->components[i].comp_number);

        code = gx_ht_write_component( &pdht->components[i],
                                      data,
                                      &tmp_size );
        req_size += tmp_size;
    }
    if (code < 0 && code != gs_error_rangecheck)
        return code;
    else if (*psize < req_size) {
        *psize = req_size;
        return 0;
    }
    req_size = *psize;

    /* the halftone type is known to fit in a byte */
    *data++ = (byte)pdht->type;
    /* the number of components is known to fit in a byte */
    *data++ = (byte)num_dev_comps;

    /* serialize the halftone components */
    for (i = 0, code = 0; i < num_dev_comps && code == 0; i++) {
        uint    tmp_size = req_size - used_size;

        code = gx_ht_write_component( &pdht->components[i],
                                      data,
                                      &tmp_size );
        used_size += tmp_size;
        data += tmp_size;
    }

    if (code < 0) {
        if (code == gs_error_rangecheck)
            code = gs_error_unknownerror;
        return code;
    }

    *psize = used_size;
    return 0;
}

/*
 * Reconstruct a halftone from its serial representation, and install it
 * as the current halftone. The buffer provided is expected to be large
 * enough to hold the entire halftone.
 *
 * The reading and installation phases are combined in this routine so as
 * to avoid unnecessarily allocating a device halftone and its component
 * array, just to release them immediately after installation is complete.
 * There is also not much reason to reconstuct a halftone except to make
 * it the current halftone.
 *
 * Returns the number of bytes read, or <0 in the event of an error.
 */
int
gx_ht_read_and_install(
    gs_imager_state *       pis,
    const gx_device *       dev,
    const byte *            data,
    uint                    size,
    gs_memory_t *           mem )
{
    gx_ht_order_component   components[GX_DEVICE_COLOR_MAX_COMPONENTS];
    const byte *            data0 = data;
    gx_device_halftone      dht;
    int                     num_dev_comps;
    int                     i, code;

    /* fill in some fixed fields */
    memset(&dht.order, 0, sizeof(dht.order));
    memset(&dht.rc, 0, sizeof(dht.rc));
    dht.id = gs_no_id;      /* updated during installation */
    dht.components = components;
    dht.lcm_width = 1;      /* recalculated during installation */
    dht.lcm_height = 1;

    /* clear pointers in the components array in case we need to abort */
    memset(components, 0, sizeof(components));

    /* get the halftone type */
    if (size-- < 1)
        return_error(gs_error_rangecheck);
    dht.type = (gs_halftone_type)(*data++);
    num_dev_comps = dht.num_dev_comp = dht.num_comp = *data++;

    /* process the component orders */
    for (i = 0, code = 0; i < num_dev_comps && code >= 0; i++) {
        components[i].comp_number = i;
        code = gx_ht_read_component(&components[i], data, size, mem);
        if (code >= 0) {
            size -= code;
            data += code;
        }
    }

    /* if everything is OK, install the halftone */
    if (code >= 0)
        code = gx_imager_dev_ht_install(pis, &dht, dht.type, dev);

    /*
     * If installation failed, discard the allocated elements. We can't
     * use the gx_device_halftone_release procedure, as the components
     * array is on the stack rather than in the heap.
     */
    if (code < 0) {
        for (i = 0; i < num_dev_comps; i++)
            gx_ht_order_release(&components[i].corder, mem, false);
    }

    return code < 0 ? code : data - data0;
}
