/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevmswn.c */
/*
 * Microsoft Windows 3.n driver for Ghostscript.
 * Original version by Russell Lang and Maurice Castro with help from
 * Programming Windows, 2nd Ed., Charles Petzold, Microsoft Press;
 * created from gdevbgi.c and gnuplot/term/win.trm 5th June 1992.
 * Extensively modified by L. Peter Deutsch, Aladdin Enterprises.
 */
#include "gdevmswn.h"
#include "gp.h"
#include "gpcheck.h"
#include "gsparam.h"
#include "gdevpccm.h"
#ifdef __DLL__
#include "gsdll.h"
#endif

/* Forward references */
private void near win_makeimg(P1(gx_device_win *));
private int win_set_bits_per_pixel(P2(gx_device_win *, int));
LRESULT CALLBACK _export WndImgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK _export WndImgChildProc(HWND, UINT, WPARAM, LPARAM);

#define TIMER_ID 1

/* Open the win driver */
int
win_open(gx_device *dev)
{	HDC hdc;
	WNDCLASS wndclass;
	static BOOL registered = FALSE;

	/* Initialize the scrolling information. */
	
	wdev->cxClient = wdev->cyClient = 0;
	wdev->nVscrollPos = wdev->nVscrollMax = 0;
	wdev->nHscrollPos = wdev->nHscrollMax = 0;

	wdev->update = wdev->timer = FALSE;

	if (dev->width == INITIAL_WIDTH)
	  dev->width  = (int)(8.5  * dev->x_pixels_per_inch);
	if (dev->height == INITIAL_HEIGHT)
	  dev->height = (int)(11.0 * dev->y_pixels_per_inch);

	if (!wdev->dll) {
	  /* If this is the first instance, register the window classes. */
	  if (!registered) {
	    /* register the window class for graphics */
	    wndclass.style = CS_HREDRAW | CS_VREDRAW;
	    wndclass.lpfnWndProc = WndImgChildProc;
	    wndclass.cbClsExtra = 0;
	    wndclass.cbWndExtra = sizeof(LONG);
	    wndclass.hInstance = phInstance;
	    wndclass.hIcon = LoadIcon(phInstance,(LPSTR)MAKEINTRESOURCE(IMAGE_ICON));
	    wndclass.hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
	    wndclass.hbrBackground = GetStockObject(WHITE_BRUSH);
	    wndclass.lpszMenuName = NULL;
	    wndclass.lpszClassName = szAppName;
	    RegisterClass(&wndclass);

	    wndclass.style = CS_HREDRAW | CS_VREDRAW;
	    wndclass.lpfnWndProc = WndImgProc;
	    wndclass.cbClsExtra = 0;
	    wndclass.cbWndExtra = sizeof(LONG);
	    wndclass.hInstance = phInstance;
	    wndclass.hIcon = LoadIcon(phInstance,(LPSTR)MAKEINTRESOURCE(IMAGE_ICON));
	    wndclass.hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
	    wndclass.hbrBackground = GetStockObject(WHITE_BRUSH);
	    wndclass.lpszMenuName = NULL;
	    wndclass.lpszClassName = szImgName;
	    RegisterClass(&wndclass);
	
	    registered = TRUE;
	  }

	  win_makeimg(wdev);
	} /* !wdev->dll */

	wdev->hdctext = NULL;

	if (wdev->BitsPerPixel == 0) {
	    /* Set parameters that were unknown before opening device */
	    /* Find out if the device supports color */
	    /* We recognize 1, 4, 8, 15, 16 and 24 bit/pixel devices */
	    hdc = GetDC(NULL);	/* get hdc for desktop */
	    wdev->BitsPerPixel = GetDeviceCaps(hdc,PLANES) * GetDeviceCaps(hdc,BITSPIXEL);
	    ReleaseDC(NULL,hdc);
	    wdev->mapped_color_flags = 0;
	}

	if (win_set_bits_per_pixel(wdev, wdev->BitsPerPixel) < 0)
	    return gs_error_limitcheck;

	if (wdev->nColors > 0) {
	    /* create palette for display */
	    if ((wdev->limgpalette = win_makepalette(wdev))
		== (LPLOGPALETTE)NULL)
		return win_nomemory();
	    wdev->himgpalette = CreatePalette(wdev->limgpalette);
	}

	return 0;
}

/* Make the output appear on the screen. */
int
win_sync_output(gx_device *dev)
{
    if (wdev->timer)
	KillTimer(wdev->hwndimgchild, TIMER_ID);
    wdev->timer = FALSE;
    wdev->update = FALSE;
#ifdef __DLL__
    if (wdev->dll) {
	(*pgsdll_callback)(GSDLL_SYNC, (unsigned char *)wdev, 0);
	return 0;
    }
#endif

    if (gsview) {
	SendMessage(gsview_hwnd, WM_GSVIEW, SYNC_OUTPUT, (LPARAM)NULL);
    }
    else {
	if ( !IsWindow(wdev->hwndimg) ) {  /* some clod closed the window */
		win_makeimg(wdev);
	}
	if ( !IsIconic(wdev->hwndimg) ) {  /* redraw window */
		InvalidateRect(wdev->hwndimg, NULL, 1);
		UpdateWindow(wdev->hwndimg);
	}
    }
    return(0);
}

/* Make the window visible, and display the output. */
int
win_output_page(gx_device *dev, int copies, int flush)
{
	if (wdev->timer)
	    KillTimer(wdev->hwndimgchild, TIMER_ID);
	wdev->timer = FALSE;
	wdev->update = FALSE;

#ifdef __DLL__
        if (wdev->dll) {
	    (*pgsdll_callback)(GSDLL_PAGE, (unsigned char *)wdev, 0);
	    return 0;
        }
#endif

	if (gsview) {
	    if (copies == -2) {
	        SendMessage(gsview_hwnd, WM_GSVIEW, END, (LPARAM)NULL);
	    }
	    else if (copies == -1) {
	        SendMessage(gsview_hwnd, WM_GSVIEW, BEGIN, (LPARAM)NULL);
	    }
	    else {
	        SendMessage(gsview_hwnd, WM_GSVIEW, OUTPUT_PAGE, (LPARAM)NULL);
	        gsview_next = FALSE;
	        /* wait for NEXT_PAGE message */
	        while (!gsview_next)
		    process_interrupts();
	    }
	    return(0);
	}
	else {
	    if (IsIconic(wdev->hwndimg))    /* useless as an Icon so fix it */
		ShowWindow(wdev->hwndimg, SW_SHOWNORMAL);
	    BringWindowToTop(wdev->hwndimg);
	}
	return( win_sync_output(dev) );
}

/* Close the win driver */
int
win_close(gx_device *dev)
{
	/* Free resources */
	if (wdev->timer)
	    KillTimer(wdev->hwndimgchild, TIMER_ID);
	wdev->timer = FALSE;
	wdev->update = FALSE;

	if (wdev->nColors > 0) {
	    gs_free(wdev->mapped_color_flags, 4096, 1, "win_set_bits_per_pixel");
	    DeleteObject(wdev->himgpalette);
	    gs_free((char *)(wdev->limgpalette), 1, sizeof(LOGPALETTE) + 
		(1<<(wdev->color_info.depth)) * sizeof(PALETTEENTRY),
		"win_close");
	}

	if (!wdev->dll) {
	    if (gsview)
		DestroyWindow(wdev->hwndimgchild);
	    else
		DestroyWindow(wdev->hwndimg);
	    process_interrupts();	/* process WIN_DESTROY message */
	}

	return(0);
}

/* Map a r-g-b color to the colors available under Windows */
gx_color_index
win_map_rgb_color(gx_device *dev, gx_color_value r, gx_color_value g,
  gx_color_value b)
{
	if (!dev->is_open)
	    return 0;
	switch(wdev->BitsPerPixel) {
	  case 24:
		return (((unsigned long)b >> (gx_color_value_bits - 8)) << 16) +
		       (((unsigned long)g >> (gx_color_value_bits - 8)) << 8) +
		       (((unsigned long)r >> (gx_color_value_bits - 8)));
	  case 16:
	  case 15:
		return ((unsigned long)win_color_value(b) << 16) +
		       ((unsigned long)win_color_value(g) << 8) +
		       ((unsigned long)win_color_value(r));
	  case 8: {
		int i;
		LPLOGPALETTE lpal = wdev->limgpalette;
		PALETTEENTRY *pep;
		byte cr, cg, cb;
		int mc_index;
		byte mc_mask;

		/* Check for a color in the palette of 64. */
		{	static const byte pal64[32] = {
				1, 0, 0, 0, 0,  0, 0, 0, 0, 0,
				1, 0, 0, 0, 0,  0, 0, 0, 0, 0,  0,
				1, 0, 0, 0, 0,  0, 0, 0, 0, 0,
				1
			};
			if ( pal64[r >> (gx_color_value_bits - 5)] &&
			     pal64[g >> (gx_color_value_bits - 5)] &&
			     pal64[b >> (gx_color_value_bits - 5)]
			   )
				return (gx_color_index)(
				  ((r >> (gx_color_value_bits - 2)) << 4) +
				  ((g >> (gx_color_value_bits - 2)) << 2) +
				  (b >> (gx_color_value_bits - 2))
				);
		}

		/* map colors to 0->255 in 32 steps */
		cr = win_color_value(r);
		cg = win_color_value(g);
		cb = win_color_value(b);

		/* Search in palette, skipping the first 64. */
		mc_index = ((cr >> 3) << 7) + ((cg >> 3) << 2) + (cb >> 6);
		mc_mask = 0x80 >> ((cb >> 3) & 7);
		if ( wdev->mapped_color_flags[mc_index] & mc_mask )
		  for ( i = wdev->nColors, pep = &lpal->palPalEntry[i];
		      --pep, --i >= 64;
		      )
		{	if ( cr == pep->peRed &&
			     cg == pep->peGreen &&
			     cb == pep->peBlue
			   )
				return((gx_color_index)i);	/* found it */
		}
		
		/* next try adding it to palette */
		i = wdev->nColors;
		if (i < 220) { /* allow 36 for windows and other apps */
			LPLOGPALETTE lipal = wdev->limgpalette;
			wdev->nColors = i+1;

			DeleteObject(wdev->himgpalette);
	 		lipal->palPalEntry[i].peFlags = 0;
			lipal->palPalEntry[i].peRed   =  cr;
			lipal->palPalEntry[i].peGreen =  cg;
			lipal->palPalEntry[i].peBlue  =  cb;
			lipal->palNumEntries = wdev->nColors;
			wdev->himgpalette = CreatePalette(lipal);

			wdev->mapped_color_flags[mc_index] |= mc_mask;
			return((gx_color_index)i);	/* return new palette index */
		}

		return(gx_no_color_index);  /* not found - dither instead */
		}
	  case 4:
		if ((r == g) && (g == b) && (r >= gx_max_color_value / 3 * 2 - 1)
		   && (r < gx_max_color_value / 4 * 3))
			return ((gx_color_index)8);	/* light gray */
		return pc_4bit_map_rgb_color(dev, r, g, b);
	}
	return (gx_default_map_rgb_color(dev,r,g,b));
}

/* Map a color code to r-g-b. */
int
win_map_color_rgb(gx_device *dev, gx_color_index color,
  gx_color_value prgb[3])
{	gx_color_value one;
	if (!dev->is_open)
	    return 0;
	switch(wdev->BitsPerPixel) {
	  case 24:
	  case 16:
	  case 15:
		one = (gx_color_value) (gx_max_color_value / 255);
		prgb[0] = ((color)     & 255) * one;
		prgb[1] = ((color>>8)  & 255) * one;
		prgb[2] = ((color>>16) & 255) * one;
		break;
	  case 8:
		one = (gx_color_value) (gx_max_color_value / 255);
		prgb[0] = wdev->limgpalette->palPalEntry[(int)color].peRed * one;
		prgb[1] = wdev->limgpalette->palPalEntry[(int)color].peGreen * one;
		prgb[2] = wdev->limgpalette->palPalEntry[(int)color].peBlue * one;
		break;
	  case 4:
		if (color == 8)	/* VGA light gray */
		    prgb[0] = prgb[1] = prgb[2] = (gx_max_color_value / 4 * 3);
		else
		    pc_4bit_map_color_rgb(dev, color, prgb);
		break;
	  default:
		prgb[0] = prgb[1] = prgb[2] = 
			(int)color ? gx_max_color_value : 0;
	}
	return 0;
}

/* Get Win parameters */
int
win_get_params(gx_device *dev, gs_param_list *plist)
{	int code = gx_default_get_params(dev, plist);
	gs_param_string gvs;
	gvs.data = wdev->GSVIEW_STR, gvs.size = strlen(gvs.data),
	  gvs.persistent = false;
	code < 0 ||
	(code = param_write_int(plist, "BitsPerPixel", &wdev->BitsPerPixel)) < 0 ||
	(code = param_write_int(plist, "UpdateInterval", &wdev->UpdateInterval)) < 0 ||
	(code = param_write_string(plist, "GSVIEW", &gvs)) < 0;
	return code;
}

/* Set window parameters -- size and resolution. */
/* We implement this ourselves so that we can do it without */
/* closing and opening the device. */
int
win_put_params(gx_device *dev, gs_param_list *plist)
{	int ecode = 0, code;
	bool is_open = dev->is_open;
	int width = dev->width;
	int height = dev->height;
	int old_bpp = dev->color_info.depth;
	int bpp = old_bpp;
	byte *old_flags = wdev->mapped_color_flags;
	int uii = wdev->UpdateInterval;
	gs_param_string gsvs;

	/* Handle extra parameters */
	switch ( code = param_read_string(plist, "GSVIEW", &gsvs) )
	{
	case 0:
		if ( dev->is_open )
		  ecode = gs_error_rangecheck;
		else if ( gsvs.size >= win_gsview_sizeof )
		  ecode = gs_error_limitcheck;
		else
		  break;
		goto gsve;
	default:
		ecode = code;
gsve:		param_signal_error(plist, "GSVIEW", ecode);
	case 1:
		gsvs.data = 0;
		break;
	}

	switch ( code = param_read_int(plist, "UpdateInterval", &uii) )
	{
	case 0:
		if ( uii < 0 )
		  ecode = gs_error_rangecheck;
		else
		  break;
		goto uie;
	default:
		ecode = code;
uie:		param_signal_error(plist, "UpdateInterval", ecode);
	case 1:
		break;
	}

	switch ( code = param_read_int(plist, "BitsPerPixel", &bpp) )
	{
	case 0:
		if ( dev->is_open )
		  ecode = gs_error_rangecheck;
		else
		  {	/* Don't release existing mapped_color_flags. */
			if ( bpp != 8 )
			  wdev->mapped_color_flags = 0;
			code = win_set_bits_per_pixel(wdev, bpp);
			if ( code < 0 )
			  ecode = code;
			else
			  break;
		  }
		goto bppe;
	default:
		ecode = code;
bppe:		param_signal_error(plist, "BitsPerPixel", ecode);
	case 1:
		break;
	}

	if ( ecode >= 0 )
	  {	/* Prevent gx_default_put_params from closing the device. */
		dev->is_open = false;
		ecode = gx_default_put_params(dev, plist);
		dev->is_open = is_open;
	  }
	if ( ecode < 0 )
	  {	/* If we allocated mapped_color_flags, release it. */
		if ( wdev->mapped_color_flags != 0 && old_flags == 0 )
		  gs_free(wdev->mapped_color_flags, 4096, 1,
			  "win_put_params");
		if ( bpp != old_bpp )
		  win_set_bits_per_pixel(wdev, old_bpp);
		return ecode;
	  }

	/* Hand off the change to the implementation. */
	if ( is_open && (bpp != old_bpp ||
			 dev->width != width || dev->height != height)
	   )
	{	int ccode;
		(*wdev->free_bitmap)(wdev);
		ccode = (*wdev->alloc_bitmap)(wdev, (gx_device *)wdev);
		if ( ccode < 0 )
		{	/* Bad news!  Some of the other device parameters */
			/* may have changed.  We don't handle this. */
			/* This is ****** WRONG ******. */
			dev->width = width;
			dev->height = height;
			win_set_bits_per_pixel(wdev, old_bpp);
			(*wdev->alloc_bitmap)(wdev, dev);
			return ccode;
		}
	}
	wdev->UpdateInterval = uii;
	if ( gsvs.data != 0 )
	  {	memcpy(wdev->GSVIEW_STR, gsvs.data, gsvs.size);
		wdev->GSVIEW_STR[gsvs.size] = 0;
	  }
	if (IsWindow(wdev->hwndimg) && IsWindow(wdev->hwndimgchild)) {
	    RECT rect;
	    /* erase bitmap - before window gets redrawn */
	    (*dev_proc(dev, fill_rectangle))(dev, 0, 0,
		dev->width, dev->height,
		win_map_rgb_color(dev, gx_max_color_value, 
		    gx_max_color_value, gx_max_color_value));
	    /* cause scroll bars to be redrawn */
	    GetClientRect(wdev->hwndimgchild,&rect);
	    SendMessage(wdev->hwndimgchild, WM_SIZE, SIZE_RESTORED, 
		MAKELPARAM(rect.right-rect.left, rect.bottom-rect.top));
	}
#ifdef __DLL__
	if (wdev->dll)
	    (*pgsdll_callback)(GSDLL_SIZE, (unsigned char *)dev,
		    (dev->width & 0xffff) + ( (dev->height & 0xffff)<<16) );
#endif

	return 0;
}


/* ------ Internal routines ------ */

#undef wdev

/* make image window */
private void near 
win_makeimg(gx_device_win *wdev)
{	HMENU sysmenu;
	RECT rect;
	/* parent window */
	if (gsview) {
	    wdev->hwndimg = gsview_hwnd;
	    if (IsIconic(wdev->hwndimg))
		ShowWindow(wdev->hwndimg, SW_SHOWNORMAL);
	}
	else {
	    wdev->hwndimg = CreateWindow(szImgName, (LPSTR)szImgName,
		  WS_OVERLAPPEDWINDOW,
		  CW_USEDEFAULT, CW_USEDEFAULT, 
		  CW_USEDEFAULT, CW_USEDEFAULT, 
		  NULL, NULL, phInstance, (void FAR *)wdev);
	    ShowWindow(wdev->hwndimg, SW_SHOWMINNOACTIVE);
	}
	/* child for graphics */
	GetClientRect(wdev->hwndimg, &rect);
	CreateWindow(szAppName, (LPSTR)szImgName,
		  WS_CHILD | WS_VSCROLL | WS_HSCROLL,
		  rect.left, rect.top,
		  rect.right-rect.left, rect.bottom-rect.top, 
		  wdev->hwndimg, NULL, phInstance, (void FAR *)wdev);
	ShowWindow(wdev->hwndimgchild, SW_SHOWNORMAL);

	if (!gsview) {
	    /* modify the menu to have the new items we want */
	    sysmenu = GetSystemMenu(wdev->hwndimg,0);	/* get the sysmenu */
	    AppendMenu(sysmenu, MF_SEPARATOR, 0, NULL);
	    AppendMenu(sysmenu, MF_STRING, M_COPY_CLIP, "Copy to Clip&board");
	}
}


/* out of memory error message box */
int
win_nomemory(void)
{
       	MessageBox(hwndtext,(LPSTR)"Not enough memory",(LPSTR) szAppName, MB_ICONSTOP);
	return gs_error_limitcheck;
}


LPLOGPALETTE
win_makepalette(gx_device_win *wdev)
{	int i, val;
	LPLOGPALETTE logpalette;
	logpalette = (LPLOGPALETTE)gs_malloc(1, sizeof(LOGPALETTE) + 
		(1<<(wdev->color_info.depth)) * sizeof(PALETTEENTRY),
		"win_makepalette");
	if (logpalette == (LPLOGPALETTE)NULL)
		return(0);
	logpalette->palVersion = 0x300;
	logpalette->palNumEntries = wdev->nColors;
	for (i=0; i<wdev->nColors; i++) {
	  logpalette->palPalEntry[i].peFlags = 0;
	  switch (wdev->nColors) {
		case 64:
		  /* colors are rrggbb */
		  logpalette->palPalEntry[i].peRed   = ((i & 0x30)>>4)*85;
		  logpalette->palPalEntry[i].peGreen = ((i & 0xC)>>2)*85;
		  logpalette->palPalEntry[i].peBlue  = (i & 3)*85;
		  break;
		case 16:
		  /* colors are irgb */
		  val = (i & 8 ? 255 : 128);
		  logpalette->palPalEntry[i].peRed   = i & 4 ? val : 0;
		  logpalette->palPalEntry[i].peGreen = i & 2 ? val : 0;
		  logpalette->palPalEntry[i].peBlue  = i & 1 ? val : 0;
		  if (i == 8) {	/* light gray */
		      logpalette->palPalEntry[i].peRed   = 
		      logpalette->palPalEntry[i].peGreen = 
		      logpalette->palPalEntry[i].peBlue  = 192;
		  }
		  break;
		case 2:
		  logpalette->palPalEntry[i].peRed =
		    logpalette->palPalEntry[i].peGreen =
		    logpalette->palPalEntry[i].peBlue = (i ? 255 : 0);
		  break;
	  }
	}
	return(logpalette);
}


void
win_update(gx_device_win *wdev)
{
#ifdef __DLL__
	if (wdev->dll)
	    return;	/* can't use timer since don't have window */
#endif
	if (!wdev->UpdateInterval)
		return;
	wdev->update = TRUE;
	if (!wdev->timer) {
		SetTimer(wdev->hwndimgchild, TIMER_ID, wdev->UpdateInterval, NULL);
		wdev->timer = TRUE;
  	}
}
  
private int
win_set_bits_per_pixel(gx_device_win *wdev, int bpp)
{
static const gx_device_color_info win_24bit_color = dci_color(24,255,255);
static const gx_device_color_info win_16bit_color = dci_color(24,31,31);
static const gx_device_color_info win_8bit_color = dci_color(8,31,4);
static const gx_device_color_info win_ega_color = dci_pc_4bit;
static const gx_device_color_info win_vga_color = dci_pc_4bit;
static const gx_device_color_info win_mono_color = dci_black_and_white;
HDC hdc;
	switch(bpp) {
	    case 24:
		wdev->color_info = win_24bit_color;
		wdev->nColors = -1;
		break;
	    case 16:
	    case 15:
		/* bitmap is stored as 24bits/pixel */
		wdev->color_info = win_16bit_color;
		wdev->nColors = -1;
		break;
	    case 8:
		/* use 64 static colors and 166 dynamic colors from 8 planes */
		wdev->color_info = win_8bit_color;
		wdev->nColors = 64;
		break;
	    case 4:
		hdc = GetDC(NULL);
		if (GetDeviceCaps(hdc, VERTRES) <= 350)
		    wdev->color_info = win_ega_color;
		else
		    wdev->color_info = win_vga_color;
		ReleaseDC(NULL,hdc);
		wdev->nColors = 16;
		break;
	    case 1:
		wdev->color_info = win_mono_color;
		wdev->nColors = 2;
		break;
	    default:
		return (gs_error_rangecheck);
	}
	wdev->BitsPerPixel = bpp;

	/* If necessary, allocate and clear the mapped color flags. */
	if ( bpp == 8 )
	{	if ( wdev->mapped_color_flags == 0 )
		{	wdev->mapped_color_flags = gs_malloc(4096, 1, "win_set_bits_per_pixel");
			if ( wdev->mapped_color_flags == 0 )
				return_error(gs_error_VMerror);
		}
		memset(wdev->mapped_color_flags, 0, 4096);
	}
	else
	{	gs_free(wdev->mapped_color_flags, 4096, 1, "win_set_bits_per_pixel");
		wdev->mapped_color_flags = 0;
	}
	return 0;
}


/* child graphics window */
LRESULT CALLBACK _export
WndImgChildProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{	gx_device_win *wdev;
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	HPALETTE oldpalette;
	int nVscrollInc, nHscrollInc;

	wdev = (gx_device_win *)GetWindowLong(hwnd, 0);

	switch(message) {
		case WM_CREATE:
			wdev =  (gx_device_win *)(((CREATESTRUCT *)lParam)->lpCreateParams);
			SetWindowLong(hwnd, 0, (LONG)wdev);
			wdev->hwndimgchild = hwnd;
			if (gsview)
			    SendMessage(gsview_hwnd, WM_GSVIEW, HWND_IMGCHILD, (LPARAM)wdev->hwndimgchild);
#if WINVER >= 0x030a
			/* Enable Drag Drop */
			if (is_win31)
				DragAcceptFiles(hwnd, TRUE);
#endif
			break;
		case WM_GSVIEW:
			switch(wParam) {
				case NEXT_PAGE:
					gsview_next = TRUE;
					return 0;
				case COPY_CLIPBOARD:
					(*wdev->copy_to_clipboard)(wdev);
					return 0;
			}
			break;
		case WM_TIMER:
			if ((wParam == TIMER_ID) && wdev->update) {
				InvalidateRect(wdev->hwndimgchild, (LPRECT)NULL, FALSE);
				UpdateWindow(wdev->hwndimgchild);
				wdev->update = FALSE;
			}
			break;
		case WM_SIZE:
			if (wParam == SIZE_MINIMIZED)
				return(0);
			wdev->cyClient = HIWORD(lParam);
			wdev->cxClient = LOWORD(lParam);

			wdev->cyAdjust = min(wdev->height, wdev->cyClient) - wdev->cyClient;
			wdev->cyClient += wdev->cyAdjust;

			wdev->nVscrollMax = max(0, wdev->height - wdev->cyClient);
			wdev->nVscrollPos = min(wdev->nVscrollPos, wdev->nVscrollMax);

			SetScrollRange(hwnd, SB_VERT, 0, wdev->nVscrollMax, FALSE);
			SetScrollPos(hwnd, SB_VERT, wdev->nVscrollPos, TRUE);

			wdev->cxAdjust = min(wdev->width,  wdev->cxClient) - wdev->cxClient;
			wdev->cxClient += wdev->cxAdjust;

			wdev->nHscrollMax = max(0, wdev->width - wdev->cxClient);
			wdev->nHscrollPos = min(wdev->nHscrollPos, wdev->nHscrollMax);

			SetScrollRange(hwnd, SB_HORZ, 0, wdev->nHscrollMax, FALSE);
			SetScrollPos(hwnd, SB_HORZ, wdev->nHscrollPos, TRUE);

			if (gsview && wdev->cxAdjust > 0)	/* don't make gsview window grow */
				wdev->cxAdjust = 0;
			if (gsview && wdev->cyAdjust > 0)
				wdev->cyAdjust = 0;

			if ((wParam==SIZENORMAL) && (wdev->cxAdjust!=0 || wdev->cyAdjust!=0)) {
			    GetWindowRect(GetParent(hwnd),&rect);
			    MoveWindow(GetParent(hwnd),rect.left,rect.top,
				rect.right-rect.left+wdev->cxAdjust,
				rect.bottom-rect.top+wdev->cyAdjust, TRUE);
			    wdev->cxAdjust = wdev->cyAdjust = 0;
			}
			if (gsview)
				SendMessage(gsview_hwnd, WM_GSVIEW, SCROLL_POSITION, 
					MAKELPARAM(wdev->nHscrollPos, wdev->nVscrollPos));
			return(0);
		case WM_VSCROLL:
			switch(LOWORD(wParam)) {
				case SB_TOP:
					nVscrollInc = -wdev->nVscrollPos;
					break;
				case SB_BOTTOM:
					nVscrollInc = wdev->nVscrollMax - wdev->nVscrollPos;
					break;
				case SB_LINEUP:
					nVscrollInc = -wdev->cyClient/16;
					break;
				case SB_LINEDOWN:
					nVscrollInc = wdev->cyClient/16;
					break;
				case SB_PAGEUP:
					nVscrollInc = min(-1,-wdev->cyClient);
					break;
				case SB_PAGEDOWN:
					nVscrollInc = max(1,wdev->cyClient);
					break;
				case SB_THUMBPOSITION:
#ifdef __WIN32__
					nVscrollInc = HIWORD(wParam) - wdev->nVscrollPos;
#else
					nVscrollInc = LOWORD(lParam) - wdev->nVscrollPos;
#endif
					break;
				default:
					nVscrollInc = 0;
				}
			if ((nVscrollInc = max(-wdev->nVscrollPos, 
				min(nVscrollInc, wdev->nVscrollMax - wdev->nVscrollPos)))!=0) {
				wdev->nVscrollPos += nVscrollInc;
				ScrollWindow(hwnd,0,-nVscrollInc,NULL,NULL);
				SetScrollPos(hwnd,SB_VERT,wdev->nVscrollPos,TRUE);
				UpdateWindow(hwnd);
			}
			if (gsview)
				SendMessage(gsview_hwnd, WM_GSVIEW, SCROLL_POSITION, 
					MAKELPARAM(wdev->nHscrollPos, wdev->nVscrollPos));
			return(0);
		case WM_HSCROLL:
			switch(LOWORD(wParam)) {
				case SB_LINEUP:
					nHscrollInc = -wdev->cxClient/16;
					break;
				case SB_LINEDOWN:
					nHscrollInc = wdev->cyClient/16;
					break;
				case SB_PAGEUP:
					nHscrollInc = min(-1,-wdev->cxClient);
					break;
				case SB_PAGEDOWN:
					nHscrollInc = max(1,wdev->cxClient);
					break;
				case SB_THUMBPOSITION:
#ifdef __WIN32__
					nHscrollInc = HIWORD(wParam) - wdev->nHscrollPos;
#else
					nHscrollInc = LOWORD(lParam) - wdev->nHscrollPos;
#endif
					break;
				default:
					nHscrollInc = 0;
				}
			if ((nHscrollInc = max(-wdev->nHscrollPos, 
				min(nHscrollInc, wdev->nHscrollMax - wdev->nHscrollPos)))!=0) {
				wdev->nHscrollPos += nHscrollInc;
				ScrollWindow(hwnd,-nHscrollInc,0,NULL,NULL);
				SetScrollPos(hwnd,SB_HORZ,wdev->nHscrollPos,TRUE);
				UpdateWindow(hwnd);
			}
			if (gsview)
				SendMessage(gsview_hwnd, WM_GSVIEW, SCROLL_POSITION, 
					MAKELPARAM(wdev->nHscrollPos, wdev->nVscrollPos));
			return(0);
		case WM_KEYDOWN:
			switch(LOWORD(wParam)) {
				case VK_HOME:
					SendMessage(hwnd,WM_VSCROLL,SB_TOP,0L);
					break;
				case VK_END:
					SendMessage(hwnd,WM_VSCROLL,SB_BOTTOM,0L);
					break;
				case VK_PRIOR:
					SendMessage(hwnd,WM_VSCROLL,SB_PAGEUP,0L);
					break;
				case VK_NEXT:
					SendMessage(hwnd,WM_VSCROLL,SB_PAGEDOWN,0L);
					break;
				case VK_UP:
					SendMessage(hwnd,WM_VSCROLL,SB_LINEUP,0L);
					break;
				case VK_DOWN:
					SendMessage(hwnd,WM_VSCROLL,SB_LINEDOWN,0L);
					break;
				case VK_LEFT:
					SendMessage(hwnd,WM_HSCROLL,SB_PAGEUP,0L);
					break;
				case VK_RIGHT:
					SendMessage(hwnd,WM_HSCROLL,SB_PAGEDOWN,0L);
					break;
				case VK_RETURN:
					BringWindowToTop(hwndtext);
					break;
			}
			return(0);
		case WM_PAINT:
			{
			int sx,sy,wx,wy,dx,dy;
			hdc = BeginPaint(hwnd, &ps);
			if (wdev->nColors > 0) {
			    oldpalette = SelectPalette(hdc,wdev->himgpalette,NULL);
			    RealizePalette(hdc);
			}
			SetMapMode(hdc, MM_TEXT);
			SetBkMode(hdc,OPAQUE);
			GetClientRect(hwnd, &rect);
#ifdef __WIN32__
			SetViewportExtEx(hdc, rect.right, rect.bottom, NULL);
#else
			SetViewportExt(hdc, rect.right, rect.bottom);
#endif
			dx = rect.left;	/* destination */
			dy = rect.top;
			wx = rect.right-rect.left; /* width */
			wy = rect.bottom-rect.top;
			sx = rect.left;	/* source */
			sy = rect.top;
			sx += wdev->nHscrollPos; /* scrollbars */
			sy += wdev->nVscrollPos;	
			if (sx+wx > wdev->width)
				wx = wdev->width - sx;
			if (sy+wy > wdev->height)
				wy = wdev->height - sy;
			(*wdev->repaint)(wdev,hdc,dx,dy,wx,wy,sx,sy);
			if (wdev->nColors > 0) {
			    SelectPalette(hdc,oldpalette,NULL);
			}
			EndPaint(hwnd, &ps);
			return 0;
			}
#if WINVER >= 0x030a
		case WM_DROPFILES:
			if (is_win31)
				SendMessage(hwndtext, message, wParam, lParam);
		case WM_DESTROY:
			/* disable Drag Drop */
			if (is_win31)
				DragAcceptFiles(hwnd, FALSE);
			break;
#endif
	}

not_ours:
	return DefWindowProc((HWND)hwnd, (WORD)message, (WORD)wParam, (DWORD)lParam);
}

/* parent overlapped window */
LRESULT CALLBACK _export
WndImgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	gx_device_win *wdev;

	wdev = (gx_device_win *)GetWindowLong(hwnd, 0);

	switch(message) {
		case WM_CREATE:
			wdev =  (gx_device_win *)(((CREATESTRUCT *)lParam)->lpCreateParams);
			wdev->hwndimg = hwnd;
			SetWindowLong(hwnd, 0, (LONG)wdev);
#if WINVER >= 0x030a
			/* Enable Drag Drop */
			if (is_win31)
				DragAcceptFiles(hwnd, TRUE);
#endif
			break;
		case WM_SYSCOMMAND:
			switch(LOWORD(wParam)) {
				case M_COPY_CLIP:
					(*wdev->copy_to_clipboard)(wdev);
					return 0;
			}
			break;
		case WM_KEYDOWN:
		case WM_KEYUP:
			if (wdev->hwndimgchild) {
				SendMessage(wdev->hwndimgchild, message, wParam, lParam);
				return 0;
			}
			break;
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED  && wdev->hwndimgchild!=(HWND)NULL)
			    SetWindowPos(wdev->hwndimgchild, (HWND)NULL, 0, 0,
				LOWORD(lParam), HIWORD(lParam), 
				SWP_NOZORDER | SWP_NOACTIVATE);
			return(0);
#if WINVER >= 0x030a
		case WM_DROPFILES:
			if (is_win31)
				SendMessage(hwndtext, message, wParam, lParam);
			break;
		case WM_DESTROY:
			if (is_win31)
				DragAcceptFiles(hwnd, FALSE);
#endif
			break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
