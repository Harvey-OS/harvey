/* Copyright (C) 1995, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gslib.c,v 1.15 2004/09/16 08:03:56 igor Exp $ */
/* Test program for Ghostscript library */
/* Capture stdin/out/err before gsio.h redefines them. */
#include "stdio_.h"
static FILE *real_stdin, *real_stdout, *real_stderr;
static void
get_real(void)
{
    real_stdin = stdin, real_stdout = stdout, real_stderr = stderr;
}
#include "math_.h"
#include "string_.h"
#include "gx.h"
#include "gp.h"
#include "gsalloc.h"
#include "gscdefs.h"
#include "gserrors.h"
#include "gslib.h"
#include "gsmatrix.h"
#include "gsstate.h"
#include "gscspace.h"
#include "gscolor2.h"
#include "gscoord.h"
#include "gscie.h"
#include "gscrd.h"
#include "gsiparm3.h"
#include "gsiparm4.h"
#include "gsparam.h"
#include "gspaint.h"
#include "gspath.h"
#include "gspath2.h"
#include "gsrop.h"
#include "gsstruct.h"
#include "gsutil.h"
#include "gxalloc.h"
#include "gxdcolor.h"		/* for gx_device_white/black */
#include "gxdevice.h"
#include "gxht.h"		/* for gs_halftone */
#include "gdevbbox.h"
#include "gdevcmap.h"
#include "gshtx.h"

/* Define whether we are processing captured data. */
/*#define CAPTURE */

/* Test programs */
private int test1(gs_state *, gs_memory_t *);	/* kaleidoscope */
private int test2(gs_state *, gs_memory_t *);	/* pattern fill */
private int test3(gs_state *, gs_memory_t *);	/* RasterOp */
private int test4(gs_state *, gs_memory_t *);	/* set resolution */
private int test5(gs_state *, gs_memory_t *);	/* images */
private int test6(gs_state *, gs_memory_t *);	/* CIE API, snapping */
private int test7(gs_state *, gs_memory_t *);	/* non-monot HT */
private int test8(gs_state *, gs_memory_t *);	/* transp patterns */

#ifdef CAPTURE
#include "k/capture.c"
private int test10(gs_state *, gs_memory_t *);	/* captured data */

#endif
private int (*tests[]) (gs_state *, gs_memory_t *) =
{
    test1, test2, test3, test4, test5,
	test6, test7, test8, 0
#ifdef CAPTURE
	test10
#endif
};

/* Include the extern for the device list. */
extern_gs_lib_device_list();

/* Forward references */
private float odsf(floatp, floatp);


int
main(int argc, const char *argv[])
{
    char achar;
    gs_ref_memory_t *imem;

#define mem ((gs_memory_t *)imem)
    gs_state *pgs;
    const gx_device *const *list;
    gx_device *dev;
    gx_device_bbox *bbdev;
    int code;

    gp_init();
    get_real();
    gs_stdin = real_stdin;
    gs_stdout = real_stdout;
    gs_stderr = real_stderr;
    gs_lib_init(stdout);
    if (argc < 2 || (achar = argv[1][0]) < '1' ||
	achar > '0' + countof(tests)
	) {
	lprintf1("Usage: gslib 1..%c\n", '0' + countof(tests));
	exit(1);
    }
    gs_debug['@'] = 1;
    gs_debug['?'] = 1;
/*gs_debug['B'] = 1; *//****** PATCH ******/
/*gs_debug['L'] = 1; *//****** PATCH ******/
    imem = ialloc_alloc_state(&gs_memory_default, 20000);
    imem->space = 0;
    /*
     * gs_iodev_init must be called after the rest of the inits, for
     * obscure reasons that really should be documented!
     */
    gs_iodev_init(mem);
/****** WRONG ******/
    gs_lib_device_list(&list, NULL);
    gs_copydevice(&dev, list[0], mem);
    check_device_separable(dev);
    gx_device_fill_in_procs(dev);
    bbdev =
	gs_alloc_struct_immovable(mem, gx_device_bbox, &st_device_bbox,
				  "bbox");
    gx_device_bbox_init(bbdev, dev, mem);
    /* Print out the device name just to test the gsparam.c API. */
    {
	gs_c_param_list list;
	gs_param_string nstr;

	gs_c_param_list_write(&list, mem);
	code = gs_getdeviceparams(dev, (gs_param_list *) & list);
	if (code < 0) {
	    lprintf1("getdeviceparams failed! code = %d\n", code);
	    exit(1);
	}
	gs_c_param_list_read(&list);
	code = param_read_string((gs_param_list *) & list, "Name", &nstr);
	if (code < 0) {
	    lprintf1("reading Name failed! code = %d\n", code);
	    exit(1);
	}
	dputs("Device name = ");
	debug_print_string(nstr.data, nstr.size);
	dputs("\n");
	gs_c_param_list_release(&list);
    }
    /*
     * If this is a device that takes an OutputFile, set the OutputFile
     * to "-" in the copy.
     */
    {
	gs_c_param_list list;
	gs_param_string nstr;

	gs_c_param_list_write(&list, mem);
	param_string_from_string(nstr, "-");
	code = param_write_string((gs_param_list *)&list, "OutputFile", &nstr);
	if (code < 0) {
	    lprintf1("writing OutputFile failed! code = %d\n", code);
	    exit(1);
	}
	gs_c_param_list_read(&list);
	code = gs_putdeviceparams(dev, (gs_param_list *)&list);
	gs_c_param_list_release(&list);
	if (code < 0 && code != gs_error_undefined) {
	    lprintf1("putdeviceparams failed! code = %d\n", code);
	    exit(1);
	}
    }
    dev = (gx_device *) bbdev;
    pgs = gs_state_alloc(mem);
    gs_setdevice_no_erase(pgs, dev);	/* can't erase yet */
    {
	gs_point dpi;
	gs_screen_halftone ht;

	gs_dtransform(pgs, 72.0, 72.0, &dpi);
	ht.frequency = min(fabs(dpi.x), fabs(dpi.y)) / 16.001;
	ht.angle = 0;
	ht.spot_function = odsf;
	gs_setscreen(pgs, &ht);
    }
    /* gsave and grestore (among other places) assume that */
    /* there are at least 2 gstates on the graphics stack. */
    /* Ensure that now. */
    gs_gsave(pgs);
    gs_erasepage(pgs);

    code = (*tests[achar - '1']) (pgs, mem);
    gs_output_page(pgs, 1, 1);
    {
	gs_rect bbox;

	gx_device_bbox_bbox(bbdev, &bbox);
	dprintf4("Bounding box: [%g %g %g %g]\n",
		 bbox.p.x, bbox.p.y, bbox.q.x, bbox.q.y);
    }
    if (code)
	dprintf1("**** Test returned code = %d.\n", code);
    dputs("Done.  Press <enter> to exit.");
    fgetc(gs_stdin);
    gs_lib_finit(0, 0);
    return 0;
#undef mem
}
/* Ordered dither spot function */
private float
odsf(floatp x, floatp y)
{
    static const byte dither[256] =
    {
	0x0E, 0x8E, 0x2E, 0xAE, 0x06, 0x86, 0x26, 0xA6, 0x0C, 0x8C, 0x2C, 0xAC, 0x04, 0x84, 0x24, 0xA4,
	0xCE, 0x4E, 0xEE, 0x6E, 0xC6, 0x46, 0xE6, 0x66, 0xCC, 0x4C, 0xEC, 0x6C, 0xC4, 0x44, 0xE4, 0x64,
	0x3E, 0xBE, 0x1E, 0x9E, 0x36, 0xB6, 0x16, 0x96, 0x3C, 0xBC, 0x1C, 0x9C, 0x34, 0xB4, 0x14, 0x94,
	0xFE, 0x7E, 0xDE, 0x5E, 0xF6, 0x76, 0xD6, 0x56, 0xFC, 0x7C, 0xDC, 0x5C, 0xF4, 0x74, 0xD4, 0x54,
	0x01, 0x81, 0x21, 0xA1, 0x09, 0x89, 0x29, 0xA9, 0x03, 0x83, 0x23, 0xA3, 0x0B, 0x8B, 0x2B, 0xAB,
	0xC1, 0x41, 0xE1, 0x61, 0xC9, 0x49, 0xE9, 0x69, 0xC3, 0x43, 0xE3, 0x63, 0xCB, 0x4B, 0xEB, 0x6B,
	0x31, 0xB1, 0x11, 0x91, 0x39, 0xB9, 0x19, 0x99, 0x33, 0xB3, 0x13, 0x93, 0x3B, 0xBB, 0x1B, 0x9B,
	0xF1, 0x71, 0xD1, 0x51, 0xF9, 0x79, 0xD9, 0x59, 0xF3, 0x73, 0xD3, 0x53, 0xFB, 0x7B, 0xDB, 0x5B,
	0x0D, 0x8D, 0x2D, 0xAD, 0x05, 0x85, 0x25, 0xA5, 0x0F, 0x8F, 0x2F, 0xAF, 0x07, 0x87, 0x27, 0xA7,
	0xCD, 0x4D, 0xED, 0x6D, 0xC5, 0x45, 0xE5, 0x65, 0xCF, 0x4F, 0xEF, 0x6F, 0xC7, 0x47, 0xE7, 0x67,
	0x3D, 0xBD, 0x1D, 0x9D, 0x35, 0xB5, 0x15, 0x95, 0x3F, 0xBF, 0x1F, 0x9F, 0x37, 0xB7, 0x17, 0x97,
	0xFD, 0x7D, 0xDD, 0x5D, 0xF5, 0x75, 0xD5, 0x55, 0xFF, 0x7F, 0xDF, 0x5F, 0xF7, 0x77, 0xD7, 0x57,
	0x02, 0x82, 0x22, 0xA2, 0x0A, 0x8A, 0x2A, 0xAA, 0x00, 0x80, 0x20, 0xA0, 0x08, 0x88, 0x28, 0xA8,
	0xC2, 0x42, 0xE2, 0x62, 0xCA, 0x4A, 0xEA, 0x6A, 0xC0, 0x40, 0xE0, 0x60, 0xC8, 0x48, 0xE8, 0x68,
	0x32, 0xB2, 0x12, 0x92, 0x3A, 0xBA, 0x1A, 0x9A, 0x30, 0xB0, 0x10, 0x90, 0x38, 0xB8, 0x18, 0x98,
	0xF2, 0x72, 0xD2, 0x52, 0xFA, 0x7A, 0xDA, 0x5A, 0xF0, 0x70, 0xD0, 0x50, 0xF8, 0x78, 0xD8, 0x58
    };
    int i = (int)((x + 1) * 7.9999);
    int j = (int)((y + 1) * 7.9999);

    return dither[16 * i + j] / 256.0;
}

/* Fill a rectangle. */
private int
fill_rect1(gs_state * pgs, floatp x, floatp y, floatp w, floatp h)
{
    gs_rect r;

    r.q.x = (r.p.x = x) + w;
    r.q.y = (r.p.y = y) + h;
    return gs_rectfill(pgs, &r, 1);
}

/* Stubs for GC */
const gs_ptr_procs_t ptr_struct_procs =
{NULL, NULL, NULL};
const gs_ptr_procs_t ptr_string_procs =
{NULL, NULL, NULL};
const gs_ptr_procs_t ptr_const_string_procs =
{NULL, NULL, NULL};
void *				/* obj_header_t * */
gs_reloc_struct_ptr(const void * /* obj_header_t * */ obj, gc_state_t * gcst)
{
    return (void *)obj;
}
void
gs_reloc_string(gs_string * sptr, gc_state_t * gcst)
{
}
void
gs_reloc_const_string(gs_const_string * sptr, gc_state_t * gcst)
{
}

/* Other stubs */
void
gs_to_exit(const gs_memory_t *mem, int exit_status)
{
    gs_lib_finit(mem, exit_status, 0);
}

void
gs_abort(const gs_memory_t *mem)
{
    gs_to_exit(mem, 1); /* cleanup */
    gp_do_exit(1); /* system independent exit() */	
}


/* Return the number with the magnitude of x and the sign of y. */
/* This is a BSD addition to libm; not all compilers have it. */
private double
gs_copysign(floatp x, floatp y)
{
   return ( y >= 0  ? fabs(x) : -fabs(x) );
}


/* ---------------- Test program 1 ---------------- */
/* Draw a colored kaleidoscope. */

/* Random number generator */
private long rand_state = 1;
private long
rand(void)
{
#define A 16807
#define M 0x7fffffff
#define Q 127773		/* M / A */
#define R 2836			/* M % A */
    rand_state = A * (rand_state % Q) - R * (rand_state / Q);
    /* Note that rand_state cannot be 0 here. */
    if (rand_state <= 0)
	rand_state += M;
#undef A
#undef M
#undef Q
#undef R
    return rand_state;
}
private int
test1(gs_state * pgs, gs_memory_t * mem)
{
    int n;

    gs_scale(pgs, 72.0, 72.0);
    gs_translate(pgs, 4.25, 5.5);
    gs_scale(pgs, 4.0, 4.0);
    gs_newpath(pgs);
    for (n = 200; --n >= 0;) {
	int j;

#define rf() (rand() / (1.0 * 0x10000 * 0x8000))
	double r = rf(), g = rf(), b = rf();
	double x0 = rf(), y0 = rf(), x1 = rf(), y1 = rf(), x2 = rf(), y2 = rf();

	gs_setrgbcolor(pgs, r, g, b);
	for (j = 0; j < 6; j++) {
	    gs_gsave(pgs);
	    gs_rotate(pgs, 60.0 * j);
	    gs_moveto(pgs, x0, y0);
	    gs_lineto(pgs, x1, y1);
	    gs_lineto(pgs, x2, y2);
	    gs_fill(pgs);
	    gs_grestore(pgs);
	}
    }
#undef mem
    return 0;
}

/* ---------------- Test program 2 ---------------- */
/* Fill an area with a pattern. */

private int
test2(gs_state * pgs, gs_memory_t * mem)
{
    gs_client_color cc;
    gx_tile_bitmap tile;
    /*const */ byte tpdata[] =
    {
    /* Define a pattern that looks like this:
       ..xxxx
       .....x
       .....x
       ..xxxx
       .x....
       x.....
     */
	0x3c, 0, 0, 0, 0x04, 0, 0, 0, 0x04, 0, 0, 0, 0x3c, 0, 0, 0,
	0x40, 0, 0, 0, 0x80, 0, 0, 0
    };

    gs_newpath(pgs);
    gs_moveto(pgs, 100.0, 300.0);
    gs_lineto(pgs, 500.0, 500.0);
    gs_lineto(pgs, 200.0, 100.0);
    gs_lineto(pgs, 300.0, 500.0);
    gs_lineto(pgs, 500.0, 200.0);
    gs_closepath(pgs);
    gs_setrgbcolor(pgs, 0.0, 0.0, 0.0);
    gs_gsave(pgs);
    gs_fill(pgs);
    gs_grestore(pgs);
    tile.data = tpdata;
    tile.raster = 4;
    tile.size.x = tile.rep_width = 6;
    tile.size.y = tile.rep_height = 6;
    tile.id = gx_no_bitmap_id;
    gs_makebitmappattern(&cc, &tile, true, pgs, NULL);
    /* Note: color space is DeviceRGB */
    cc.paint.values[0] = 0.0;
    cc.paint.values[1] = 1.0;
    cc.paint.values[2] = 1.0;
    gs_setpattern(pgs, &cc);
    gs_eofill(pgs);
    gs_makebitmappattern(&cc, &tile, false, pgs, NULL);
    gs_setcolor(pgs, &cc);
    gs_moveto(pgs, 50.0, 50.0);
    gs_lineto(pgs, 300.0, 50.0);
    gs_lineto(pgs, 50.0, 300.0);
    gs_closepath(pgs);
    gs_setrgbcolor(pgs, 1.0, 0.0, 0.0);
    gs_gsave(pgs);
    gs_fill(pgs);
    gs_grestore(pgs);
    gs_setpattern(pgs, &cc);
    gs_eofill(pgs);
    return 0;
}

/* ---------------- Test program 3 ---------------- */
/* Exercise RasterOp a little. */
/* Currently, this only works with monobit devices. */

private int
test3(gs_state * pgs, gs_memory_t * mem)
{
    gx_device *dev = gs_currentdevice(pgs);
    gx_color_index black = gx_device_black(dev);
    gx_color_index white = gx_device_white(dev);
    gx_color_index black2[2];
    gx_color_index black_white[2];
    gx_color_index white_black[2];
    long pattern[max(align_bitmap_mod / sizeof(long), 1) * 4];

#define pbytes ((byte *)pattern)
    gx_tile_bitmap tile;

    black2[0] = black2[1] = black;
    black_white[0] = white_black[1] = black;
    black_white[1] = white_black[0] = white;
    pbytes[0] = 0xf0;
    pbytes[align_bitmap_mod] = 0x90;
    pbytes[align_bitmap_mod * 2] = 0x90;
    pbytes[align_bitmap_mod * 3] = 0xf0;
    tile.data = pbytes;
    tile.raster = align_bitmap_mod;
    tile.size.x = tile.size.y = 4;
    tile.id = gs_next_ids(1);
    tile.rep_width = tile.rep_height = 4;
    (*dev_proc(dev, copy_rop))
	(dev, NULL, 0, 0, gx_no_bitmap_id, black2,
	 &tile, white_black, 100, 100, 150, 150, 0, 0, rop3_T);
    (*dev_proc(dev, copy_rop))
	(dev, NULL, 0, 0, gx_no_bitmap_id, black2,
	 NULL, NULL, 120, 120, 110, 110, 0, 0, ~rop3_S & rop3_1);
    (*dev_proc(dev, copy_rop))
	(dev, NULL, 0, 0, gx_no_bitmap_id, black2,
	 &tile, white_black, 110, 110, 130, 130, 0, 0, rop3_T ^ rop3_D);
#undef pbytes
    return 0;
}

/* ---------------- Test program 4 ---------------- */
/* Set the resolution dynamically. */

private int
test4(gs_state * pgs, gs_memory_t * mem)
{
    gs_c_param_list list;
    float resv[2];
    gs_param_float_array ares;
    int code;
    gx_device *dev = gs_currentdevice(pgs);

    gs_c_param_list_write(&list, mem);
    resv[0] = resv[1] = 100;
    ares.data = resv;
    ares.size = 2;
    ares.persistent = true;
    code = param_write_float_array((gs_param_list *) & list,
				   "HWResolution", &ares);
    if (code < 0) {
	lprintf1("Writing HWResolution failed: %d\n", code);
	exit(1);
    }
    gs_c_param_list_read(&list);
    code = gs_putdeviceparams(dev, (gs_param_list *) & list);
    gs_c_param_list_release(&list);
    if (code < 0) {
	lprintf1("Setting HWResolution failed: %d\n", code);
	exit(1);
    }
    gs_initmatrix(pgs);
    gs_initclip(pgs);
    if (code == 1) {
	code = (*dev_proc(dev, open_device)) (dev);
	if (code < 0) {
	    lprintf1("Reopening device failed: %d\n", code);
	    exit(1);
	}
    }
    gs_moveto(pgs, 0.0, 72.0);
    gs_rlineto(pgs, 72.0, 0.0);
    gs_rlineto(pgs, 0.0, 72.0);
    gs_closepath(pgs);
    gs_stroke(pgs);
    return 0;
}

/* ---------------- Test program 5 ---------------- */
/* Test masked (and non-masked) images. */

private int
test5(gs_state * pgs, gs_memory_t * mem)
{
    gx_device *dev = gs_currentdevice(pgs);
    gx_image_enum_common_t *info;
    gx_image_plane_t planes[5];
    gx_drawing_color dcolor;
    int code;
    static const byte data3[] =
    {
	0x00, 0x44, 0x88, 0xcc,
	0x44, 0x88, 0xcc, 0x00,
	0x88, 0xcc, 0x00, 0x44,
	0xcc, 0x00, 0x44, 0x88
    };
    gs_color_space gray_cs;

    gs_cspace_init_DeviceGray(&gray_cs);

    /*
     * Neither ImageType 3 nor 4 needs a current color,
     * but some intermediate code assumes it's valid.
     */
    set_nonclient_dev_color(&dcolor, 0);

    /* Scale everything up, and fill the background. */
    {
	gs_matrix mat;

	gs_currentmatrix(pgs, &mat);
	mat.xx = gs_copysign(98.6, mat.xx);
	mat.yy = gs_copysign(98.6, mat.yy);
	mat.tx = floor(mat.tx) + 0.499;
	mat.ty = floor(mat.ty) + 0.499;
	gs_setmatrix(pgs, &mat);
    }
    gs_setrgbcolor(pgs, 1.0, 0.9, 0.9);
    fill_rect1(pgs, 0.25, 0.25, 4.0, 6.0);
    gs_setrgbcolor(pgs, 0.5, 1.0, 0.5);

#if 0
    /* Make things a little more interesting.... */
    gs_translate(pgs, 1.0, 1.0);
    gs_rotate(pgs, 10.0);
    gs_scale(pgs, 1.3, 0.9);
#endif

#define do_image(image, idata)\
  BEGIN\
  code = gx_device_begin_typed_image(dev, (gs_imager_state *)pgs, NULL,\
     (gs_image_common_t *)&image, NULL, &dcolor, NULL, mem, &info);\
  /****** TEST code >= 0 ******/\
  planes[0].data = idata;\
  planes[0].data_x = 0;\
  planes[0].raster = (image.Height * image.BitsPerComponent + 7) >> 3;\
  code = gx_image_plane_data(info, planes, image.Height);\
  /****** TEST code == 1 ******/\
  code = gx_image_end(info, true);\
  /****** TEST code >= 0 ******/\
  END

#define W 4
#define H 4

    /* Test an unmasked image. */
    gs_gsave(pgs);
    {
	gs_image1_t image1;
	void *info1;
        gs_color_space cs;

        gs_cspace_init_DeviceGray(&cs);
	gs_image_t_init(&image1, &cs);
	/* image */
	image1.ImageMatrix.xx = W;
	image1.ImageMatrix.yy = -H;
	image1.ImageMatrix.ty = H;
	/* data_image */
	image1.Width = W;
	image1.Height = H;
	image1.BitsPerComponent = 8;

	gs_translate(pgs, 0.5, 4.0);
	code = gx_device_begin_image(dev, (gs_imager_state *) pgs,
				     &image1, gs_image_format_chunky,
				     NULL, &dcolor, NULL, mem, &info1);
/****** TEST code >= 0 ******/
	planes[0].data = data3;
	planes[0].data_x = 0;
	planes[0].raster =
	    (image1.Height * image1.BitsPerComponent + 7) >> 3;
	/* Use the old image_data API. */
	code = gx_image_data(info1, &planes[0].data, 0,
			     planes[0].raster, image1.Height);
/****** TEST code == 1 ******/
	code = gx_image_end(info1, true);
/****** TEST code >= 0 ******/
    }
    gs_grestore(pgs);

    /* Test an explicitly masked image. */
    gs_gsave(pgs);
    {
	gs_image3_t image3;
	static const byte data3mask[] =
	{
	    0x60,
	    0x90,
	    0x90,
	    0x60
	};
	static const byte data3x2mask[] =
	{
	    0x66,
	    0x99,
	    0x99,
	    0x66,
	    0x66,
	    0x99,
	    0x99,
	    0x66
	};

	gs_image3_t_init(&image3, &gray_cs, interleave_scan_lines);
	/* image */
	image3.ImageMatrix.xx = W;
	image3.ImageMatrix.yy = -H;
	image3.ImageMatrix.ty = H;
	/* data_image */
	image3.Width = W;
	image3.Height = H;
	image3.BitsPerComponent = 8;
	/* MaskDict */
	image3.MaskDict.ImageMatrix = image3.ImageMatrix;
	image3.MaskDict.Width = image3.Width;
	image3.MaskDict.Height = image3.Height;

	/* Display with 1-for-1 mask and image. */
	gs_translate(pgs, 0.5, 2.0);
	code = gx_device_begin_typed_image(dev, (gs_imager_state *) pgs,
				       NULL, (gs_image_common_t *) & image3,
					   NULL, &dcolor, NULL, mem, &info);
/****** TEST code >= 0 ******/
	planes[0].data = data3mask;
	planes[0].data_x = 0;
	planes[0].raster = (image3.MaskDict.Height + 7) >> 3;
	planes[1].data = data3;
	planes[1].data_x = 0;
	planes[1].raster =
	    (image3.Height * image3.BitsPerComponent + 7) >> 3;
	code = gx_image_plane_data(info, planes, image3.Height);
/****** TEST code == 1 ******/
	code = gx_image_end(info, true);
/****** TEST code >= 0 ******/

	/* Display with 2-for-1 mask and image. */
	image3.MaskDict.ImageMatrix.xx *= 2;
	image3.MaskDict.ImageMatrix.yy *= 2;
	image3.MaskDict.ImageMatrix.ty *= 2;
	image3.MaskDict.Width *= 2;
	image3.MaskDict.Height *= 2;
	gs_translate(pgs, 1.5, 0.0);
	code = gx_device_begin_typed_image(dev, (gs_imager_state *) pgs,
				       NULL, (gs_image_common_t *) & image3,
					   NULL, &dcolor, NULL, mem, &info);
/****** TEST code >= 0 ******/
	planes[0].data = data3x2mask;
	planes[0].raster = (image3.MaskDict.Width + 7) >> 3;
	{
	    int i;

	    for (i = 0; i < H; ++i) {
		planes[1].data = 0;
		code = gx_image_plane_data(info, planes, 1);
		planes[0].data += planes[0].raster;
/****** TEST code == 0 ******/
		planes[1].data = data3 + i * planes[1].raster;
		code = gx_image_plane_data(info, planes, 1);
		planes[0].data += planes[0].raster;
/****** TEST code >= 0 ******/
	    }
	}
/****** TEST code == 1 ******/
	code = gx_image_end(info, true);
/****** TEST code >= 0 ******/
    }
    gs_grestore(pgs);

    /* Test a chroma-keyed masked image. */
    gs_gsave(pgs);
    {
	gs_image4_t image4;
	const byte *data4 = data3;

	gs_image4_t_init(&image4, &gray_cs);
	/* image */
	image4.ImageMatrix.xx = W;
	image4.ImageMatrix.yy = -H;
	image4.ImageMatrix.ty = H;
	/* data_image */
	image4.Width = W;
	image4.Height = H;
	image4.BitsPerComponent = 8;

	/* Display with a single mask color. */
	gs_translate(pgs, 0.5, 0.5);
	image4.MaskColor_is_range = false;
	image4.MaskColor[0] = 0xcc;
	do_image(image4, data4);

	/* Display a second time with a color range. */
	gs_translate(pgs, 1.5, 0.0);
	image4.MaskColor_is_range = true;
	image4.MaskColor[0] = 0x40;
	image4.MaskColor[1] = 0x90;
	do_image(image4, data4);
    }
    gs_grestore(pgs);

#undef W
#undef H
#undef do_image
    return 0;
}

/* ---------------- Test program 6 ---------------- */
/* Test the C API for CIE CRDs, and color snapping. */

private void
spectrum(gs_state * pgs, int n)
{
    float den = n;
    float den1 = n - 1;
    float den2 = (n * 2 - 1) * n;
    int a, b, c;

    for (a = 0; a < n; ++a)
	for (b = 0; b < n; ++b)
	    for (c = 0; c < n; ++c) {
		double size = (n * 2 - c * 2 - 1) / den2;
		gs_client_color cc;

		cc.paint.values[0] = a / den1;
		cc.paint.values[1] = b / den1;
		cc.paint.values[2] = c / den1;
		gs_setcolor(pgs, &cc);
		fill_rect1(pgs,
			   a / den + c / den2, b / den + c / den2,
			   size, size);
	    }
}
private float
render_abc(floatp v, const gs_cie_render * ignore_crd)
{
    return v / 2;
}
private int
set_cmap_method(gx_device_cmap *cmdev, gx_device_color_mapping_method_t method,
		gs_state *pgs, gs_memory_t *mem)
{
    gs_c_param_list list;
    int cmm = method;

    gs_c_param_list_write(&list, mem);
    param_write_int((gs_param_list *)&list, "ColorMappingMethod", &cmm);
    gs_c_param_list_read(&list);
    gs_putdeviceparams((gx_device *)cmdev, (gs_param_list *)&list);
    gs_c_param_list_release(&list);
    gs_setdevice_no_init(pgs, (gx_device *)cmdev);
    return 0;
}
private int
test6(gs_state * pgs, gs_memory_t * mem)
{
    gs_color_space *pcs;
    gs_cie_abc *pabc;
    gs_cie_render *pcrd;
    static const gs_vector3 white_point =
    {1, 1, 1};
    static const gs_cie_render_proc3 encode_abc =
    {
	{render_abc, render_abc, render_abc}
    };
    gx_device_cmap *cmdev;
    int code;
    gs_color_space rgb_cs;

    gs_cspace_init_DeviceRGB(&rgb_cs);

    gs_scale(pgs, 150.0, 150.0);
    gs_translate(pgs, 0.5, 0.5);
    gs_setcolorspace(pgs, &rgb_cs);
    spectrum(pgs, 5);
    gs_translate(pgs, 1.2, 0.0);
    /* We must set the CRD before the color space. */
    code = gs_cie_render1_build(&pcrd, mem, "test6");
    if (code < 0)
	return code;
    gs_cie_render1_initialize(pcrd, NULL, &white_point, NULL,
			      NULL, NULL, NULL,
			      NULL, NULL, NULL,
			      NULL, &encode_abc, NULL,
			      NULL);
    gs_setcolorrendering(pgs, pcrd);
    gs_cspace_build_CIEABC(&pcs, NULL, mem);
    /* There should be an API for initializing CIE color spaces too.... */
    pabc = pcs->params.abc;
    pabc->common.points.WhitePoint = white_point;
    gs_cie_abc_complete(pabc);
    /* End of initializing the color space. */
    gs_setcolorspace(pgs, pcs);
    spectrum(pgs, 5);
    /* Now test color snapping. */
    cmdev =
	gs_alloc_struct_immovable(mem, gx_device_cmap, &st_device_cmap,
				  "cmap device");
    gdev_cmap_init(cmdev, gs_currentdevice(pgs),
		   device_cmap_snap_to_primaries);
    gs_setdevice_no_init(pgs, (gx_device *) cmdev);
    gs_setrgbcolor(pgs, 0.0, 0.0, 0.0);		/* back to DeviceRGB space */
    gs_translate(pgs, -1.2, 1.2);
    spectrum(pgs, 5);
    gs_translate(pgs, 1.2, 0.0);
    set_cmap_method(cmdev, device_cmap_monochrome, pgs, mem);
    spectrum(pgs, 5);
    gs_translate(pgs, -1.2, 1.2);
    set_cmap_method(cmdev, device_cmap_color_to_black_over_white, pgs, mem);
    spectrum(pgs, 5);
    return 0;
}

/* ---------------- Test program 7 ---------------- */
/* Test the C API for non-monotonic halftones. */

private int
test7(gs_state * pgs, gs_memory_t * mem)
{
    /* Define a non-monotonic 4 x 4 halftone with 4 gray levels. */
    static const byte masks[1 * 4 * 4] =
    {
    /* 0% */
	0x00, 0x00, 0x00, 0x00,
    /* 25% */
	0x80, 0x40, 0x20, 0x10,
    /* 50% */
	0xa0, 0xa0, 0x50, 0x50,
    /* 75% */
	0xd0, 0xe0, 0x70, 0xb0
    };
    gs_ht *pht;
    int code;
    int i;

    /* Fabricate a Type 5 halftone. */
    code = gs_ht_build(&pht, 1, mem);
    dprintf1("ht build code = %d\n", code);
    code = gs_ht_set_mask_comp(pht, 0,
			       4, 4, 4, masks, NULL, NULL);
    dprintf1("set mask code = %d\n", code);
    code = gs_sethalftone(pgs, pht);
    dprintf1("sethalftone code = %d\n", code);
    for (i = 0; i <= 4; ++i) {
	gs_setgray(pgs, i / 4.0);
	fill_rect1(pgs, 100 + i * 100, 100, 50, 50);
    }
    return 0;
}

/* ---------------- Test program 8 ---------------- */
/* Test partially transparent patterns */

private int
test8(gs_state * pgs, gs_memory_t * mem)
{
    /*
     * Define a 16 x 16 pattern using a 4-entry palette
     * (white, red, green, black).
     */
    static const byte pdata[] =
    {
	0x7f, 0xff, 0x00, 0x03,
	0x7f, 0xff, 0x00, 0x0c,
	0x50, 0x00, 0x00, 0x30,
	0x50, 0x00, 0x00, 0xc0,
	0x50, 0x00, 0x03, 0x00,
	0x50, 0x00, 0x0c, 0x00,
	0x50, 0x00, 0x30, 0x00,
	0x50, 0x00, 0xc0, 0x00,
	0xf0, 0x00, 0xc0, 0x00,
	0xf0, 0x00, 0x30, 0x00,
	0xf0, 0x00, 0x0c, 0x00,
	0xf0, 0x00, 0x03, 0x00,
	0xf0, 0x00, 0x00, 0xc0,
	0xf0, 0x00, 0x00, 0x30,
	0xea, 0x55, 0xaa, 0x5c,
	0xea, 0x55, 0xaa, 0x57,
    };
    gs_depth_bitmap ptile;
    gs_const_string table;
    gs_color_space *pcs;
    gs_client_color ccolor;
    gs_color_space rgb_cs;

    gs_cspace_init_DeviceRGB(&rgb_cs);

    table.data =
	(const byte *)"\377\377\377\377\000\000\000\377\000\000\000\000";
    table.size = 12;
    gs_cspace_build_Indexed(&pcs, &rgb_cs, 4, &table, mem);
    ptile.data = pdata;
    ptile.raster = 4;
    ptile.size.x = ptile.size.y = 16;
    ptile.id = gs_no_bitmap_id;
    ptile.pix_depth = 2;
    ptile.num_comps = 1;
    gs_makepixmappattern(&ccolor, &ptile, false /*mask */ , NULL /*pmat */ ,
			 gs_no_id, pcs, 0 /*white_index */ , pgs, mem);
    {
	gs_rect r;

	r.p.x = 100;
	r.p.y = 100;
	r.q.x = 200;
	r.q.y = 200;
	gs_setrgbcolor(pgs, 1.0, 1.0, 0.0);
	gs_rectfill(pgs, &r, 1);
	gs_setpattern(pgs, &ccolor);
	gs_settexturetransparent(pgs, true);
	gs_rectfill(pgs, &r, 1);
	r.p.x += 150;
	r.q.x += 150;
	gs_setrgbcolor(pgs, 1.0, 1.0, 0.0);
	gs_rectfill(pgs, &r, 1);
	gs_setpattern(pgs, &ccolor);
	gs_settexturetransparent(pgs, false);
	gs_rectfill(pgs, &r, 1);
    }
    return 0;
}


#ifdef CAPTURE

/* ---------------- Test program 10 ---------------- */
/* Replay captured data for printer output. */

private const char outfile[] = "t.pbm";
private const float ypage_wid = 11.0;
private const float xpage_len = 17.0;
private const int rotate_value = 0;
private const float scale_x = 0.45;
private const float scale_y = 0.45;
private const float xmove_origin = 0.0;
private const float ymove_origin = 0.0;

private int
test10(gs_state * pgs, gs_memory_t * mem)
{
    gs_c_param_list list;
    gs_param_string nstr, OFstr;
    gs_param_float_array PSa;
    gs_param_float_array HWRa;
    gs_param_int_array HWSa;
    int HWSize[2];
    float HWResolution[2], PageSize[2];
    long MaxBitmap;
    int code;
    gx_device *dev = gs_currentdevice(pgs);
    float xlate_x, xlate_y;
    gs_rect cliprect;

    gs_c_param_list_write(&list, mem);
    code = gs_getdeviceparams(dev, (gs_param_list *) & list);
    if (code < 0) {
	lprintf1("getdeviceparams failed! code = %d\n", code);
	exit(1);
    }
    gs_c_param_list_read(&list);
    code = param_read_string((gs_param_list *) & list, "Name", &nstr);
    if (code < 0) {
	lprintf1("reading Name failed! code = %d\n", code);
	exit(1);
    }
    code = param_read_int_array((gs_param_list *) & list,
				"HWSize", &HWSa);
    if (code < 0) {
	lprintf1("reading HWSize failed! code = %d\n", code);
	exit(1);
    }
    eprintf3("HWSize[%d] = [ %d, %d ]\n", HWSa.size,
	     HWSa.data[0], HWSa.data[1]);
    code = param_read_float_array((gs_param_list *) & list,
				  "HWResolution", &HWRa);
    if (code < 0) {
	lprintf1("reading Resolution failed! code = %d\n", code);
	exit(1);
    }
    eprintf3("HWResolution[%d] = [ %f, %f ]\n", HWRa.size,
	     HWRa.data[0], HWRa.data[1]);
    code = param_read_float_array((gs_param_list *) & list,
				  "PageSize", &PSa);
    if (code < 0) {
	lprintf1("reading PageSize failed! code = %d\n", code);
	exit(1);
    }
    eprintf3("PageSize[%d] = [ %f, %f ]\n", PSa.size,
	     PSa.data[0], PSa.data[1]);
    code = param_read_long((gs_param_list *) & list,
			   "MaxBitmap", &MaxBitmap);
    if (code < 0) {
	lprintf1("reading MaxBitmap failed! code = %d\n", code);
	exit(1);
    }
    eprintf1("MaxBitmap = %ld\n", MaxBitmap);
    /* Switch to param list functions to "write" */
    gs_c_param_list_write(&list, mem);
    /* Always set the PageSize. */
    PageSize[0] = 72.0 * ypage_wid;
    PageSize[1] = 72.0 * xpage_len;
    PSa.data = PageSize;
    code = param_write_float_array((gs_param_list *) & list,
				   "PageSize", &PSa);
    if (nstr.data[0] != 'v') {
	/* Set the OutputFile string file name */
	OFstr.persistent = false;
	OFstr.data = outfile;
	OFstr.size = strlen(outfile);
	code = param_write_string((gs_param_list *) & list,
				  "OutputFile", &OFstr);
	if (code < 0) {
	    lprintf1("setting OutputFile name failed, code=%d\n",
		     code);
	    exit(1);
	}
	if (nstr.data[0] == 'x') {
	    HWResolution[0] = HWResolution[1] = 72.0;
	} else {
	    HWResolution[0] = HWResolution[1] = 360.0;
	}
	HWRa.data = HWResolution;
	HWSize[0] = (int)(HWResolution[0] * ypage_wid);
	HWSize[1] = (int)(HWResolution[1] * xpage_len);
	eprintf3("	HWSize = [%d,%d], HWResolution = %f dpi\n",
		 HWSize[0], HWSize[1], HWResolution[0]);
	HWSa.data = HWSize;
	code = param_write_float_array((gs_param_list *) & list,
				       "HWResolution", &HWRa);
	code = param_write_int_array((gs_param_list *) & list,
				     "HWSize", &HWSa);
	MaxBitmap = 1000000L;
	code = param_write_long((gs_param_list *) & list,
				"MaxBitmap", &MaxBitmap);
    }
    gs_c_param_list_read(&list);
    code = gs_putdeviceparams(dev, (gs_param_list *) & list);
    eprintf1("putdeviceparams: code=%d\n", code);
    gs_c_param_list_release(&list);

    /* note: initgraphics no longer resets the color or color space */
    gs_erasepage(pgs);
    gs_initgraphics(pgs);
    {
        gs_color_space cs;

        gs_cspace_init_DeviceGray(&cs);
        gs_setcolorspace(pgs, &cs);
    }
    
    gs_clippath(pgs);
    gs_pathbbox(pgs, &cliprect);
    eprintf4("	cliprect = [[%g,%g],[%g,%g]]\n",
	     cliprect.p.x, cliprect.p.y, cliprect.q.x, cliprect.q.y);
    gs_newpath(pgs);

    switch (((rotate_value + 270) / 90) & 3) {
	default:
	case 0:		/* 0 = 360 degrees in PS == 90 degrees in printer */
	    xlate_x = cliprect.p.x;
	    xlate_y = cliprect.p.y;
	    break;
	case 1:		/* 90 degrees in PS = 180 degrees printer */
	    xlate_x = cliprect.q.x;
	    xlate_y = cliprect.p.y;
	    break;
	case 2:		/* 180 degrees in PS == 270 degrees in printer */
	    xlate_x = cliprect.q.x;
	    xlate_y = cliprect.q.y;
	    break;
	case 3:		/* 270 degrees in PS == 0 degrees in printer */
	    xlate_x = cliprect.p.x;
	    xlate_y = cliprect.q.y;
	    break;
    }
    eprintf2("translate origin to [ %f, %f ]\n", xlate_x, xlate_y);
    gs_translate(pgs, xlate_x, xlate_y);

    /* further move (before rotate) by user requested amount */
    gs_translate(pgs, 72.0 * (float)xmove_origin, 72.0 * (float)ymove_origin);

    gs_rotate(pgs, (float)rotate_value + 270.0);
    gs_scale(pgs, scale_x * 72.0 / 2032.0,
	     scale_y * 72.0 / 2032.0);
    gs_setlinecap(pgs, gs_cap_butt);
    gs_setlinejoin(pgs, gs_join_bevel);
    gs_setfilladjust(pgs, 0.0, 0.0);

    capture_exec(pgs);
    return 0;
}

#endif /* CAPTURE */
