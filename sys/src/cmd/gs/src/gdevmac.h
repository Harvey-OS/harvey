/* Copyright (C) 1994, 1995, 1997 Artifex Software, Inc.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND. No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Free Public License (the 
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC. The License grants you
  the right to copy, modify and redistribute AFPL Ghostscript, but only
  under certain conditions described in the License. Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/


#ifndef gdevmac_INCLUDED
#  define gdevmac_INCLUDED


#include "gx.h"
#include "gxdevice.h"
#include "gp.h"
#include "gp_mac.h"
#include "gsdll.h"

#include "gsutil.h"
#include "gxxfont.h"
#include "gsstruct.h"
#include "gserrors.h"

#include "gdevmacpictop.h"
#include <Fonts.h>
#include <FixMath.h>
#include <Resources.h>
#include "math_.h"
#include <string.h>
#include <stdlib.h>



/* Default device settings */

#define DEFAULT_PAGE_SIZE_H		8.5
#define DEFAULT_PAGE_SIZE_V		11.0
#define DEFAULT_DEV_DPI			72.0
#define DEFAULT_PAGE_DPI		72.0

#define DEFAULT_DEV_WIDTH  		(DEFAULT_PAGE_SIZE_H * DEFAULT_DEV_DPI)
#define DEFAULT_DEV_HEIGHT 		(DEFAULT_PAGE_SIZE_V * DEFAULT_DEV_DPI)
#define DEFAULT_PAGE_WIDTH  	(DEFAULT_PAGE_SIZE_H * DEFAULT_PAGE_DPI)
#define DEFAULT_PAGE_HEIGHT 	(DEFAULT_PAGE_SIZE_V * DEFAULT_PAGE_DPI)


/* Define the Macintosh device */

typedef struct gx_device_macos_s
{
	gx_device_common;
	char			outputFileName[gp_file_name_sizeof];
	FILE			*outputFile;
	PicHandle		pic;
	short			*currPicPos;
	bool			outputPage;
	bool			useXFonts;
	FMFontStyle		lastFontFace;
	FMFontSize		lastFontSize;
	FMFontFamily	lastFontID;
	int				numUsedFonts;
	FMFontFamily	usedFontIDs[256];
} gx_device_macos;

#define DEV_MAC_NAME	"macos"


/* Device Procedures */

dev_proc_open_device(mac_open);
dev_proc_get_initial_matrix(mac_get_initial_matrix);
dev_proc_sync_output(mac_sync_output);
dev_proc_output_page(mac_output_page);
dev_proc_get_params(mac_get_params);
dev_proc_put_params(mac_put_params);
dev_proc_close_device(mac_close);
dev_proc_fill_rectangle(mac_fill_rectangle);
dev_proc_strip_tile_rectangle(mac_strip_tile_rectangle);
dev_proc_copy_mono(mac_copy_mono);
dev_proc_copy_color(mac_copy_color);
dev_proc_draw_line(mac_draw_line);
dev_proc_copy_alpha(mac_copy_alpha);
dev_proc_get_xfont_procs(mac_get_xfont_procs);

/* Declare the xfont procedures */
private xfont_proc_lookup_font(mac_lookup_font);
private xfont_proc_char_xglyph(mac_char_xglyph);
private xfont_proc_char_metrics(mac_char_metrics);
private xfont_proc_render_char(mac_render_char);
private xfont_proc_release(mac_release);


/* Define a MacOS xfont. */

typedef struct mac_xfont_s mac_xfont;
struct mac_xfont_s {
	gx_xfont_common	common;
	gx_device		*dev;
	Str255			fontName;
	FMFontFamily	fontID;
	FMFontStyle		fontFace;
	FMFontSize		fontSize;
	int				fontEncoding;
	FMetricRec		fontMetrics;
};


/* Memory handling macros */

#define	CheckMem(a,b)																\
		{																			\
			long	offset = (long) mdev->currPicPos - (long) *mdev->pic;			\
			long	len = GetHandleSize((Handle) mdev->pic);						\
			if (len - offset < a) {													\
				HUnlock((Handle) mdev->pic);										\
				SetHandleSize((Handle) mdev->pic, len + b);							\
				if (MemError() != noErr) return gs_error_VMerror;					\
				HLockHi((Handle) mdev->pic);										\
				mdev->currPicPos = (short*) ((long) *mdev->pic + offset);			\
			}																		\
		}

#define ResetPage()																	\
		{																			\
			if (mdev->outputPage) {													\
				mdev->outputPage = false;											\
				mdev->currPicPos = (short*) *mdev->pic;								\
				mdev->currPicPos += 42; /* header len */							\
				mdev->lastFontID = mdev->lastFontSize = mdev->lastFontFace = -1;	\
				mdev->numUsedFonts = 0;												\
			}																		\
		}


/* Other datatypes */

typedef struct {
	unsigned short red;
	unsigned short green;
	unsigned short blue;
} colorRGB;

typedef struct {
	float h;
	float s;
	float v;
} colorHSV;


/* Helper function definitions */

private int		mac_save_pict(gx_device * dev);
private void	mac_convert_rgb_hsv(colorRGB *inRGB, colorHSV *HSV);
private void	mac_convert_hsv_rgb(colorHSV *inHSV, colorRGB *RGB);

private void	mac_find_font_family(ConstStringPtr fname, int len,
									 FMFontFamily *fontFamilyID, FMFontStyle *fontFace);
private int		mac_get_font_encoding(mac_xfont *macxf);
private void	mac_get_font_resource(mac_xfont *macxf, ResType *resType, short *resID);

private int     mac_set_colordepth(gx_device *dev, int depth);


/* additional DLL function definition */

#pragma export on

int GSDLLAPI gsdll_get_pict(unsigned char *, PicHandle *);
typedef int (GSDLLAPI * PFN_gsdll_get_pict) (unsigned char *, PicHandle *);

#pragma export off

#endif 				/* gdevmac_INCLUDED */
