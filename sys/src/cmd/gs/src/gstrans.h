/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gstrans.h,v 1.15 2005/08/30 17:08:55 igor Exp $ */
/* Transparency definitions and interface */

#ifndef gstrans_INCLUDED
#  define gstrans_INCLUDED

#include "gstparam.h"
#include "gxcomp.h"

/*
 * Define the operations for the PDF 1.4 transparency compositor.
 */
typedef enum {
    PDF14_PUSH_DEVICE,
    PDF14_POP_DEVICE,
    PDF14_BEGIN_TRANS_GROUP,
    PDF14_END_TRANS_GROUP,
    PDF14_INIT_TRANS_MASK,
    PDF14_BEGIN_TRANS_MASK,
    PDF14_END_TRANS_MASK,
    PDF14_SET_BLEND_PARAMS
} pdf14_compositor_operations;

#define PDF14_OPCODE_NAMES \
{\
    "PDF14_PUSH_DEVICE      ",\
    "PDF14_POP_DEVICE       ",\
    "PDF14_BEGIN_TRANS_GROUP",\
    "PDF14_END_TRANS_GROUP  ",\
    "PDF14_INIT_TRANS_MASK  ",\
    "PDF14_BEGIN_TRANS_MASK ",\
    "PDF14_END_TRANS_MASK   ",\
    "PDF14_SET_BLEND_PARAMS "\
}

/* Bit definitions for serializing PDF 1.4 parameters */
#define PDF14_SET_BLEND_MODE    (1 << 0)
#define PDF14_SET_TEXT_KNOCKOUT (1 << 1)
#define PDF14_SET_SHAPE_ALPHA   (1 << 2)
#define PDF14_SET_OPACITY_ALPHA (1 << 3)

#ifndef gs_function_DEFINED
typedef struct gs_function_s gs_function_t;
#  define gs_function_DEFINED
#endif

typedef struct gs_transparency_source_s {
    float alpha;		/* constant alpha */
    gs_transparency_mask_t *mask;
} gs_transparency_source_t;

struct gs_pdf14trans_params_s {
    /* The type of trasnparency operation */
    pdf14_compositor_operations pdf14_op;
    /* Changed parameters flag */
    int changed;
    /* Parameters from the gs_transparency_group_params_t structure */
    bool Isolated;
    bool Knockout;
    gs_rect bbox;
    /*The transparency channel selector */
    gs_transparency_channel_selector_t csel;
    /* Parameters from the gx_transparency_mask_params_t structure */
    gs_transparency_mask_subtype_t subtype;
    int Background_components;
    bool function_is_identity;
    float Background[GS_CLIENT_COLOR_MAX_COMPONENTS];
    float GrayBackground;
    gs_function_t *transfer_function;
    byte transfer_fn[MASK_TRANSFER_FUNCTION_SIZE];
    /* Individual transparency parameters */
    gs_blend_mode_t blend_mode;
    bool text_knockout;
    gs_transparency_source_t opacity;
    gs_transparency_source_t shape;
    bool mask_is_image;
};

typedef struct gs_pdf14trans_params_s gs_pdf14trans_params_t;

/*
 * The PDF 1.4 transparency compositor structure. This is exactly analogous to
 * other compositor structures, consisting of a the compositor common elements
 * and the PDF 1.4 transparency specific parameters.
 */
typedef struct gs_pdf14trans_s {
    gs_composite_common;
    gs_pdf14trans_params_t  params;
} gs_pdf14trans_t;


/* Access transparency-related graphics state elements. */
int gs_setblendmode(gs_state *, gs_blend_mode_t);
gs_blend_mode_t gs_currentblendmode(const gs_state *);
int gs_setopacityalpha(gs_state *, floatp);
float gs_currentopacityalpha(const gs_state *);
int gs_setshapealpha(gs_state *, floatp);
float gs_currentshapealpha(const gs_state *);
int gs_settextknockout(gs_state *, bool);
bool gs_currenttextknockout(const gs_state *);

/*
 * Manage transparency group and mask rendering.  Eventually these will be
 * driver procedures, taking dev + pis instead of pgs.
 */

gs_transparency_state_type_t
    gs_current_transparency_type(const gs_state *pgs);

/*
 * We have to abbreviate the procedure name because procedure names are
 * only unique to 23 characters on VMS.
 */
int gs_push_pdf14trans_device(gs_state * pgs);

int gs_pop_pdf14trans_device(gs_state * pgs);

void gs_trans_group_params_init(gs_transparency_group_params_t *ptgp);

int gs_begin_transparency_group(gs_state * pgs,
				const gs_transparency_group_params_t *ptgp,
				const gs_rect *pbbox);

int gs_end_transparency_group(gs_state *pgs);

void gs_trans_mask_params_init(gs_transparency_mask_params_t *ptmp,
			       gs_transparency_mask_subtype_t subtype);

int gs_begin_transparency_mask(gs_state *pgs,
			       const gs_transparency_mask_params_t *ptmp,
			       const gs_rect *pbbox, bool mask_is_image);

int gs_end_transparency_mask(gs_state *pgs,
			     gs_transparency_channel_selector_t csel);

int gs_init_transparency_mask(gs_state *pgs,
			      gs_transparency_channel_selector_t csel);

int gs_discard_transparency_layer(gs_state *pgs);

/*
 * Imager level routines for the PDF 1.4 transparency operations.
 */
int gx_begin_transparency_group(gs_imager_state * pis, gx_device * pdev,
				const gs_pdf14trans_params_t * pparams);

int gx_end_transparency_group(gs_imager_state * pis, gx_device * pdev);

int gx_init_transparency_mask(gs_imager_state * pis,
				const gs_pdf14trans_params_t * pparams);

int gx_begin_transparency_mask(gs_imager_state * pis, gx_device * pdev,
				const gs_pdf14trans_params_t * pparams);

int gx_end_transparency_mask(gs_imager_state * pis, gx_device * pdev,
				const gs_pdf14trans_params_t * pparams);

int gx_discard_transparency_layer(gs_imager_state *pis);

/*
 * Verify that a compositor data structure is for the PDF 1.4 compositor.
 */
int gs_is_pdf14trans_compositor(const gs_composite_t * pct);

/*
 * Estimate the amount of space that will be required by the PDF 1.4
 * transparency buffers for doing the blending operations.  These buffers
 * use 8 bits per component plus one or two 8 bit alpha component values.
 * In theory there can be a large number of these buffers required.  However
 * we do not know the required number of buffers, the required numbe of
 * alpha chanels, or the number of components for the blending operations.
 * (This information is determined later as the data streams are parsed.)
 * For now we are simply assuming that we will have three buffers with five
 * eight bit values.  This is a hack but not too unreasonable.  However
 * since it is a hack, we may exceed our desired buffer space while
 * processing the file.
 */
#define NUM_PDF14_BUFFERS 3
#define NUM_ALPHA_CHANNELS 1
#define NUM_COLOR_CHANNELS 4
#define BITS_PER_CHANNEL 8
/* The estimated size of an individual PDF 1.4 buffer row (in bits) */
#define ESTIMATED_PDF14_ROW_SIZE(width) ((width) * BITS_PER_CHANNEL\
	* (NUM_ALPHA_CHANNELS + NUM_COLOR_CHANNELS))
/* The estimated size of one row in all PDF 1.4 buffers (in bits) */
#define ESTIMATED_PDF14_ROW_SPACE(width) \
	(NUM_PDF14_BUFFERS * ESTIMATED_PDF14_ROW_SIZE(width))

#endif /* gstrans_INCLUDED */
