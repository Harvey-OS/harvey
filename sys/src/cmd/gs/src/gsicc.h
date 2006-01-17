/* Copyright (C) 2001 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsicc.h,v 1.7 2002/12/03 02:41:04 dan Exp $ */
/* Structures for ICCBased color space parameters */
/* requires: gspsace.h, gscolor2.h */

#ifndef gsicc_INCLUDED
#  define gsicc_INCLUDED

#include "gscie.h"

/*
 * The gs_cie_icc_s structure will exist in all instantiations of the
 * graphic library, whether or not they provide support of the ICCBased
 * color spaces. If such support is not provided, we cannot assume that
 * the ICC support library (icclib) is included in the source code, so
 * we also cannot assume that the header file icc.h is available during
 * compilation. Hence we cannot include icc.h in this file.
 *
 * The following two opaque declarations are used to get around this
 * limitation.
 */
struct _icc;
struct _icmLuBase;

/*
 * ICCBased color spaces are a feature of PDF 1.3. They are effectively a
 * variation of the CIEBased color spaces, in which the mapping to XYZ space
 * is accomplished via an ICC profile rather than via matrices and procedures
 * in a color space dictionary.
 *
 * For historical reasons, the graphic library code for mapping XYZ colors
 * to the device color model assumes that a CIEBasedABC color space is
 * being used (there is a small exception for CIEBasedA). The mechanism used
 * by the CIE joint caches is dependent on this. To remain compatibile with
 * this arrangement, the gs_cie_icc_s structure must contain the
 * gs_cie_common_elements prefix, even though this structure serves only
 * as carrier of the white and black point information (all other parameters
 * are set to their default values).
 *
 * The icclib code was originally created to read from a file using the
 * stdio library. We can generalize the module to work with a positionable
 * stream with a simple change of header files. Somewhat more difficult
 * issues are involved adapting icclib to the graphic libraries memory
 * management scheme.
 *
 * The stream that comprises the filter is initially a PostScript object.
 * As such, it subject save/restore, garbage collection, relocation, may
 * be accessed and closed explicitly by the PostScript code. Most
 * significantly, a given stream structure instance may be re-used once it
 * has been closed.
 *
 * Unfortunately, the icclib code retains the stream pointer for
 * (potentially) long periods of time, and is blissfully unaware of all of
 * these possibilities. We also would like very much to avoid having to
 * make it aware of these complexities (so that we may easily use future
 * upgrades). The following methods are used to achieve this goal:
 *
 *    This color space structure may be used only so long as the PostScript
 *    color space array that generated it is still accessible. If this array
 *    is accessible, the stream is also accessible. Hence, we do not need to
 *    provide access to that stream via this this object, nor can we loose
 *    the stream via save/restore.
 *
 *    The color space objects themselves (the top-level profile structure
 *    pointed to by picc and the lookup object pointed to by plu) are 
 *    allocated in non-garbage collected ("foreign") memory, so we do not
 *    have to indicate access via this structure, nor do we have to be
 *    concerned about relocation of these structures.
 *
 *    (Supporting relocation in icclib would require significant changes
 *    to the library, which would complicate any future upgrades. To
 *    support garbage collection would, at a minimum, require a modified
 *    set of operands for the allocation procedure used by the library,
 *    which also seems ill advised.)
 *
 *    Because the input stream structure is re-locatable, it is necessary
 *    to update the stream pointer held in the profile data structure
 *    prior to any call to the module. We retain a pointer to the stream
 *    in the gs_cie_icc_s structure, for which a structure descriptor is
 *    provided, and use this to update the value held by the icc library.
 *    The method update_file_ptr has been added to the _icc structure to
 *    facilitate this process.
 *
 *    In principle, a user should not close the stream object included
 *    in a color space until that space is no longer needed. But there is
 *    no language-level mechanism to detect/prevent such activities, so
 *    we must deal with that possibility here. The method employed
 *    mimics that used with PostScript file objects. The file_id field is
 *    initialized to the value of (instrp->read_id | instrp->write_id),
 *    and is compared with this value prior to any call. If the values are
 *    no longer the same, the stream has been closed and a suitable
 *    error is generated.
 */
struct gs_cie_icc_s {
    gs_cie_common_elements;

    /* number of components, and their associated range */
    uint                num_components;
    gs_range4           Range;

    /* stream object, and the associated read id */
    unsigned short      file_id;
    stream *            instrp;

    /* the following are set when the structure is initialized */

    /* must the profile connection space undergo an L*a*b* ==> XYZ conversion */
    bool                pcs_is_cielab;

    /* top-level icclib data structure for the profile */
    struct _icc *       picc;

    /* "lookup" data structure in the ICC profile */
    struct _icmLuBase * plu;

    /* icclib file object for ICC stream */
    struct _icmFile   * pfile;
};

/*
 * The gs_cie_icc_s structure is responsible for foreign memory that must be
 * freed when the object is no longer accessible. It is the only CIE space
 * structure that contains such pointers. We make use of the finalization
 * procedure to handle this task.
 */
#define private_st_cie_icc()    /* in gscsicc.c */            \
    gs_private_st_suffix_add1_final( st_cie_icc,              \
                                     gs_cie_icc,              \
                                     "gs_cie_icc",            \
                                     cie_icc_enum_ptrs,       \
                                     cie_icc_reloc_ptrs,      \
                                     cie_icc_finalize,        \
                                     st_cie_common_elements_t,\
                                     instrp )

/* typedef struct gs_cie_icc_s gs_cie_icc; */   /* in gscspace.h */


/* 
 * Build an ICCBased color space.
 *
 * As with all of the CIE base color space constructurs, this will build
 * an gs_cie_icc_s structure with a reference count of 1 (not 0). If the
 * color space is passed to gs_setcolorspace, that procedure  will increment
 * the reference count again. To prevent the color space from being allocated
 * permanently, the client should call cs_adjust_count(pcspace, -1).
 * THIS IS A BUG IN THE API.
 *
 * The client is responsible for initializing the alternative color space
 * information.
 */
extern  int     gs_cspace_build_CIEICC( gs_color_space **   ppcspace,
					void *              client_data,
					gs_memory_t *       pmem );

int
gx_load_icc_profile(gs_cie_icc *picc_info);

/*
 * Increment color space reference counts.
 */
void
gx_increment_cspace_count(const gs_color_space * pcs);

#endif /* gsicc_INCLUDED */
