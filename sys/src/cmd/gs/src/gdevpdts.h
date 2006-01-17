/* Copyright (C) 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdts.h,v 1.7 2003/04/08 16:31:56 igor Exp $ */
/* Text state structure and API for pdfwrite */

#ifndef gdevpdts_INCLUDED
#  define gdevpdts_INCLUDED

#include "gsmatrix.h"

/*
 * See gdevpdtt.h for a discussion of the multiple coordinate systems that
 * the text code must use.
 */

/* ================ Types and structures ================ */

#ifndef pdf_text_state_DEFINED
#  define pdf_text_state_DEFINED
typedef struct pdf_text_state_s pdf_text_state_t;
#endif

/*
 * Clients pass in the text state values; the implementation decides when
 * (and, in the case of text positioning, how) to emit PDF commands to
 * set them in the output.
 */
typedef struct pdf_text_state_values_s {
    float character_spacing;	/* Tc */
    pdf_font_resource_t *pdfont; /* for Tf */
    double size;		/* for Tf */
    /*
     * The matrix is the transformation from text space to user space, which
     * in pdfwrite text output is the same as device space.  Thus this
     * matrix combines the effect of the PostScript CTM and the FontMatrix,
     * scaled by the inverse of the font size value.
     */
    gs_matrix matrix;		/* Tm et al */
    int render_mode;		/* Tr */
    float word_spacing;		/* Tw */
} pdf_text_state_values_t;
#define TEXT_STATE_VALUES_DEFAULT\
    0,				/* character_spacing */\
    NULL,			/* font */\
    0,				/* size */\
    { identity_matrix_body },	/* matrix */\
    0,				/* render_mode */\
    0				/* word_spacing */

/* ================ Procedures ================ */

/* ------ Exported for gdevpdfu.c ------ */

/*
 * Transition from stream context to text context.
 */
int pdf_from_stream_to_text(gx_device_pdf *pdev);

/*
 * Transition from string context to text context.
 */
int pdf_from_string_to_text(gx_device_pdf *pdev);

/*
 * Close the text aspect of the current contents part.
 */
void pdf_close_text_contents(gx_device_pdf *pdev); /* gdevpdts.h */

/* ------ Used only within the text code ------ */

/*
 * Test whether a change in render_mode requires resetting the stroke
 * parameters.
 */
bool pdf_render_mode_uses_stroke(const gx_device_pdf *pdev,
				 const pdf_text_state_values_t *ptsv);

/*
 * Read the stored client view of text state values.
 */
void pdf_get_text_state_values(gx_device_pdf *pdev,
			       pdf_text_state_values_t *ptsv);

/*
 * Set wmode to text state.
 */
void pdf_set_text_wmode(gx_device_pdf *pdev, int wmode);


/*
 * Set the stored client view of text state values.
 */
int pdf_set_text_state_values(gx_device_pdf *pdev,
			      const pdf_text_state_values_t *ptsv);

/*
 * Transform a distance from unscaled text space (text space ignoring the
 * scaling implied by the font size) to device space.
 */
int pdf_text_distance_transform(floatp wx, floatp wy,
				const pdf_text_state_t *pts,
				gs_point *ppt);

/*
 * Return the current (x,y) text position as seen by the client, in
 * unscaled text space.
 */
void pdf_text_position(const gx_device_pdf *pdev, gs_point *ppt);

/*
 * Append characters to text being accumulated, giving their advance width
 * in device space.
 */
int pdf_append_chars(gx_device_pdf * pdev, const byte * str, uint size,
		     floatp wx, floatp wy, bool nobreak);

#endif /* gdevpdts_INCLUDED */
