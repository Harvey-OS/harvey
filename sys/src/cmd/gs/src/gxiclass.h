/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*$Id: gxiclass.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Define image rendering algorithm classes */

#ifndef gxiclass_INCLUDED
#  define gxiclass_INCLUDED

/* Define the abstract type for the image enumerator state. */
typedef struct gx_image_enum_s gx_image_enum;

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

/*
 * Define the interface for routines used to render a (source) scan line.
 * If the buffer is the original client's input data, it may be unaligned;
 * otherwise, it will always be aligned.
 *
 * The image_render procedures work on fully expanded, complete rows.  These
 * take a height argument, which is an integer >= 0; they return a negative
 * code, or the number of rows actually processed (which may be less than
 * the height).  height = 0 is a special call to indicate that there is no
 * more input data; this is necessary because the last scan lines of the
 * source data may not produce any output.
 *
 * Note that the 'w' argument of the image_render procedure is the number
 * of samples, i.e., the number of pixels * the number of samples per pixel.
 * This is neither the width in pixels nor the width in bytes (in the case
 * of 12-bit samples, which expand to 2 bytes apiece).
 */
#define irender_proc(proc)\
  int proc(P6(gx_image_enum *penum, const byte *buffer, int data_x,\
	      uint w, int h, gx_device *dev))
typedef irender_proc((*irender_proc_t));

/*
 * Define procedures for selecting imaging methods according to the class of
 * the image.  Image class procedures are called in alphabetical order, so
 * their names begin with a digit that indicates their priority
 * (0_interpolate, etc.): each one may assume that all the previous ones
 * failed.  If a class procedure succeeds, it may update the enumerator
 * structure as well as returning the rendering procedure.
 */
#define iclass_proc(proc)\
  irender_proc_t proc(P1(gx_image_enum *penum))
typedef iclass_proc((*gx_image_class_t));

#endif /* gxiclass_INCLUDED */
