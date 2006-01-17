/* Copyright (C) 2003-2004 artofcode LLC.  All rights reserved.
  
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

/* $Id: sjpx.c,v 1.12 2005/03/16 14:57:42 igor Exp $ */
/* JPXDecode filter implementation -- hooks in libjasper */

#include "memory_.h"
#ifdef JPX_DEBUG
#include "stdio_.h"
#endif

#include "gserrors.h"
#include "gserror.h"
#include "gdebug.h"
#include "strimpl.h"
#include "gsmalloc.h"
#include "sjpx.h"

/* stream implementation */

/* As with the /JBIG2Decode filter, we let the library do its own 
   memory management through malloc() etc. and rely on our release() 
   proc being called to deallocate state.
*/

private_st_jpxd_state(); /* creates a gc object for our state,
			    defined in sjpx.h */

/* initialize the steam.
   this involves allocating the stream and image structures, and
   initializing the decoder.
 */
private int
s_jpxd_init(stream_state * ss)
{
    stream_jpxd_state *const state = (stream_jpxd_state *) ss;
    int status = 0;

    state->buffer = NULL;
    state->bufsize = 0;
    state->buffill = 0;
    state->stream = NULL;
    state->image = NULL;
    state->offset = 0;
    state->jpx_memory = ss->memory ? ss->memory->non_gc_memory : gs_lib_ctx_get_non_gc_memory_t();
            
    status = jas_init();

    if (!status) {
	state->buffer = gs_malloc(state->jpx_memory, 4096, 1, "JPXDecode temp buffer");
        status = (state->buffer == NULL);
    }
    if (!status)
    	state->bufsize = 4096;

    return status;
}

#ifdef JPX_DEBUG
/* dump information from a jasper image struct for debugging */
private int
dump_jas_image(jas_image_t *image)
{
    int i, numcmpts = jas_image_numcmpts(image);
    int clrspc = jas_image_clrspc(image);
    const char *csname = "unrecognized vendor space";

    if (image == NULL) return 1;

    dprintf2("JPX image is %d x %d\n",
	jas_image_width(image), jas_image_height(image));

    /* sort the colorspace */
    if jas_clrspc_isunknown(clrspc) csname = "unknown";
    else switch (clrspc) {
	case JAS_CLRSPC_CIEXYZ: csname = "CIE XYZ"; break;
	case JAS_CLRSPC_CIELAB: csname = "CIE Lab"; break;
	case JAS_CLRSPC_SGRAY: csname = "calibrated grayscale"; break;
	case JAS_CLRSPC_SRGB: csname = "sRGB"; break;
	case JAS_CLRSPC_SYCBCR: csname = "calibrated YCbCr"; break;
	case JAS_CLRSPC_GENGRAY: csname = "generic gray"; break;
	case JAS_CLRSPC_GENRGB: csname = "generic RGB"; break;
	case JAS_CLRSPC_GENYCBCR: csname = "generic YCbCr"; break;
    }
    dprintf3("  colorspace is %s (family %d, member %d)\n", 
	csname, jas_clrspc_fam(clrspc), jas_clrspc_mbr(clrspc));

    for (i = 0; i < numcmpts; i++) {
	int type = jas_image_cmpttype(image, i);
	const char *opacity = (type & JAS_IMAGE_CT_OPACITY) ? " opacity" : "";
	const char *name = "unrecognized";
	const char *issigned = "";
	if (jas_clrspc_fam(clrspc) == JAS_CLRSPC_FAM_GRAY)
	    name = "gray";
	else if (jas_clrspc_fam(clrspc) == JAS_CLRSPC_FAM_RGB)
	    switch (JAS_IMAGE_CT_COLOR(type)) {
		case JAS_IMAGE_CT_RGB_R: name = "red"; break;
		case JAS_IMAGE_CT_RGB_G: name = "green"; break;
		case JAS_IMAGE_CT_RGB_B: name = "blue"; break;
		case JAS_IMAGE_CT_UNKNOWN:
		default:
		    name = "unknown";
	    }
	else if (jas_clrspc_fam(clrspc) == JAS_CLRSPC_FAM_YCBCR)
	    switch (JAS_IMAGE_CT_COLOR(type)) {
		case JAS_IMAGE_CT_YCBCR_Y: name = "luminance Y"; break;
		case JAS_IMAGE_CT_YCBCR_CB: name = "chrominance Cb"; break;
		case JAS_IMAGE_CT_YCBCR_CR: name = "chrominance Cr"; break;
		case JAS_IMAGE_CT_UNKNOWN:
		default:
		    name = "unknown";
	    }
	if (jas_image_cmptsgnd(image, i))
	    issigned = ", signed";
	dprintf6("  component %d: type %d '%s%s' (%d bits%s)",
	    i, type, name, opacity, jas_image_cmptprec(image, i), issigned);
	dprintf4(" grid step (%d,%d) offset (%d,%d)\n",
	    jas_image_cmpthstep(image, i), jas_image_cmptvstep(image, i),
	    jas_image_cmpttlx(image, i), jas_image_cmpttly(image, i));
    }

    return 0;
}
#endif /* JPX_DEBUG */

private int
copy_row_gray(unsigned char *dest, jas_image_t *image,
	int x, int y, int bytes)
{
    int i, p;
    int v = jas_image_getcmptbytype(image, JAS_IMAGE_CT_GRAY_Y);
    int shift = max(jas_image_cmptprec(image, v) - 8, 0);

    for (i = 1; i <= bytes; i++) {
	p = jas_image_readcmptsample(image, v, x++, y);
	dest[i] = p >> shift;
    }

    return bytes;
}

private int
copy_row_rgb(unsigned char *dest, jas_image_t *image,
	int x, int y, int bytes)
{
    int i, p;
    int r = jas_image_getcmptbytype(image, JAS_IMAGE_CT_RGB_R);
    int g = jas_image_getcmptbytype(image, JAS_IMAGE_CT_RGB_G);
    int b = jas_image_getcmptbytype(image, JAS_IMAGE_CT_RGB_B);
    int shift = max(jas_image_cmptprec(image, 0) - 8, 0);
    int count = (bytes/3) * 3;

    for (i = 1; i <= count; i+=3) {
	p = jas_image_readcmptsample(image, r, x, y);
	dest[i] = p >> shift;
	p = jas_image_readcmptsample(image, g, x, y);
	dest[i+1] = p >> shift;
	p = jas_image_readcmptsample(image, b, x, y);
	dest[i+2] = p >> shift;
	x++;
    }

    return count;
}

private int
copy_row_yuv(unsigned char *dest, jas_image_t *image,
	int x, int y, int bytes)
{
    int i,j;
    int count = (bytes/3) * 3;
    int shift[3];
    int clut[3];
    int hstep[3],vstep[3];
    int p[3],q[3];

    /* get the component mapping */
    clut[0] = jas_image_getcmptbytype(image, JAS_IMAGE_CT_YCBCR_Y);
    clut[1] = jas_image_getcmptbytype(image, JAS_IMAGE_CT_YCBCR_CB);
    clut[2] = jas_image_getcmptbytype(image, JAS_IMAGE_CT_YCBCR_CR);

    for (i = 0; i < 3; i++) {
	/* shift each component up to 16 bits */
	shift[i] = 16 - jas_image_cmptprec(image, clut[i]);
	/* repeat subsampled pixels */
	hstep[i] = jas_image_cmpthstep(image, clut[i]);
	vstep[i] = jas_image_cmptvstep(image, clut[i]);
    }
    for (i = 1; i <= count; i+=3) {
	/* read the sample values */
	for (j = 0; j < 3; j++) {
	    p[j] = jas_image_readcmptsample(image, clut[j], x/hstep[j], y/vstep[j]);
	    p[j] <<= shift[j];
	}
	/* center chroma channels if necessary */
	if (!jas_image_cmptsgnd(image, clut[1])) p[1] -= 0x8000;
	if (!jas_image_cmptsgnd(image, clut[2])) p[2] -= 0x8000;
	/* rotate to RGB */
#ifdef JPX_USE_IRT
	q[1] = p[0] - ((p[1] + p[2])>>2);
	q[0] = p[1] + q[1];
	q[2] = p[2] + q[1]; 
#else
	q[0] = (int)((double)p[0] + 1.402 * p[2]);
	q[1] = (int)((double)p[0] - 0.34413 * p[1] - 0.71414 * p[2]);
	q[2] = (int)((double)p[0] + 1.772 * p[1]);
#endif
	/* clamp */
	for (j = 0; j < 3; j++){
	  if (q[j] < 0) q[j] = 0;
	  else if (q[j] > 0xFFFF) q[j] = 0xFFFF;
   	}
	/* write out the pixel */
	dest[i] = q[0] >> 8;
	dest[i+1] = q[1] >> 8;
	dest[i+2] = q[2] >> 8;
	x++;
    }

    return count;
}

private int
copy_row_default(unsigned char *dest, jas_image_t *image,
	int x, int y, int bytes)
{
    int i, c,n;
    int count;

    n = jas_image_numcmpts(image);
    count = (bytes/n) * n;
    for (i = 1; i <= count; i+=n) {
	for (c = 0; c < n; c++)
	    dest[i+c] = jas_image_readcmptsample(image, c, x, y);
	x++;
    }

    return count;
}

/* buffer the input stream into our state */
private int
s_jpxd_buffer_input(stream_jpxd_state *const state, stream_cursor_read *pr,
		       long bytes)
{
    /* grow internal buffer if necessary */
    if (bytes > state->bufsize - state->buffill) {
        int newsize = state->bufsize;
        unsigned char *newbuf = NULL;
        while (newsize - state->buffill < bytes)
            newsize <<= 1;
        newbuf = (unsigned char *)gs_malloc(state->jpx_memory, newsize, 1, 
					    "JPXDecode temp buffer");
        /* TODO: check for allocation failure */
        memcpy(newbuf, state->buffer, state->buffill);
        gs_free(state->jpx_memory, state->buffer, state->bufsize, 1,
		"JPXDecode temp buffer");
        state->buffer = newbuf;
        state->bufsize = newsize;
    }

    /* copy requested amount of data and return */
    memcpy(state->buffer + state->buffill, pr->ptr + 1, bytes);
    state->buffill += bytes;
    pr->ptr += bytes;
    return bytes;
}

/* decode the compressed image data saved in our state */
private int
s_jpxd_decode_image(stream_jpxd_state * state)
{
    jas_stream_t *stream = state->stream;
    jas_image_t *image = NULL;

    /* see if an image is available */
    if (stream != NULL) {
	image = jas_image_decode(stream, -1, 0);
	if (image == NULL) {
	    dprintf("unable to decode JPX image data.\n");
	    return ERRC;
	}
#ifdef JPX_USE_JASPER_CM
	/* convert non-rgb multicomponent colorspaces to sRGB */
	if (jas_image_numcmpts(image) > 1 && 
	    jas_clrspc_fam(jas_image_clrspc(image)) != JAS_CLRSPC_FAM_RGB) {
	    jas_cmprof_t *outprof;
	    jas_image_t *rgbimage = NULL;
	    outprof = jas_cmprof_createfromclrspc(JAS_CLRSPC_SRGB);
	    if (outprof != NULL)
		rgbimage = jas_image_chclrspc(image, outprof, JAS_CMXFORM_INTENT_PER);
	    if (rgbimage != NULL) {
		jas_image_destroy(image);
		image = rgbimage;
	    }
	}
#endif
	state->image = image;
        state->offset = 0;
        jas_stream_close(stream);
        state->stream = NULL;

#ifdef JPX_DEBUG
	dump_jas_image(image);
#endif

    }

    return 0;
}

/* process a secton of the input and return any decoded data.
   see strimpl.h for return codes.
 */
private int
s_jpxd_process(stream_state * ss, stream_cursor_read * pr,
                 stream_cursor_write * pw, bool last)
{
    stream_jpxd_state *const state = (stream_jpxd_state *) ss;
    jas_stream_t *stream = state->stream;
    long in_size = pr->limit - pr->ptr;
    long out_size = pw->limit - pw->ptr;
    int status = 0;
    
    /* note that the gs stream library expects offset-by-one
       indexing of its buffers while we use zero indexing */
       
    /* JasPer has its own stream library, but there's no public
       api for handing it pieces. We need to add some plumbing 
       to convert between gs and jasper streams. In the meantime
       just buffer the entire stream, since it can handle that
       as input. */
    
    /* pass all available input to the decoder */
    if (in_size > 0) {
	s_jpxd_buffer_input(state, pr, in_size);
    }
    if ((last == 1) && (stream == NULL) && (state->image == NULL)) {
	/* turn our buffer into a stream */
	stream = jas_stream_memopen((char*)state->buffer, state->bufsize);
	state->stream = stream;
    }
    if (out_size > 0) {
        if (state->image == NULL) {
	    status = s_jpxd_decode_image(state);
        }
        if (state->image != NULL) {
            jas_image_t *image = state->image;
	    int numcmpts = jas_image_numcmpts(image);
	    int stride = numcmpts*jas_image_width(image);
            long image_size = stride*jas_image_height(image);
	    int clrspc = jas_image_clrspc(image);
	    int x, y;
	    long usable, done;
	    y = state->offset / stride;
	    x = state->offset - y*stride; /* bytes, not samples */
	    usable = min(out_size, stride - x);
	    x = x/numcmpts;               /* now samples */
	    /* copy data out of the decoded image data */
	    /* be lazy and only write the rest of the current row */
	    switch (jas_clrspc_fam(clrspc)) {
		case JAS_CLRSPC_FAM_RGB:
		    done = copy_row_rgb(pw->ptr, image, x, y, usable);
		    break;
		case JAS_CLRSPC_FAM_YCBCR:
		    done = copy_row_yuv(pw->ptr, image, x, y, usable);
		    break;
		case JAS_CLRSPC_FAM_GRAY:
		    done = copy_row_gray(pw->ptr, image, x, y, usable);
		    break;
		case JAS_CLRSPC_FAM_XYZ:
		case JAS_CLRSPC_FAM_LAB:
		case JAS_CLRSPC_FAM_UNKNOWN:
		default:
		    done = copy_row_default(pw->ptr, image, x, y, usable);
		    break;
	    }
	    pw->ptr += done;
            state->offset += done;
            status = (state->offset < image_size) ? 1 : 0;
        }
    }    
    
    return status;
}

/* stream release.
   free all our decoder state.
 */
private void
s_jpxd_release(stream_state *ss)
{
    stream_jpxd_state *const state = (stream_jpxd_state *) ss;

    if (state) {
        if (state->image) jas_image_destroy(state->image);
    	if (state->stream) jas_stream_close(state->stream);
	if (state->buffer) gs_free(state->jpx_memory, state->buffer, state->bufsize, 1,
				"JPXDecode temp buffer");
    }
}

/* set stream defaults.
   this hook exists to avoid confusing the gc with bogus
   pointers. we use it similarly just to NULL all the pointers.
   (could just be done in _init?)
 */
private void
s_jpxd_set_defaults(stream_state *ss)
{
    stream_jpxd_state *const state = (stream_jpxd_state *) ss;
    
    state->stream = NULL;
    state->image = NULL;
    state->offset = 0;
    state->buffer = NULL;
    state->bufsize = 0;
    state->buffill = 0;
}


/* stream template */
const stream_template s_jpxd_template = {
    &st_jpxd_state, 
    s_jpxd_init,
    s_jpxd_process,
    1, 1, /* min in and out buffer sizes we can handle 
                     should be ~32k,64k for efficiency? */
    s_jpxd_release,
    s_jpxd_set_defaults
};
