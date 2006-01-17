/*
  Copyright (C) 2001 artofcode LLC.
  
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

  Author: Raph Levien <raph@artofcode.com>
*/
/* $Id: gxblend.h,v 1.3 2002/02/21 22:24:53 giles Exp $ */
/* PDF 1.4 blending functions */

#ifndef gxblend_INCLUDED
#  define gxblend_INCLUDED

typedef bits16 ArtPixMaxDepth;

#define ART_MAX_CHAN 16

/**
 * art_blend_pixel: Compute PDF 1.4 blending function.
 * @dst: Where to store resulting pixel.
 * @backdrop: Backdrop pixel color.
 * @src: Source pixel color.
 * @n_chan: Number of channels.
 * @blend_mode: Blend mode.
 *
 * Computes the blend of two pixels according the PDF 1.4 transparency
 * spec (section 3.2, Blend Mode). A few things to keep in mind about
 * this implementation:
 *
 * 1. This is a reference implementation, not a high-performance one.
 * Blending using this function will incur a function call and switch
 * statement per pixel, and will also incur the extra cost of 16 bit
 * math.
 *
 * 2. Zero is black, one is white. In a subtractive color space such
 * as CMYK, all pixels should be represented as "complemented", as
 * described in section 3.1 (Blending Color Space) of the PDF 1.4
 * transparency spec.
 *
 * 3. I haven't really figured out how to handle the Compatible blend
 * mode. I wouldn't be surprised if it required an API change.
 **/
void
art_blend_pixel(ArtPixMaxDepth * dst, const ArtPixMaxDepth * backdrop,
		const ArtPixMaxDepth * src, int n_chan,
		gs_blend_mode_t blend_mode);

/**
 * art_blend_pixel_8: Compute PDF 1.4 blending function on 8-bit pixels.
 * @dst: Where to store resulting pixel.
 * @backdrop: Backdrop pixel color.
 * @src: Source pixel color.
 * @n_chan: Number of channels.
 * @blend_mode: Blend mode.
 *
 * Computes the blend of two pixels according the PDF 1.4 transparency
 * spec (section 3.2, Blend Mode). A few things to keep in mind about
 * this implementation:
 *
 * 1. This is a reference implementation, not a high-performance one.
 * Blending using this function will incur a function call and switch
 * statement per pixel, and will also incur the extra cost of 16 bit
 * math.
 *
 * 2. Zero is black, one is white. In a subtractive color space such
 * as CMYK, all pixels should be represented as "complemented", as
 * described in section 3.1 (Blending Color Space) of the PDF 1.4
 * transparency spec.
 *
 * 3. I haven't really figured out how to handle the Compatible blend
 * mode. I wouldn't be surprised if it required an API change.
 **/
void
art_blend_pixel_8(byte *dst, const byte *backdrop,
		  const byte *src, int n_chan, gs_blend_mode_t blend_mode);

/**
 * art_pdf_union_8: Union together two alpha values.
 * @alpha1: One alpha value.
 * @alpha2: Another alpha value.
 *
 * Return value: Union (@alpha1, @alpha2).
 **/
byte art_pdf_union_8(byte alpha1, byte alpha2);

/**
 * art_pdf_union_mul_8: Union together two alpha values, with mask.
 * @alpha1: One alpha value.
 * @alpha2: Another alpha value.
 * @alpha_mask: A mask alpha value;
 *
 * Return value: Union (@alpha1, @alpha2 * @alpha_mask).
 **/
byte art_pdf_union_mul_8(byte alpha1, byte alpha2, byte alpha_mask);

/**
 * art_pdf_composite_pixel_alpha_8: Composite two alpha pixels.
 * @dst: Where to store resulting pixel, also initially backdrop color.
 * @src: Source pixel color.
 * @n_chan: Number of channels.
 * @blend_mode: Blend mode.
 *
 * Composites two pixels using the basic compositing operation. A few
 * things to keep in mind:
 *
 * 1. This is a reference implementation, not a high-performance one.
 *
 * 2. All pixels are assumed to have a single alpha channel.
 *
 * 3. Zero is black, one is white.
 *
 * Also note that src and dst are expected to be allocated aligned to
 * 32 bit boundaries, ie bytes from [0] to [(n_chan + 3) & -4] may
 * be accessed.
 **/
void
art_pdf_composite_pixel_alpha_8(byte *dst, const byte *src, int n_chan,

				gs_blend_mode_t blend_mode);

/**
 * art_pdf_uncomposite_group_8: Uncomposite group pixel.
 * @dst: Where to store uncomposited pixel.
 * @backdrop: Backdrop.
 * @src: Composited source pixel.
 * @src_alpha_g: alpha_g value associated with @src.
 * @n_chan: Number of channels.
 *
 * Performs uncompositing operation as described in 5.3 of the Adobe spec.
 **/
void
art_pdf_uncomposite_group_8(byte *dst,
			    const byte *backdrop,

			    const byte *src, byte src_alpha_g, int n_chan);

/**
 * art_pdf_recomposite_group_8: Recomposite group pixel.
 * @dst: Where to store pixel, also initial backdrop of group.
 * @dst_alpha_g: Optional pointer to alpha g value associated with @dst.
 * @alpha: Alpha mask value.
 * @src_alpha_g: alpha_g value associated with @src.
 * @blend_mode: Blend mode for compositing.
 *
 * Note: this is only for non-isolated groups. This covers only the
 * single-alpha case. A separate function is needed for dual-alpha,
 * and that probably needs to treat knockout separately.
 *
 * @src_alpha_g corresponds to $\alpha g_n$ in the Adobe notation.
 *
 * @alpha corresponds to $fk_i \cdot fm_i \cdot qk_i \cdot qm_i$.
 **/
void
art_pdf_recomposite_group_8(byte *dst, byte *dst_alpha_g,
			    const byte *src, byte src_alpha_g,
			    int n_chan,

			    byte alpha, gs_blend_mode_t blend_mode);

/**
 * art_pdf_composite_group_8: Composite group pixel.
 * @dst: Where to store pixel, also initial backdrop of group.
 * @dst_alpha_g: Optional pointer to alpha g value.
 * @alpha: Alpha mask value.
 *
 * Note: this is only for isolated groups. This covers only the
 * single-alpha case. A separate function is needed for dual-alpha,
 * and that probably needs to treat knockout separately.
 *
 * @alpha corresponds to $fk_i \cdot fm_i \cdot qk_i \cdot qm_i$.
 **/
void
art_pdf_composite_group_8(byte *dst, byte *alpha_g,
			  const byte *src,

			  int n_chan, byte alpha, gs_blend_mode_t blend_mode);

/**
 * art_pdf_composite_knockout_simple_8: Simple knockout compositing.
 * @dst: Destination pixel. 
 * @dst_shape: Shape associated with @dst.
 * @src: Source pixel.
 * @n_chan: Number of channels.
 * @opacity: Opacity.
 *
 * This function handles the simplest knockout case: an isolated
 * knockout group, and an elementary shape. The alpha channel of @src
 * is interpreted as shape.
 **/
void
art_pdf_composite_knockout_simple_8(byte *dst,
				    byte *dst_shape,

				    const byte *src,
				    int n_chan, byte opacity);

/**
 * art_pdf_composite_knockout_isolated_8: Simple knockout compositing.
 * @dst: Destination pixel. 
 * @dst_shape: Shape associated with @dst.
 * @src: Source pixel.
 * @n_chan: Number of channels.
 * @shape: Shape.
 * @alpha_mask: Alpha mask.
 * @shape_mask: Shape mask.
 *
 * This function handles compositin in an isolated knockout case. The
 * alpha channel of @src is interpreted as alpha.
 **/
void
art_pdf_composite_knockout_isolated_8(byte *dst,
				      byte *dst_shape,
				      const byte *src,
				      int n_chan,
				      byte shape,
				      byte alpha_mask, byte shape_mask);

/**
 * art_pdf_composite_knockout_8: General knockout compositing.
 * @dst: Destination pixel. 
 * @dst_alpha_g: Pointer to alpha g value associated with @dst.
 * @backdrop: Backdrop pixel (initial backdrop of knockout group).
 * @src: Source pixel.
 * @n_chan: Number of channels.
 * @shape: Shape.
 * @alpha_mask: Alpha mask.
 * @shape_mask: Shape mask.
 * @blend_mode: Blend mode for compositing.
 *
 * This function handles compositing in the case where the knockout
 * group is non-isolated. If the @src pixels themselves come from a
 * non-isolated group, they should be uncomposited before calling this
 * routine.
 **/
void
art_pdf_composite_knockout_8(byte *dst,
			     byte *dst_alpha_g,
			     const byte *backdrop,
			     const byte *src,
			     int n_chan,
			     byte shape,

			     byte alpha_mask,
			     byte shape_mask, gs_blend_mode_t blend_mode);

#endif /* gxblend_INCLUDED */
