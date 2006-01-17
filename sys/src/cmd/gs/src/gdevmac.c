/* Copyright (C) 1994-2003 artofcode LLC.  All rights reserved.
  
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

/* $Id: gdevmac.c,v 1.8 2003/04/08 12:17:17 giles Exp $ */
/* MacOS bitmap output device. This code is superceeded by
   the newer gsapi_* interface and the DISPLAY device. Please
   use that instead. See doc/API.htm for more information */
   
#include "gdevmac.h"
#include "gsparam.h"
#include "gsdll.h"



/* The device descriptor */

gx_device_procs gs_mac_procs = {
	mac_open,							/* open_device */
	mac_get_initial_matrix,				/* get_initial_matrix */
	mac_sync_output,					/* sync_output */
	mac_output_page,					/* output_page */
	mac_close,							/* close_device */
	gx_default_rgb_map_rgb_color,		/* map_rgb_color */
	gx_default_rgb_map_color_rgb,		/* map_color_rgb */
	mac_fill_rectangle,					/* fill_rectangle */
	NULL,								/* tile_rectangle */
	mac_copy_mono,				/* copy_mono */
	NULL,//	mac_copy_color,				/* copy_color */
	mac_draw_line,				/* draw_line */
	NULL,								/* get_bits */
	mac_get_params,						/* get_params */
	mac_put_params,						/* put_params */
	NULL,								/* map_cmyk_color */
	mac_get_xfont_procs,				/* get_xfont_procs */
	NULL, 								/* get_xfont_device */
	NULL, 								/* map_rgb_alpha_color */
	gx_page_device_get_page_device,		/* get_page_device */
	gx_default_get_alpha_bits, 			/* get_alpha_bits */
	mac_copy_alpha,						/* copy_alpha */
	NULL,								/* get_band */
	NULL,								/* copy_rop */
	NULL,								/* fill_path */
	NULL,								/* stroke_path */
	NULL,								/* fill_mask */
	NULL,								/* fill_trapezoid */
	NULL,								/* fill_parallelogram */
	NULL,								/* fill_triangle */
	NULL,								/* draw_thin_line */
	NULL,								/* begin_image */
	NULL,								/* image_data */
	NULL,								/* end_image */
	NULL //mac_strip_tile_rectangle			/* strip_tile_rectangle */
};



/* The instance is public. */

gx_device_macos gs_macos_device = {
	std_device_color_body(gx_device_macos,
						  &gs_mac_procs,
						  DEV_MAC_NAME,
						  DEFAULT_DEV_WIDTH, /* x and y extent (nominal) */
						  DEFAULT_DEV_HEIGHT,
	  					  DEFAULT_DEV_DPI,	/* x and y density (nominal) */
	  					  DEFAULT_DEV_DPI,
	  					  /*dci_color(*/8, 255, 256/*)*/),
	{ 0 },			/* std_procs */
	"",				/* Output Filename */
	NULL,			/* Output File */
	NULL,			/* PicHandle to "draw" into */
	NULL,			/* PicPtr */
	false,			/* outputPage */
	false,			/* use XFont interface (render with local TrueType fonts) */
	-1,				/* lastFontFace */
	-1,				/* lastFontSize */
	-1,				/* lastFontID */
	0				/* numUsedFonts */
};





private int
mac_open(register gx_device *dev)
{
	gx_device_macos				* mdev = (gx_device_macos *)dev;
	
	static short picHeader[42] = {	0x0000,									// picture size
									0x0000, 0x0000, 0x0318, 0x0264,			// bounding rect at 72dpi
									0x0011, 0x02ff, 0x0c00, 0xfffe, 0x0000,	// version/header opcodes
									0x0048, 0x0000,							// best x resolution
									0x0048, 0x0000,							// best y resolution
									0x0000, 0x0000, 0x0318, 0x0264,			// optimal src rect at 72dpi
									0x0000,									// reserved
									
									0x0000, 0x001e,							// DefHilite
									0x0008, 0x0048,							// PenMode
									0x001a, 0x0000, 0x0000, 0x0000,			// RGBFgCol = Black
									0x001b, 0xFFFF, 0xFFFF, 0xFFFF,			// RGBBkCol = White
									
									0x0001, 0x000A,							// set clipping
									0x0000, 0x0000, 0x0318, 0x0264,			// clipping rect
									0x0032, 0x0000, 0x0000, 0x0318, 0x0264	// erase rect
								};

	
	mac_set_colordepth(dev, mdev->color_info.depth);
	
	mdev->numUsedFonts = 0;
	mdev->lastFontFace = -1;
	mdev->lastFontSize = -1;
	mdev->lastFontID = -1;
	
	mdev->pic		= (PicHandle) NewHandle(500000);
	if (mdev->pic == 0)	// error, not enough memory
		return gs_error_VMerror;
	
	HLockHi((Handle) mdev->pic);	// move handle high and lock it
	
	mdev->currPicPos	= (short*) *mdev->pic;
	memcpy(mdev->currPicPos, picHeader, 42*2);
	mdev->currPicPos += 42;
	
	// enter correct dimensions and resolutions
	((short*)(*mdev->pic))[ 3]	= mdev->MediaSize[1];
	((short*)(*mdev->pic))[ 4]	= mdev->MediaSize[0];
	
	((short*)(*mdev->pic))[16] = ((short*)(*mdev->pic))[35]	= ((short*)(*mdev->pic))[40] = mdev->height;
	((short*)(*mdev->pic))[17] = ((short*)(*mdev->pic))[36] = ((short*)(*mdev->pic))[41] = mdev->width;
	
	((short*)(*mdev->pic))[10]	= (((long) X2Fix( mdev->x_pixels_per_inch )) & 0xFFFF0000) >> 16;
	((short*)(*mdev->pic))[11]	=  ((long) X2Fix( mdev->x_pixels_per_inch )) & 0x0000FFFF;
	((short*)(*mdev->pic))[12]	= (((long) X2Fix( mdev->y_pixels_per_inch )) & 0xFFFF0000) >> 16;
	((short*)(*mdev->pic))[13]	=  ((long) X2Fix( mdev->y_pixels_per_inch )) & 0x0000FFFF;
	
	// finish picture, but dont increment pointer, we want to go on drawing
	*mdev->currPicPos = 0x00ff;
	
	// notify the caller that a new device was opened
	if (pgsdll_callback)
		(*pgsdll_callback) (GSDLL_DEVICE, (char *)mdev, 1);
	
	return 0;
}



private void
mac_get_initial_matrix(register gx_device *dev, register gs_matrix *pmat)
{
	pmat->xx = dev->x_pixels_per_inch /  72.0;
	pmat->xy = 0;
	pmat->yx = 0;
	pmat->yy = dev->y_pixels_per_inch / -72.0;
	pmat->tx = 0;
	pmat->ty = dev->height;
}



/* Make the output appear on the screen. */
int
mac_sync_output(gx_device * dev)
{
	gx_device_macos		* mdev = (gx_device_macos *)dev;
	
	// finish picture, but dont increment pointer, we want to go on drawing
	*mdev->currPicPos = 0x00ff;
	
	// tell the caller to sync
	if (pgsdll_callback)
		(*pgsdll_callback) (GSDLL_SYNC, (char *)mdev, 0);
	
	return (0);
}



int
mac_output_page(gx_device * dev, int copies, int flush)
{
	gx_device_macos		* mdev = (gx_device_macos *)dev;
	int					code = 0;
	
	mdev->outputPage = true;
	
	if (strcmp(mdev->outputFileName, "")) {
		// save file
		code = mac_save_pict(dev);
	}
	
	// tell the caller that the page is done
	if (pgsdll_callback)
		(*pgsdll_callback) (GSDLL_PAGE, (char *)mdev, 0);
	
	gx_finish_output_page(dev, copies, flush);
	
	return code;
}


private int
mac_save_pict(gx_device * dev)
{
	gx_device_macos		* mdev = (gx_device_macos *)dev;
	int					code = 0;
	int					i;
	
	if (mdev->outputFile == NULL) {
		code = gx_device_open_output_file(dev, mdev->outputFileName, true, true, &(mdev->outputFile));
		if (code < 0) return code;
	}
	
	for (i=0; i<512; i++) fputc(0, mdev->outputFile);
	fwrite(*(mdev->pic), sizeof(char), ((long) mdev->currPicPos - (long) *mdev->pic + 2), mdev->outputFile);
	
	gx_device_close_output_file(dev, mdev->outputFileName, mdev->outputFile);
	mdev->outputFile = NULL;
	
	return code;
}


/* Close the device. */
private int
mac_close(register gx_device *dev)
{
	gx_device_macos	* mdev = (gx_device_macos *)dev;
	
	long	len;
	
	HUnlock((Handle) mdev->pic);	// no more changes in the pict -> unlock handle
	if (strcmp(mdev->outputFileName, "")) {
		DisposeHandle((Handle) mdev->pic);
		gx_device_close_output_file(dev, mdev->outputFileName, mdev->outputFile);
		mdev->outputFile = 0;
	} else {
		len = (long)mdev->currPicPos - (long)*mdev->pic;
		SetHandleSize((Handle) mdev->pic, len + 10);  // +10 just for the case
	}
	
	// notify the caller that the device was closed
	// it has to dispose the PICT handle when it is ready!
	if (pgsdll_callback)
		(*pgsdll_callback) (GSDLL_DEVICE, (char *)mdev, 0);
	
	return 0;
}



/* Fill a rectangle with a color. */
private int
mac_fill_rectangle(register gx_device *dev,
					  int x, int y, int w, int h,
					  gx_color_index color)
{
	gx_device_macos		* mdev = (gx_device_macos *)dev;
	
	/* ignore a fullpage rect directly after an output_page, this would clear the pict */
	if (mdev->outputPage &&
			(x == 0) && (y == 0) && (w == mdev->width) && (h == mdev->height)) {
		return 0;
	}
	
	CheckMem(1024, 100*1024);
	ResetPage();
	
	GSSetFgCol(dev, mdev->currPicPos, color);
	PICT_fillRect(mdev->currPicPos, x, y, w, h);
	
	PICT_OpEndPicGoOn(mdev->currPicPos);
	
	return 0;
}



/* Draw a line */
private int
mac_draw_line (register gx_device *dev,
				  int x0, int y0,
				  int x1, int y1,
				  gx_color_index color)
{
	gx_device_macos		* mdev = (gx_device_macos *)dev;
	
	CheckMem(1024, 100*1024);
	ResetPage();
	
	GSSetFgCol(dev, mdev->currPicPos, color);
	PICT_Line(mdev->currPicPos, x0, y0, x1, y1);
	
	PICT_OpEndPicGoOn(mdev->currPicPos);
	
	return 0;
}



/* Copy a monochrome bitmap. */
private int
mac_copy_mono (register gx_device *dev,
				  const unsigned char *base, int data_x, int raster, gx_bitmap_id id,
				  int x, int y, int w, int h,
				  gx_color_index color_0, gx_color_index color_1)
{
	gx_device_macos		* mdev = (gx_device_macos *)dev;
	
	int				byteCount = raster * h;
	short			copyMode;
	
	// this case doesn't change the picture -> return without wasting time
	if (color_0 == gx_no_color_index && color_1 == gx_no_color_index)
		return 0;
	
	fit_copy(dev, base, data_x, raster, id, x, y, w, h);
	
	CheckMem(10*1024 + byteCount*10,  100*1024 + byteCount*10);
	ResetPage();
	
	if (color_0 == gx_no_color_index)
		copyMode = srcOr;
	else if (color_1 == gx_no_color_index)
		copyMode = notSrcBic;	// this mode is untested ! (no file found which is using it)
	else
		copyMode = srcCopy;
	
	copyMode += ditherCopy;
	
	GSSetBkCol(dev, mdev->currPicPos, color_0);
	GSSetFgCol(dev, mdev->currPicPos, color_1);
	
	PICTWriteOpcode(mdev->currPicPos, 0x0098);
	PICTWriteInt(mdev->currPicPos, raster);
	PICTWriteRect(mdev->currPicPos, 0, 0, raster*8, h);
	PICTWriteRect(mdev->currPicPos, data_x, 0, w, h);
	PICTWriteRect(mdev->currPicPos, x, y, w, h);
	PICTWriteInt(mdev->currPicPos, copyMode);
	PICTWriteDataPackBits(mdev->currPicPos, base, raster, h);
	
	PICT_OpEndPicGoOn(mdev->currPicPos);
	
	return 0;
}



/* Fill a region with a color and apply a per-pixel alpha-value */
/* alpha value is simulated by changed the value to white. Full transparency means white color */
/* that's why this will only work on a fully white background!!!! */
private int
mac_copy_alpha(gx_device *dev, const unsigned char *base, int data_x,
		   int raster, gx_bitmap_id id, int x, int y, int w, int h,
		   gx_color_index color, int depth)
{
	gx_device_macos		* mdev = (gx_device_macos *)dev;
	
	ColorSpec			*colorTable;
	short				copyMode, shade, maxShade = (1 << depth) - 1, byteCount = raster * h;
	gx_color_value		rgb[3];
	colorHSV			colHSV;
	colorRGB			colRGB;
	float				saturation, value;
	
	fit_copy(dev, base, data_x, raster, id, x, y, w, h);
	
	CheckMem( byteCount*4 + 200*1024, byteCount*4 + 500*1024 );
	ResetPage();
	
	colorTable = (ColorSpec*) malloc(sizeof(ColorSpec) * (maxShade+1));
	if (colorTable == NULL)
		return gs_error_VMerror;
	
	(*dev_proc(dev, map_color_rgb))(dev, color, rgb);
	colRGB.red = rgb[0];
	colRGB.green = rgb[1];
	colRGB.blue = rgb[2];
	mac_convert_rgb_hsv(&colRGB, &colHSV);
	saturation = colHSV.s;
	value = colHSV.v;
	
	for (shade=0; shade <= maxShade; shade++) {
		colorTable[shade].value = maxShade -  shade;
		
		colHSV.s = saturation * (1.0 - (float)shade/(float)maxShade);
		colHSV.v = value + ((1.0 - value) * (float)shade/(float)maxShade);
		
		mac_convert_hsv_rgb(&colHSV, &colRGB);
		colorTable[shade].rgb.red   = colRGB.red;
		colorTable[shade].rgb.green = colRGB.green;
		colorTable[shade].rgb.blue  = colRGB.blue;
	}
	copyMode = srcCopy + ditherCopy;
	
	GSSetStdCol(mdev->currPicPos);
	
	if (raster < 8) {
		PICTWriteOpcode(mdev->currPicPos, 0x0090);
	} else {
		PICTWriteOpcode(mdev->currPicPos, 0x0098);
	}
	PICTWritePixMap(mdev->currPicPos, 0, 0, raster*8/depth, h, raster, 0, 0,
					X2Fix(mdev->x_pixels_per_inch), X2Fix(mdev->y_pixels_per_inch), depth);
	PICTWriteColorTable(mdev->currPicPos, 0, maxShade+1, colorTable);
	PICTWriteRect(mdev->currPicPos, data_x, 0, w, h);
	PICTWriteRect(mdev->currPicPos, x, y, w, h);
	PICTWriteInt(mdev->currPicPos, copyMode);
	PICTWriteDataPackBits(mdev->currPicPos, base, raster, h);
	
	PICT_OpEndPicGoOn(mdev->currPicPos);
	
	free(colorTable);
	
	return 0;
}



void
mac_convert_rgb_hsv(colorRGB *inRGB, colorHSV *HSV)
{
#define NORMALIZE_RGB(col)	((float)col/(float)0xFFFF)
	
	float	min = 1.0, temp;
	float	r = NORMALIZE_RGB(inRGB->red),
			g = NORMALIZE_RGB(inRGB->green),
			b = NORMALIZE_RGB(inRGB->blue);
	
	HSV->h = 0;
	
	HSV->v = r;
	if (g > HSV->v) HSV->v = g;
	if (b  > HSV->v) HSV->v = b;
	
	min = r;
	if (g < min) min = g;
	if (b  < min) min = b;
	
	temp = HSV->v - min;
	
	if (HSV->v > 0)
		HSV->s = temp / HSV->v;
	else
		HSV->s = 0;
	
	if (HSV->s > 0) {
		float	rd = (HSV->v - r) / temp,
				gd = (HSV->v - g) / temp,
				bd = (HSV->v - b) / temp;
		
		if (HSV->v == r) {
			if (min == g)	HSV->h = 5 + bd;
			else			HSV->h = 1 - gd;
		} else if (HSV->v == g) {
			if (min == b)	HSV->h = 1 + rd;
			else			HSV->h = 3 - bd;
		} else {
			if (min == r)	HSV->h = 3 + gd;
			else			HSV->h = 5 - rd;
		}
		
		if (HSV->h < 6)		HSV->h *= 60;
		else				HSV->h = 0;
	}
}


void
mac_convert_hsv_rgb(colorHSV *inHSV, colorRGB *RGB)
{
	if (inHSV->s == 0) {
		RGB->red = RGB->green = RGB->blue = inHSV->v * 0xFFFF;
	} else {
		float	h = inHSV->h / 60;
		int		i = trunc(h);
		float	fract = h - i;
		unsigned short	t1 = (inHSV->v * (1 - inHSV->s)) * 0xFFFF,
						t2 = (inHSV->v * (1 - inHSV->s * fract)) * 0xFFFF,
						t3 = (inHSV->v * (1 - inHSV->s * (1 - fract))) * 0xFFFF,
						v  = inHSV->v * 0xFFFF;
		
		switch(i) {
			case 0:		RGB->red   = v;
						RGB->green = t3;
						RGB->blue  = t1;
						break;
						
			case 1:		RGB->red   = t2;
						RGB->green = v;
						RGB->blue  = t1;
						break;
						
			case 2:		RGB->red   = t1;
						RGB->green = v;
						RGB->blue  = t3;
						break;
						
			case 3:		RGB->red   = t1;
						RGB->green = t2;
						RGB->blue  = v;
						break;
						
			case 4:		RGB->red   = t3;
						RGB->green = t1;
						RGB->blue  = v;
						break;
						
			case 5:		RGB->red   = v;
						RGB->green = t1;
						RGB->blue  = t2;
						break;
		}
	}
}



// set color info and procedures according to pixeldepth
private int
mac_set_colordepth(gx_device *dev, int depth)
{
	gx_device_macos				* mdev = (gx_device_macos *)dev;
	gx_device_color_info		* ci = &mdev->color_info;
	
	if (depth != 1 && depth != 4 && depth != 7 && depth != 8 && depth != 24)
		return gs_error_rangecheck;
	
	mdev->color_info.depth = depth;
	switch (depth)
	{
		case 1:		// Black/White
			ci->num_components	= 1;
			ci->max_gray		= 1;		ci->max_color		= 0;
			ci->dither_grays	= 2;		ci->dither_colors	= 0;
			set_dev_proc(dev, map_rgb_color, gx_default_b_w_map_rgb_color);
			set_dev_proc(dev, map_color_rgb, gx_default_b_w_map_color_rgb);
			break;
			
		case 4:		// 4Bit-Gray
			ci->num_components	= 1;
			ci->max_gray		= 15;		ci->max_color		= 0;
			ci->dither_grays	= 16;		ci->dither_colors	= 0;
			set_dev_proc(dev, map_rgb_color, gx_default_gray_map_rgb_color);
			set_dev_proc(dev, map_color_rgb, gx_default_gray_map_color_rgb);
			break;
		
		case 7:		// 8Bit-Gray
			ci->depth			= 7;
			ci->num_components	= 1;
			ci->max_gray		= 255;		ci->max_color		= 0;
			ci->dither_grays	= 256;		ci->dither_colors	= 0;
			set_dev_proc(dev, map_rgb_color, gx_default_gray_map_rgb_color);
			set_dev_proc(dev, map_color_rgb, gx_default_gray_map_color_rgb);
			break;
		
		case 8:		// 8Bit-Color
			ci->num_components	= 3;
			ci->max_gray		= 15;		ci->max_color		= 5;
			ci->dither_grays	= 16;		ci->dither_colors	= 6;
			set_dev_proc(dev, map_rgb_color, gx_default_rgb_map_rgb_color);
			set_dev_proc(dev, map_color_rgb, gx_default_rgb_map_color_rgb);
			break;
		
/*		case 16:	// 16Bit-Color
			ci->num_components	= 3;
			ci->max_gray		= 255;		ci->max_color		= 65535;
			ci->dither_grays	= 256;		ci->dither_colors	= 65536;
			set_dev_proc(dev, map_rgb_color, gx_default_rgb_map_rgb_color);
			set_dev_proc(dev, map_color_rgb, gx_default_rgb_map_color_rgb);
			break;
*/		
		case 24:	// 24Bit-Color
			ci->num_components	= 3;
			ci->max_gray		= 255;		ci->max_color		= 16777215;
			ci->dither_grays	= 256;		ci->dither_colors	= 16777216;
			set_dev_proc(dev, map_rgb_color, gx_default_rgb_map_rgb_color);
			set_dev_proc(dev, map_color_rgb, gx_default_rgb_map_color_rgb);
			break;
	}
	
	return 0;
}


private int
mac_put_params(gx_device *dev, gs_param_list *plist)
{	
	gx_device_macos		*mdev	= (gx_device_macos *)dev;
	
	int						isOpen = mdev->is_open;
	int						code;
    bool					useXFonts;
	int						depth;
    gs_param_string			outputFile;
	
	// Get the UseExternalFonts Parameter
	code = param_read_bool(plist, "UseExternalFonts", &useXFonts);
	if (!code)
		mdev->useXFonts = useXFonts;
	
	// Get the BitsPerPixel Parameter
	code = param_read_int(plist, "BitsPerPixel", &depth);
	if (!code) {
		code = mac_set_colordepth(dev, depth);
		if (code)
			param_return_error(plist, "BitsPerPixel", gs_error_rangecheck);
	}
	
	// Get OutputFile
	code = param_read_string(plist, "OutputFile", &outputFile);
	if (code < 0) {
		param_signal_error(plist, "OutputFile", code);
		return code;
	} else if (code == 0) {
		
		if (dev->LockSafetyParams &&
			bytes_compare(outputFile.data, outputFile.size,
			    (const byte *)mdev->outputFileName, strlen(mdev->outputFileName))) {
			param_signal_error(plist, "OutputFile", gs_error_invalidaccess);
			return gs_error_invalidaccess;
		}
		if (outputFile.size > (gp_file_name_sizeof - 1)) {
			param_signal_error(plist, "OutputFile", gs_error_limitcheck);
			return gs_error_limitcheck;
		}
		
		/* If filename changed, close file. */
		if (outputFile.data != 0 &&
				bytes_compare(outputFile.data, outputFile.size,
					(const byte *)mdev->outputFileName, strlen(mdev->outputFileName))) {
			/* Close the file if it's open. */
			if (mdev->outputFile != NULL) {
		    	gx_device_close_output_file(dev, mdev->outputFileName, mdev->outputFile);
				memcpy(mdev->outputFileName, outputFile.data, outputFile.size);
				mdev->outputFileName[outputFile.size] = 0;
				gx_device_open_output_file(dev, mdev->outputFileName, true, true, &(mdev->outputFile));
		    } else {
				memcpy(mdev->outputFileName, outputFile.data, outputFile.size);
				mdev->outputFileName[outputFile.size] = 0;
		    }
	    }
	}
	
	// Get the Default Parameters
	mdev->is_open = 0;
	code = gx_default_put_params( dev, plist );
	mdev->is_open = isOpen;
	
	return code;
}


private int
mac_get_params(gx_device *dev, gs_param_list *plist)
{
	gx_device_macos		*mdev	= (gx_device_macos *)dev;
	
	int						code;
    gs_param_string			outputFile;
	
	code = gx_default_get_params(dev, plist);
	if (code < 0)
		return code;
	
	// UseExternalFonts
	code = param_write_bool(plist, "UseExternalFonts", &(mdev->useXFonts));
	
	// color depth
	code = param_write_int(plist, "BitsPerPixel", &(mdev->color_info.depth));
	
	// output file name
	outputFile.data = (const byte *) mdev->outputFileName;
	outputFile.size = strlen(mdev->outputFileName);
	outputFile.persistent = false;
	code = param_write_string(plist, "OutputFile", &outputFile);
	
	return code;
}



/* let the caller get the device PictHandle, he has to draw it to screen */
int GSDLLAPI
gsdll_get_pict(unsigned char *dev, PicHandle *thePict)
{
	gx_device_macos			* mdev = (gx_device_macos *)dev;
	
	*thePict = mdev->pic;
	
	return 0;
}






/*************************************************************************************/
/*************************************************************************************/
/**		Experimental functions!														**/
/*************************************************************************************/
/*************************************************************************************/




#if 0
/* NOT FUNCTIONAL !!! */
/* Copy a color bitmap. */
private int
mac_copy_color (register gx_device *dev,
				const unsigned char *base, int data_x, int raster,  gx_bitmap_id id,
				int x, int y, int w, int h)
{
	gx_device_macos		* mdev = (gx_device_macos *)dev;
	
	int				byteCount = raster * h, color;
	gx_color_value	rgb[3];
	
	fit_copy(dev, base, data_x, raster, id, x, y, w, h);
	
	CheckMem(10*1024 + byteCount*4, 100*1024 + byteCount*4);
	ResetPage();
	
	GSSetStdCol(mdev->currPicPos);		// Sets FgCol to Black and BkCol to White
	
	if (mdev->color_info.depth == 24) {
		PICTWriteOpcode(mdev->currPicPos, 0x009A);
		PICTWriteLong(mdev->currPicPos, 0x000000FF);
		PICTWritePixMap(mdev->currPicPos, 0, 0, raster/4, h, raster, 2, 0,
						X2Fix(mdev->x_pixels_per_inch), X2Fix(mdev->y_pixels_per_inch), 32);
		PICTWriteRect(mdev->currPicPos, data_x, 0, w, h);
		PICTWriteRect(mdev->currPicPos, x, y, w, h);
		PICTWriteInt(mdev->currPicPos, srcCopy);
		
/*		memcpy(mdev->currPicPos, base, byteCount);
		(char*)(mdev->currPicPos) += byteCount;*/
		
		{
			short	i;
			byteCount = 0;
			
			for (i=0; i<raster/4*h; i++) {
			//	PICTWriteByte(mdev->currPicPos, 0x00);
				PICTWriteByte(mdev->currPicPos, 0x00);
				PICTWriteByte(mdev->currPicPos, 0x00);
				PICTWriteByte(mdev->currPicPos, 0x00);
				byteCount += 3;
			}
		}
		
		if (byteCount % 2)
			PICTWriteFillByte(mdev->currPicPos);
		
	} else if (mdev->color_info.depth <= 8) {
		ColorSpec		*colorTable;
		
		colorTable = (ColorSpec*) malloc(sizeof(ColorSpec) * (1 << mdev->color_info.depth));
		for (color=0; color < (1 << mdev->color_info.depth); color++) {
			(*dev_proc(dev, map_color_rgb))(dev, color, rgb);
			colorTable[color].value		= color;
			colorTable[color].rgb.red	= rgb[0];
			colorTable[color].rgb.green	= rgb[1];
			colorTable[color].rgb.blue	= rgb[2];
		}
		
		PICTWriteOpcode(mdev->currPicPos, 0x0098);
		PICTWritePixMap(mdev->currPicPos, 0, 0, raster*8/mdev->color_info.depth, h, raster, 1, 0,
						X2Fix(mdev->x_pixels_per_inch), X2Fix(mdev->y_pixels_per_inch),
						mdev->color_info.depth);
		PICTWriteColorTable(mdev->currPicPos, 0, (1 << mdev->color_info.depth), colorTable);
		PICTWriteRect(mdev->currPicPos, data_x, 0, w, h);
		PICTWriteRect(mdev->currPicPos, x, y, w, h);
		PICTWriteInt(mdev->currPicPos, srcCopy);
		
		PICTWriteDataPackBits(mdev->currPicPos, base, raster, h);
		
		free(colorTable);
	} else {
		gx_default_copy_color( dev, base, data_x, raster, id, x, y, w, h );
	}
	
	PICT_OpEndPicGoOn(mdev->currPicPos);
	
	return 0;
}
#endif



#if 0
/* tile a rectangle with a bitmap or pixmap */
private int
mac_strip_tile_rectangle(register gx_device *dev, const gx_strip_bitmap *tile,
						 int x, int y, int w, int h,
						 gx_color_index color_0, gx_color_index color_1,
						 int phase_x, int phase_y)
{
	gx_device_macos		* mdev = (gx_device_macos *)dev;
	
	int					byteCount = tile->raster * tile->size.y;
/*	
	// tile is a pixmap
	if (color_0 == gx_no_color_index && color_1 == gx_no_color_index)
		return 0;// gx_default_strip_tile_rectangle(dev, tile, x, y, w, h, color_0, color_1, phase_x, phase_y);
	
	if (color_0 != gx_no_color_index && color_1 != gx_no_color_index) {
		// monochrome tiles
		if (phase_x != 0 ||Êphase_y != 0 || tile->shift != 0 ||
				tile->strip_height != 0 ||Êtile->strip_shift != 0) {
			return gx_default_strip_tile_rectangle(dev, tile, x, y, w, h, color_0, color_1, phase_x, phase_y);
		}
		
	} else {
		return 0;//gx_default_strip_tile_rectangle(dev, tile, x, y, w, h, color_0, color_1, phase_x, phase_y);
	}
*/
}
#endif   










