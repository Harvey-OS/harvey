/* Copyright (C) 1994, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsdevice.h,v 1.6 2002/06/16 08:45:42 lpd Exp $ */
/* Device and page control API */

#ifndef gsdevice_INCLUDED
#  define gsdevice_INCLUDED

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

#ifndef gx_device_memory_DEFINED
#  define gx_device_memory_DEFINED
typedef struct gx_device_memory_s gx_device_memory;
#endif

#ifndef gs_matrix_DEFINED
#  define gs_matrix_DEFINED
typedef struct gs_matrix_s gs_matrix;
#endif

#ifndef gs_param_list_DEFINED
#  define gs_param_list_DEFINED
typedef struct gs_param_list_s gs_param_list;
#endif

/* Device procedures not involving a graphics state. */

int gs_opendevice(gx_device *);
int gs_copyscanlines(gx_device *, int, byte *, uint, int *, uint *);
const gx_device *gs_getdevice(int);
int gs_copydevice(gx_device **, const gx_device *, gs_memory_t *);
/*
 * If keep_open is true and dev->is_open is true, the copy *may* have
 * is_open = true; otherwise, the copy will have is_open = false.
 * copydevice is equivalent to copydevice2 with keep_open = false.
 */
int gs_copydevice2(gx_device **pnew_dev, const gx_device *dev,
		   bool keep_open, gs_memory_t *mem);

#define gs_makeimagedevice(pdev, pmat, w, h, colors, colors_size, mem)\
  gs_makewordimagedevice(pdev, pmat, w, h, colors, colors_size, false, true, mem)
int gs_makewordimagedevice(gx_device ** pnew_dev, const gs_matrix * pmat,
			   uint width, uint height,
			   const byte * colors, int num_colors,
			   bool word_oriented, bool page_device,
			   gs_memory_t * mem);

#define gs_initialize_imagedevice(mdev, pmat, w, h, colors, colors_size, mem)\
  gs_initialize_wordimagedevice(mdev, pmat, w, h, colors, color_size, false, true, mem)
int gs_initialize_wordimagedevice(gx_device_memory * new_dev,
				  const gs_matrix * pmat,
				  uint width, uint height,
				  const byte * colors, int colors_size,
				  bool word_oriented, bool page_device,
				  gs_memory_t * mem);
const char *gs_devicename(const gx_device *);
void gs_deviceinitialmatrix(gx_device *, gs_matrix *);

/* VMS limits identifiers to 31 characters. */
int gs_get_device_or_hw_params(gx_device *, gs_param_list *, bool);
#define gs_getdeviceparams(dev, plist)\
  gs_get_device_or_hw_params(dev, plist, false)
#define gs_gethardwareparams(dev, plist)\
  gs_get_device_or_hw_params(dev, plist, true)
/* BACKWARD COMPATIBILITY */
#define gs_get_device_or_hardware_params gs_get_device_or_hw_params

int gs_putdeviceparams(gx_device *, gs_param_list *);
int gs_closedevice(gx_device *);

/* Device procedures involving an imager state. */

#ifndef gs_imager_state_DEFINED
#  define gs_imager_state_DEFINED
typedef struct gs_imager_state_s gs_imager_state;
#endif

int gs_imager_putdeviceparams(gs_imager_state *pis, gx_device *dev,
			      gs_param_list *plist);

/* Device procedures involving a graphics state. */

#ifndef gs_state_DEFINED
#  define gs_state_DEFINED
typedef struct gs_state_s gs_state;
#endif

int gs_flushpage(gs_state *);
int gs_copypage(gs_state *);
int gs_output_page(gs_state *, int, int);
int gs_nulldevice(gs_state *);
int gs_setdevice(gs_state *, gx_device *);
int gs_setdevice_no_erase(gs_state *, gx_device *);		/* returns 1 */
						/* if erasepage required */
int gs_setdevice_no_init(gs_state *, gx_device *);
gx_device *gs_currentdevice(const gs_state *);

/* gzstate.h redefines the following: */
#ifndef gs_currentdevice_inline
#  define gs_currentdevice_inline(pgs) gs_currentdevice(pgs)
#endif

int gs_state_putdeviceparams(gs_state *pgs, gs_param_list *plist);

#endif /* gsdevice_INCLUDED */
