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

/* $Id: gdevpdts.c,v 1.28 2005/01/24 15:37:48 igor Exp $ */
/* Text state management for pdfwrite */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gdevpdfx.h"
#include "gdevpdtx.h"
#include "gdevpdtf.h"		/* for pdfont->FontType */
#include "gdevpdts.h"

/* ================ Types and structures ================ */

/*
 * We accumulate text, and possibly horizontal or vertical moves (depending
 * on the font's writing direction), until forced to emit them.  This
 * happens when changing text state parameters, when the buffer is full, or
 * when exiting text mode.
 *
 * Note that movement distances are measured in unscaled text space.
 */
typedef struct pdf_text_move_s {
    int index;			/* within buffer.chars */
    float amount;
} pdf_text_move_t;
#define MAX_TEXT_BUFFER_CHARS 200 /* arbitrary, but overflow costs 5 chars */
#define MAX_TEXT_BUFFER_MOVES 50 /* ibid. */
typedef struct pdf_text_buffer_s {
    /*
     * Invariant:
     *   count_moves <= MAX_TEXT_BUFFER_MOVES
     *   count_chars <= MAX_TEXT_BUFFER_CHARS
     *   0 < moves[0].index < moves[1].index < ... moves[count_moves-1].index
     *	   <= count_chars
     *   moves[*].amount != 0
     */
    pdf_text_move_t moves[MAX_TEXT_BUFFER_MOVES + 1];
    byte chars[MAX_TEXT_BUFFER_CHARS];
    int count_moves;
    int count_chars;
} pdf_text_buffer_t;
#define TEXT_BUFFER_DEFAULT\
    { { 0, 0 } },		/* moves */\
    { 0 },			/* chars */\
    0,				/* count_moves */\
    0				/* count_chars */

/*
 * We maintain two sets of text state values (as defined in gdevpdts.h): the
 * "in" set reflects the current state as seen by the client, while the
 * "out" set reflects the current state as seen by an interpreter processing
 * the content stream emitted so far.  We emit commands to make "out" the
 * same as "in" when necessary.
 */
/*typedef struct pdf_text_state_s pdf_text_state_t;*/  /* gdevpdts.h */
struct pdf_text_state_s {
    /* State as seen by client */
    pdf_text_state_values_t in; /* see above */
    gs_point start;		/* in.txy as of start of buffer */
    pdf_text_buffer_t buffer;
    int wmode;			/* WMode of in.font */
    /* State relative to content stream */
    pdf_text_state_values_t out; /* see above */
    double leading;		/* TL (not settable, only used internally) */
    bool use_leading;		/* if true, use T* or ' */
    bool continue_line;
    gs_point line_start;
    gs_point out_pos;		/* output position */
};
private const pdf_text_state_t ts_default = {
    /* State as seen by client */
    { TEXT_STATE_VALUES_DEFAULT },	/* in */
    { 0, 0 },			/* start */
    { TEXT_BUFFER_DEFAULT },	/* buffer */
    0,				/* wmode */
    /* State relative to content stream */
    { TEXT_STATE_VALUES_DEFAULT },	/* out */
    0,				/* leading */
    0 /*false*/,		/* use_leading */
    0 /*false*/,		/* continue_line */
    { 0, 0 },			/* line_start */
    { 0, 0 }			/* output position */
};
/* GC descriptor */
gs_private_st_ptrs2(st_pdf_text_state, pdf_text_state_t,  "pdf_text_state_t",
		    pdf_text_state_enum_ptrs, pdf_text_state_reloc_ptrs,
		    in.pdfont, out.pdfont);

/* ================ Procedures ================ */

/* ---------------- Private ---------------- */

/*
 * Append a writing-direction movement to the text being accumulated.  If
 * the buffer is full, or the requested movement is not in writing
 * direction, return <0 and do nothing.  (This is different from
 * pdf_append_chars.)  Requires pts->buffer.count_chars > 0.
 */
private int
append_text_move(pdf_text_state_t *pts, floatp dw)
{
    int count = pts->buffer.count_moves;
    int pos = pts->buffer.count_chars;
    double rounded;

    if (count > 0 && pts->buffer.moves[count - 1].index == pos) {
	/* Merge adjacent moves. */
	dw += pts->buffer.moves[--count].amount;
    }
    /* Round dw if it's very close to an integer. */
    rounded = floor(dw + 0.5);
    if (fabs(dw - rounded) < 0.001)
	dw = rounded;
    if (dw < -MAX_USER_COORD) {
	/* Acrobat reader 4.0c, 5.0 can't handle big offsets. 
	   Adobe Reader 6 can. */
	return -1;
    }
    if (dw != 0) {
	if (count == MAX_TEXT_BUFFER_MOVES)
	    return -1;
	pts->buffer.moves[count].index = pos;
	pts->buffer.moves[count].amount = dw;
	++count;
    }
    pts->buffer.count_moves = count;
    return 0;
}

/*
 * Set *pdist to the distance (dx,dy), in the space defined by *pmat.
 */
private int
set_text_distance(gs_point *pdist, floatp dx, floatp dy, const gs_matrix *pmat)
{
    int code = gs_distance_transform_inverse(dx, dy, pmat, pdist);
    double rounded;

    if (code < 0)
	return code;
    /* If the distance is very close to integers, round it. */
    if (fabs(pdist->x - (rounded = floor(pdist->x + 0.5))) < 0.0005)
	pdist->x = rounded;
    if (fabs(pdist->y - (rounded = floor(pdist->y + 0.5))) < 0.0005)
	pdist->y = rounded;
    return 0;
}

/*
 * Test whether the transformation parts of two matrices are compatible.
 */
private bool
matrix_is_compatible(const gs_matrix *pmat1, const gs_matrix *pmat2)
{
    return (pmat2->xx == pmat1->xx && pmat2->xy == pmat1->xy &&
	    pmat2->yx == pmat1->yx && pmat2->yy == pmat1->yy);
}

/*
 * Try to handle a change of text position with TJ or a space
 * character.  If successful, return >=0, if not, return <0.
 */
private int
add_text_delta_move(gx_device_pdf *pdev, const gs_matrix *pmat)
{
    pdf_text_state_t *const pts = pdev->text->text_state;

    if (matrix_is_compatible(pmat, &pts->in.matrix)) {
	double dx = pmat->tx - pts->in.matrix.tx,
	    dy = pmat->ty - pts->in.matrix.ty;
	gs_point dist;
	double dw, dnotw, tdw;
	int code;

	code = set_text_distance(&dist, dx, dy, pmat);
	if (code < 0)
	    return code;
	if (pts->wmode)
	    dw = dist.y, dnotw = dist.x;
	else
	    dw = dist.x, dnotw = dist.y;
	if (dnotw == 0 && pts->buffer.count_chars > 0 &&
	    /*
	     * Acrobat Reader limits the magnitude of user-space
	     * coordinates.  Also, AR apparently doesn't handle large
	     * positive movement values (negative X displacements), even
	     * though the PDF Reference says this bug was fixed in AR3.
	     *
	     * Old revisions used the upper threshold 1000 for tdw,
	     * but it appears too big when a font sets a too big
	     * character width in setcachedevice. Particularly this happens 
	     * with a Type 3 font generated by Aldus Freehand 4.0
	     * to represent a texture - see bug #687051.
	     * The problem is that when the Widths is multiplied 
	     * to the font size, the viewer represents the result
	     * with insufficient fraction bits to represent the precise width.
	     * We work around that problem here restricting tdw
	     * with a smaller threshold 990. Our intention is to 
	     * disable Tj when the real glyph width appears smaller
	     * than 1% of the width specified in setcachedevice.
	     * A Td instruction will be generated instead.
	     * Note that the value 990 is arbitrary and may need a
	     * further adjustment.
	     */
	    (tdw = dw * -1000.0 / pts->in.size,
	     tdw >= -MAX_USER_COORD && tdw < 990)
	    ) {
	    /* Use TJ. */
	    int code = append_text_move(pts, tdw);

	    if (code >= 0)
		goto finish;
	}
    }
    return -1;
 finish:
    pts->in.matrix = *pmat;
    return 0;
}

/*
 * Set the text matrix for writing text.  The translation component of the
 * matrix is the text origin.  If the non-translation components of the
 * matrix differ from the current ones, write a Tm command; if there is only
 * a Y translation, set use_leading so the next text string will be written
 * with ' rather than Tj; otherwise, write a Td command.
 */
private int
pdf_set_text_matrix(gx_device_pdf * pdev)
{
    pdf_text_state_t *pts = pdev->text->text_state;
    stream *s = pdev->strm;

    pts->use_leading = false;
    if (matrix_is_compatible(&pts->out.matrix, &pts->in.matrix)) {
	gs_point dist;
	int code;

	code = set_text_distance(&dist, pts->start.x - pts->line_start.x,
			  pts->start.y - pts->line_start.y, &pts->in.matrix);
	if (code < 0)
	    return code;
	if (dist.x == 0 && dist.y < 0) {
	    /* Use TL, if needed, and T* or '. */
	    float dist_y = (float)-dist.y;

	    if (fabs(pts->leading - dist_y) > 0.0005) {
		pprintg1(s, "%g TL\n", dist_y);
		pts->leading = dist_y;
	    }
	    pts->use_leading = true;
	} else {
	    /* Use Td. */
	    pprintg2(s, "%g %g Td\n", dist.x, dist.y);
	}
    } else {			/* Use Tm. */
	/*
	 * See stream_to_text in gdevpdfu.c for why we need the following
	 * matrix adjustments.
	 */
	double sx = 72.0 / pdev->HWResolution[0],
	    sy = 72.0 / pdev->HWResolution[1];

	pprintg6(s, "%g %g %g %g %g %g Tm\n",
		 pts->in.matrix.xx * sx, pts->in.matrix.xy * sy,
		 pts->in.matrix.yx * sx, pts->in.matrix.yy * sy,
		 pts->start.x * sx, pts->start.y * sy);
    }
    pts->line_start.x = pts->start.x;
    pts->line_start.y = pts->start.y;
    pts->out.matrix = pts->in.matrix;
    return 0;
}

/* ---------------- Public ---------------- */

/*
 * Allocate and initialize text state bookkeeping.
 */
pdf_text_state_t *
pdf_text_state_alloc(gs_memory_t *mem)
{
    pdf_text_state_t *pts =
	gs_alloc_struct(mem, pdf_text_state_t, &st_pdf_text_state,
			"pdf_text_state_alloc");

    if (pts == 0)
	return 0;
    *pts = ts_default;
    return pts;
}

/*
 * Set the text state to default values.
 */
void
pdf_set_text_state_default(pdf_text_state_t *pts)
{
    *pts = ts_default;
}

/*
 * Copy the text state.
 */
void
pdf_text_state_copy(pdf_text_state_t *pts_to, pdf_text_state_t *pts_from)
{
    *pts_to = *pts_from;
}

/*
 * Reset the text state to its condition at the beginning of the page.
 */
void
pdf_reset_text_page(pdf_text_data_t *ptd)
{
    pdf_set_text_state_default(ptd->text_state);
}

/*
 * Reset the text state after a grestore.
 */
void
pdf_reset_text_state(pdf_text_data_t *ptd)
{
    pdf_text_state_t *pts = ptd->text_state;

    pts->out = ts_default.out;
    pts->leading = 0;
}

/*
 * Transition from stream context to text context.
 */
int
pdf_from_stream_to_text(gx_device_pdf *pdev)
{
    pdf_text_state_t *pts = pdev->text->text_state;

    gs_make_identity(&pts->out.matrix);
    pts->line_start.x = pts->line_start.y = 0;
    pts->continue_line = false; /* Not sure, probably doesn't matter. */
    pts->buffer.count_chars = 0;
    pts->buffer.count_moves = 0;
    return 0;
}


/*
 *  Flush text from buffer.
 */
private int
flush_text_buffer(gx_device_pdf *pdev)
{
    pdf_text_state_t *pts = pdev->text->text_state;
    stream *s = pdev->strm;

    if (pts->buffer.count_moves > 0) {
	int i, cur = 0;

	if (pts->use_leading)
	    stream_puts(s, "T*");
	stream_puts(s, "[");
	for (i = 0; i < pts->buffer.count_moves; ++i) {
	    int next = pts->buffer.moves[i].index;

	    pdf_put_string(pdev, pts->buffer.chars + cur, next - cur);
	    pprintg1(s, "%g", pts->buffer.moves[i].amount);
	    cur = next;
	}
	if (pts->buffer.count_chars > cur)
	    pdf_put_string(pdev, pts->buffer.chars + cur,
			   pts->buffer.count_chars - cur);
	stream_puts(s, "]TJ\n");
    } else {
	pdf_put_string(pdev, pts->buffer.chars, pts->buffer.count_chars);
	stream_puts(s, (pts->use_leading ? "'\n" : "Tj\n"));
    }
    pts->buffer.count_chars = 0;
    pts->buffer.count_moves = 0;
    pts->use_leading = false;
    return 0;
}

/*
 * Transition from string context to text context.
 */
private int
sync_text_state(gx_device_pdf *pdev)
{
    pdf_text_state_t *pts = pdev->text->text_state;
    stream *s = pdev->strm;
    int code;

    if (pts->buffer.count_chars == 0)
	return 0;		/* nothing to output */

    if (pts->continue_line) 
	return flush_text_buffer(pdev);

    /* Bring text state parameters up to date. */

    if (pts->out.character_spacing != pts->in.character_spacing) {
	pprintg1(s, "%g Tc\n", pts->in.character_spacing);
	pts->out.character_spacing = pts->in.character_spacing;
    }

    if (pts->out.pdfont != pts->in.pdfont || pts->out.size != pts->in.size) {
	pdf_font_resource_t *pdfont = pts->in.pdfont;

	pprints1(s, "/%s ", ((pdf_resource_t *)pts->in.pdfont)->rname);
	pprintg1(s, "%g Tf\n", pts->in.size);
	pts->out.pdfont = pdfont;
	pts->out.size = pts->in.size;
	/*
	 * In PDF, the only place to specify WMode is in the CMap
	 * (a.k.a. Encoding) of a Type 0 font.
	 */
	pts->wmode =
	    (pdfont->FontType == ft_composite ?
	     pdfont->u.type0.WMode : 0);
	code = pdf_used_charproc_resources(pdev, pdfont);
	if (code < 0)
	    return code;
    }

    if (memcmp(&pts->in.matrix, &pts->out.matrix, sizeof(pts->in.matrix)) ||
	 ((pts->start.x != pts->out_pos.x || pts->start.y != pts->out_pos.y) &&
	  (pts->buffer.count_chars != 0 || pts->buffer.count_moves != 0))) {
	/* pdf_set_text_matrix sets out.matrix = in.matrix */
	code = pdf_set_text_matrix(pdev);
	if (code < 0)
	    return code;
    }

    if (pts->out.render_mode != pts->in.render_mode) {
	pprintg1(s, "%g Tr\n", pts->in.render_mode);
	pts->out.render_mode = pts->in.render_mode;
    }

    if (pts->out.word_spacing != pts->in.word_spacing) {
	if (memchr(pts->buffer.chars, 32, pts->buffer.count_chars)) {
	    pprintg1(s, "%g Tw\n", pts->in.word_spacing);
	    pts->out.word_spacing = pts->in.word_spacing;
	}
    }

    return flush_text_buffer(pdev);
}

int
pdf_from_string_to_text(gx_device_pdf *pdev)
{
    return sync_text_state(pdev);
}

/*
 * Close the text aspect of the current contents part.
 */
void
pdf_close_text_contents(gx_device_pdf *pdev)
{
    /*
     * Clear the font pointer.  This is probably left over from old code,
     * but it is appropriate in case we ever choose in the future to write
     * out and free font resources before the end of the document.
     */
    pdf_text_state_t *pts = pdev->text->text_state;

    pts->in.pdfont = pts->out.pdfont = 0;
    pts->in.size = pts->out.size = 0;
}

/*
 * Test whether a change in render_mode requires resetting the stroke
 * parameters.
 */
bool
pdf_render_mode_uses_stroke(const gx_device_pdf *pdev,
			    const pdf_text_state_values_t *ptsv)
{
    return (pdev->text->text_state->in.render_mode != ptsv->render_mode &&
	    ptsv->render_mode != 0);
}

/*
 * Read the stored client view of text state values.
 */
void
pdf_get_text_state_values(gx_device_pdf *pdev, pdf_text_state_values_t *ptsv)
{
    *ptsv = pdev->text->text_state->in;
}

/*
 * Set wmode to text state.
 */
void
pdf_set_text_wmode(gx_device_pdf *pdev, int wmode)
{
    pdf_text_state_t *pts = pdev->text->text_state;

    pts->wmode = wmode;
}


/*
 * Set the stored client view of text state values.
 */
int
pdf_set_text_state_values(gx_device_pdf *pdev,
			  const pdf_text_state_values_t *ptsv)
{
    pdf_text_state_t *pts = pdev->text->text_state;

    if (pts->buffer.count_chars > 0) {
	int code;

	if (pts->in.character_spacing == ptsv->character_spacing &&
	    pts->in.pdfont == ptsv->pdfont && pts->in.size == ptsv->size &&
	    pts->in.render_mode == ptsv->render_mode &&
	    pts->in.word_spacing == ptsv->word_spacing
	    ) {
	    if (!memcmp(&pts->in.matrix, &ptsv->matrix,
			sizeof(pts->in.matrix)))
		return 0;
	    /* add_text_delta_move sets pts->in.matrix if successful */
	    code = add_text_delta_move(pdev, &ptsv->matrix);
	    if (code >= 0)
		return 0;
	}
	code = sync_text_state(pdev);
	if (code < 0)
	    return code;
    }

    pts->in = *ptsv;
    pts->continue_line = false;
    return 0;
}

/*
 * Transform a distance from unscaled text space (text space ignoring the
 * scaling implied by the font size) to device space.
 */
int
pdf_text_distance_transform(floatp wx, floatp wy, const pdf_text_state_t *pts,
			    gs_point *ppt)
{
    return gs_distance_transform(wx, wy, &pts->in.matrix, ppt);
}

/*
 * Return the current (x,y) text position as seen by the client, in
 * unscaled text space.
 */
void
pdf_text_position(const gx_device_pdf *pdev, gs_point *ppt)
{
    pdf_text_state_t *pts = pdev->text->text_state;

    ppt->x = pts->in.matrix.tx;
    ppt->y = pts->in.matrix.ty;
}

/*
 * Append characters to text being accumulated, giving their advance width
 * in device space.
 */
int
pdf_append_chars(gx_device_pdf * pdev, const byte * str, uint size,
		 floatp wx, floatp wy, bool nobreak)
{
    pdf_text_state_t *pts = pdev->text->text_state;
    const byte *p = str;
    uint left = size;

    if (pts->buffer.count_chars == 0 && pts->buffer.count_moves == 0) {
	pts->out_pos.x = pts->start.x = pts->in.matrix.tx;
	pts->out_pos.y = pts->start.y = pts->in.matrix.ty;
    }
    while (left)
	if (pts->buffer.count_chars == MAX_TEXT_BUFFER_CHARS ||
	    (nobreak && pts->buffer.count_chars + left > MAX_TEXT_BUFFER_CHARS)) {
	    int code = sync_text_state(pdev);

	    if (code < 0)
		return code;
	    /* We'll keep a continuation of this line in the buffer,
	     * but the current input parameters don't correspond to
	     * the current position, because the text was broken in a
	     * middle with unknown current point.
	     * Don't change the output text state parameters
	     * until input parameters are changed. 
	     * pdf_set_text_state_values will reset the 'continue_line' flag 
	     * at that time.
	     */
	    pts->continue_line = true;
	} else {
	    int code = pdf_open_page(pdev, PDF_IN_STRING);
	    uint copy;

	    if (code < 0)
		return code;
	    copy = min(MAX_TEXT_BUFFER_CHARS - pts->buffer.count_chars, left);
	    memcpy(pts->buffer.chars + pts->buffer.count_chars, p, copy);
	    pts->buffer.count_chars += copy;
	    p += copy;
	    left -= copy;
	}
    pts->in.matrix.tx += wx;
    pts->in.matrix.ty += wy;
    pts->out_pos.x += wx;
    pts->out_pos.y += wy;
    return 0;
}
