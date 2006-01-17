/* Copyright (C) 1996-2004 Ghostgum Software Pty Ltd.  All rights reserved.
  
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

/* $Id: dwimg.c,v 1.17 2004/10/28 09:26:11 igor Exp $ */

/* display device image window for Windows */

/* This code supports both single threaded and multithreaded operation */
/* For multithread, access is shared as follows:
 * Each image has a Mutex img->hmutex, used for protected access to
 * the img->image and its dimensions.
 * Main thread can access
 *   image_find()
 *   image_new()
 *   image_delete()
 *   image_size()  
 * Main thread must acquire mutex on display_presize() and release
 * in display_size() after image_size() is called.
 * Main thread must acquire mutex on display_preclose().
 *
 * Second thread must not access image_find, image_new, image_delete
 * or image_size.  It must grab mutex before accessing img->image.
 */

#define STRICT
#include <windows.h>
#include "stdio_.h"
#include "iapi.h"

#include "dwmain.h"
#include "dwimg.h"
#include "dwreg.h"
#include "gdevdsp.h"


static const char szImgName2[] = "Ghostscript Image";
static const char szTrcName2[] = "Ghostscript Graphical Trace";

/* Forward references */
LRESULT CALLBACK WndImg2Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void register_class(void);
static void draw(IMAGE *img, HDC hdc, int dx, int dy, int wx, int wy, 
    int sx, int sy);
static HGLOBAL copy_dib(IMAGE *img);
static HPALETTE create_palette(IMAGE *img);
static void create_window(IMAGE *img);

#define M_COPY_CLIP 1
#define M_DEVICEN_GRAY 2	/* show single separation as gray */
#define M_SEPARATION 3 		/* 3 to 3+IMG_DEVICEN_MAX-1 */

#define DISPLAY_ERROR (-1)	/* return this to Ghostscript on error */

/* Define  min and max, but make sure to use the identical definition */
/* to the one that all the compilers seem to have.... */
#ifndef min
#  define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#  define max(a, b) (((a) > (b)) ? (a) : (b))
#endif


/* GUI thread only */
void image_color(unsigned int format, int index, 
    unsigned char *r, unsigned char *g, unsigned char *b);
void image_convert_line(IMAGE *img, unsigned char *dest, unsigned char *source);
void image_16BGR555_to_24BGR(int width, unsigned char *dest, 
    unsigned char *source);
void image_16BGR565_to_24BGR(int width, unsigned char *dest, 
    unsigned char *source);
void image_16RGB555_to_24BGR(int width, unsigned char *dest, 
    unsigned char *source);
void image_16RGB565_to_24BGR(int width, unsigned char *dest, 
    unsigned char *source);
void image_4CMYK_to_24BGR(int width, unsigned char *dest, 
    unsigned char *source, IMAGE_DEVICEN *devicen, int devicen_gray);
void image_32CMYK_to_24BGR(int width, unsigned char *dest, 
    unsigned char *source, IMAGE_DEVICEN *devicen, int devicen_gray);
void image_devicen_to_24BGR(int width, unsigned char *dest, 
    unsigned char *source, IMAGE_DEVICEN *devicen, int devicen_gray);


/****************************************************************/
/* These functions are only accessed by the main thread */

IMAGE *first_image = NULL;

/* image_find must only be accessed by main thread */
/* valid for main thread */
IMAGE *
image_find(void *handle, void *device)
{
    IMAGE *img;
    for (img = first_image; img!=0; img=img->next) {
	if ((img->handle == handle) && (img->device == device))
	    return img;
    }
    return NULL;
}

/* image_find must only be accessed by main thread */
/* valid for main thread */
IMAGE *
image_new(void *handle, void *device)
{
    IMAGE *img = (IMAGE *)malloc(sizeof(IMAGE));
    if (img) {
        memset(img, 0, sizeof(IMAGE));
	/* remember device and handle */
	img->handle = handle;
	img->device = device;

	img->update_tick = 100;		/* milliseconds */
	img->update_interval = 1;	/* 1 tick */
	img->update_count = 0;

        img->hmutex = INVALID_HANDLE_VALUE;

	/* add to list */
	img->next = first_image;
	first_image = img;
    }
    return img;
}

/* remove image from linked list */
/* valid for main thread */
void
image_delete(IMAGE *img)
{
    /* remove from list */
    if (img == first_image) {
	first_image = img->next;
    }
    else {
	IMAGE *tmp;
	for (tmp = first_image; tmp!=0; tmp=tmp->next) {
	    if (img == tmp->next)
		tmp->next = img->next;
	}
    }
    /* Note: img is freed by image_close, not image_delete */
}


/* resize image */
/* valid for main thread */
int
image_size(IMAGE *img, int new_width, int new_height, int new_raster, 
    unsigned int new_format, void *pimage)
{
    int i;
    img->raster = new_raster;
    img->format = new_format;
    img->image = (unsigned char *)pimage;

    /* create a BMP header for the bitmap */
    img->bmih.biSize = sizeof(BITMAPINFOHEADER);
    img->bmih.biWidth = new_width;
    img->bmih.biHeight = new_height;
    img->bmih.biPlanes = 1;

    /* Reset separations */
    for (i=0; i<IMAGE_DEVICEN_MAX; i++) {
	img->devicen[i].used = 0;
	img->devicen[i].visible = 1;
	memset(img->devicen[i].name, 0, sizeof(img->devicen[i].name));
	img->devicen[i].cyan = 0;
	img->devicen[i].magenta = 0;
	img->devicen[i].yellow = 0;
	img->devicen[i].black = 0;
    }

    switch (img->format & DISPLAY_COLORS_MASK) {
	case DISPLAY_COLORS_NATIVE:
	    switch (img->format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		    img->bmih.biBitCount = 1;
		    img->bmih.biClrUsed = 2;
		    img->bmih.biClrImportant = 2;
		    break;
		case DISPLAY_DEPTH_4:
		    /* Fixed color palette */
		    img->bmih.biBitCount = 4;
		    img->bmih.biClrUsed = 16;
		    img->bmih.biClrImportant = 16;
		    break;
		case DISPLAY_DEPTH_8:
		    /* Fixed color palette */
		    img->bmih.biBitCount = 8;
		    img->bmih.biClrUsed = 96;
		    img->bmih.biClrImportant = 96;
		    break;
		case DISPLAY_DEPTH_16:
		    /* RGB bitfields */
		    /* Bit fields */
		    if ((img->format & DISPLAY_ENDIAN_MASK)
			== DISPLAY_BIGENDIAN) {
			/* convert to 24BGR */
			img->bmih.biBitCount = 24;
			img->bmih.biClrUsed = 0;
			img->bmih.biClrImportant = 0;
		    }
		    else {
			img->bmih.biBitCount = 16;
			img->bmih.biClrUsed = 0;
			img->bmih.biClrImportant = 0;
		    }
		    break;
		default:
		    return DISPLAY_ERROR;
	    }
	    break;
	case DISPLAY_COLORS_GRAY:
	    switch (img->format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		    img->bmih.biBitCount = 1;
		    img->bmih.biClrUsed = 2;
		    img->bmih.biClrImportant = 2;
		    break;
		case DISPLAY_DEPTH_4:
		    /* Fixed gray palette */
		    img->bmih.biBitCount = 4;
		    img->bmih.biClrUsed = 16;
		    img->bmih.biClrImportant = 16;
		    break;
		case DISPLAY_DEPTH_8:
		    /* Fixed gray palette */
		    img->bmih.biBitCount = 8;
		    img->bmih.biClrUsed = 256;
		    img->bmih.biClrImportant = 256;
		    break;
		default:
		    return DISPLAY_ERROR;
	    }
	    break;
	case DISPLAY_COLORS_RGB:
	    if ((img->format & DISPLAY_DEPTH_MASK) != DISPLAY_DEPTH_8)
		return DISPLAY_ERROR;
	    if (((img->format & DISPLAY_ALPHA_MASK) == DISPLAY_UNUSED_LAST) &&
		((img->format & DISPLAY_ENDIAN_MASK) == DISPLAY_LITTLEENDIAN)) {
		/* use bitfields to display this */
		img->bmih.biBitCount = 32;
		img->bmih.biClrUsed = 0;
		img->bmih.biClrImportant = 0;
	    }
	    else {
		/* either native BGR, or we need to convert it */
		img->bmih.biBitCount = 24;
		img->bmih.biClrUsed = 0;
		img->bmih.biClrImportant = 0;
	    }
	    break;
	case DISPLAY_COLORS_CMYK:
	    switch (img->format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		case DISPLAY_DEPTH_8:
		    /* we can convert these formats */
		    break;
		default:
		    return DISPLAY_ERROR;
	    }
	    /* we can't display this natively */
	    /* we will convert it just before displaying */
	    img->bmih.biBitCount = 24;
	    img->bmih.biClrUsed = 0;
	    img->bmih.biClrImportant = 0;
	    img->devicen[0].used = 1;
	    img->devicen[0].cyan = 65535;
	    /* We already know about the CMYK components */
	    strncpy(img->devicen[0].name, "Cyan", 
		sizeof(img->devicen[0].name));
	    img->devicen[1].used = 1;
	    img->devicen[1].magenta = 65535;
	    strncpy(img->devicen[1].name, "Magenta", 
		sizeof(img->devicen[1].name));
	    img->devicen[2].used = 1;
	    img->devicen[2].yellow = 65535;
	    strncpy(img->devicen[2].name, "Yellow", 
		sizeof(img->devicen[2].name));
	    img->devicen[3].used = 1;
	    img->devicen[3].black = 65535;
	    strncpy(img->devicen[3].name, "Black", 
		sizeof(img->devicen[3].name));
	    break;
	case DISPLAY_COLORS_SEPARATION:
	    /* we can't display this natively */
	    /* we will convert it just before displaying */
	    img->bmih.biBitCount = 24;
	    img->bmih.biClrUsed = 0;
	    img->bmih.biClrImportant = 0;
	    break;
    }

    img->bmih.biCompression = 0;
    img->bmih.biSizeImage = 0;
    img->bmih.biXPelsPerMeter = 0;
    img->bmih.biYPelsPerMeter = 0;
    img->bytewidth = ((img->bmih.biWidth * img->bmih.biBitCount + 31 ) & ~31) >> 3;

    if (img->palette)
	DeleteObject(img->palette);
    img->palette = create_palette(img);

    return 0;
}

int 
image_separation(IMAGE *img,
    int comp_num, const char *name,
    unsigned short c, unsigned short m,
    unsigned short y, unsigned short k)
{
    if ((comp_num < 0) || (comp_num > IMAGE_DEVICEN_MAX))
	return DISPLAY_ERROR;
    img->devicen[comp_num].used = 1;
    strncpy(img->devicen[comp_num].name, name,
	sizeof(img->devicen[comp_num].name)-1);
    img->devicen[comp_num].cyan    = c;
    img->devicen[comp_num].magenta = m;
    img->devicen[comp_num].yellow  = y;
    img->devicen[comp_num].black   = k;
    return 0;
}


/****************************************************************/
/* These functions are only accessed by the GUI thread */

/* open window for device and add to list */
void 
image_open(IMAGE *img)
{
    /* register class */
    register_class();
   
    /* open window */
    create_window(img);
}

/* close window and remove from list */
void
image_close(IMAGE *img)
{
    DestroyWindow(img->hwnd);
    img->hwnd = NULL;

    if (img->palette)
	DeleteObject(img->palette);
    img->palette = NULL;

    if (img->hBrush)
	DeleteObject(img->hBrush);
    img->hBrush = NULL;

    free(img);
}


void
register_class(void)
{
    WNDCLASS wndclass;
    HINSTANCE hInstance = GetModuleHandle(NULL);

    /* register the window class for graphics */
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndImg2Proc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = sizeof(LONG);
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(hInstance,(LPSTR)MAKEINTRESOURCE(GSIMAGE_ICON));
    wndclass.hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
    wndclass.hbrBackground = NULL;	/* we will paint background */
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szImgName2;
    RegisterClass(&wndclass);
}

void image_separations(IMAGE *img)
{
    char buf[64];
    int i;
    int exist;
    int num_visible = 0;
    HMENU sysmenu = GetSystemMenu(img->hwnd, FALSE);
    if (((img->format & DISPLAY_COLORS_MASK) == DISPLAY_COLORS_CMYK) ||
        ((img->format & DISPLAY_COLORS_MASK) == DISPLAY_COLORS_SEPARATION)) {
	/* Add menus if needed */
	for (i=0; i<IMAGE_DEVICEN_MAX; i++) {
	    exist = 0;
	    if (img->devicen[i].menu)
		exist = GetMenuString(sysmenu, M_SEPARATION+i, 
			buf, sizeof(buf)-1, MF_BYCOMMAND) != 0;
	    if (exist && (strcmp(img->devicen[i].name, buf) != 0)) {
		/* remove it because name changed */
	       RemoveMenu(sysmenu, M_SEPARATION+i, MF_BYCOMMAND);
	       img->devicen[i].menu = 0;
	    }
	    if (img->devicen[i].name[0] && !img->devicen[i].menu) {
		AppendMenu(sysmenu, MF_STRING | MF_CHECKED, 
		    M_SEPARATION+i, img->devicen[i].name);
		img->devicen[i].menu = 1;
	    }
	    if (img->devicen[i].used && img->devicen[i].visible)
		num_visible++;
	}
	EnableMenuItem(sysmenu, M_DEVICEN_GRAY, 
	    MF_BYCOMMAND | ((num_visible <= 1) ? MF_ENABLED : MF_GRAYED));
    }
    else {
	for (i=0; i<IMAGE_DEVICEN_MAX; i++) {
	   if (img->devicen[i].menu) {
	       RemoveMenu(sysmenu, M_SEPARATION+i, MF_BYCOMMAND);
	       img->devicen[i].menu = 0;
	   }
	}
	EnableMenuItem(sysmenu, M_DEVICEN_GRAY, MF_BYCOMMAND | MF_GRAYED);
    }
}

void sep_menu(IMAGE *img, int component)
{
    int i;
    int num_visible = 0;
    img->devicen[component].visible = !img->devicen[component].visible;
    CheckMenuItem(GetSystemMenu(img->hwnd, FALSE), 
	M_SEPARATION+component, 
	(img->devicen[component].visible ? MF_CHECKED : MF_UNCHECKED));
    for (i=0; i<IMAGE_DEVICEN_MAX; i++)
        if (img->devicen[i].used && img->devicen[i].visible)
	    num_visible++;
    EnableMenuItem(GetSystemMenu(img->hwnd, FALSE), M_DEVICEN_GRAY, 
	MF_BYCOMMAND | ((num_visible <= 1) ? MF_ENABLED : MF_GRAYED));
    InvalidateRect(img->hwnd, NULL, 0);
    UpdateWindow(img->hwnd);
}


static void
create_window(IMAGE *img)
{
    HMENU sysmenu;
    LOGBRUSH lb;
    char winposbuf[256];
    char window_title[256];
    int len = sizeof(winposbuf);

    /* create background brush */
    lb.lbStyle = BS_SOLID;
    lb.lbHatch = 0;
    lb.lbColor = GetSysColor(COLOR_WINDOW);
    if (lb.lbColor = RGB(255,255,255)) /* Don't allow white */
	lb.lbColor = GetSysColor(COLOR_MENU);
    if (lb.lbColor = RGB(255,255,255)) /* Don't allow white */
	lb.lbColor = GetSysColor(COLOR_APPWORKSPACE);
    if (lb.lbColor = RGB(255,255,255)) /* Don't allow white */
	lb.lbColor = RGB(192,192,192);
    img->hBrush = CreateBrushIndirect(&lb);

    img->cxClient = img->cyClient = 0;
    img->nVscrollPos = img->nVscrollMax = 0;
    img->nHscrollPos = img->nHscrollMax = 0;
    img->x = img->y = img->cx = img->cy = CW_USEDEFAULT;
    if (win_get_reg_value((img->device != NULL ? "Image" : "Tracer"), winposbuf, &len) == 0) {
	int x, y, cx, cy;
	
	if (sscanf(winposbuf, "%d %d %d %d", &x, &y, &cx, &cy) == 4) {
	    img->x = x;
	    img->y = y;
	    img->cx = cx;
	    img->cy = cy;
	}
    }
    strcpy(window_title, (img->device != NULL ? (LPSTR)szImgName2 : (LPSTR)szTrcName2));
    {  /*
        *   This section is for debug purpose only.
	*   It allows to replace window title so that user can identify window
	*   when multiple instances of the application run in same time.
	*   Create gs\bin\gswin32.ini or gs\bin\gswin32c.ini and
	*   put an identifier to there like this :
	*
	*	[Window]
	*	Title=Current Revision
	*
	*   It is useful to compare images generated with different revisions.
	*/
        char ini_path[MAX_PATH];
	DWORD ini_path_length;

	ini_path_length = GetModuleFileName(NULL, ini_path, sizeof(ini_path));
	if (ini_path_length > 0) {
	    int i = ini_path_length - 1;
	    for (; i>=0; i--)
		if(ini_path[i] == '.')
		    break;
	    if (i < sizeof(ini_path) - 4) {
		strcpy(ini_path + i, ".ini");
		GetPrivateProfileString("Window", "Title", 
			(img->device != NULL ? (LPSTR)szImgName2 : (LPSTR)szTrcName2), 
 			window_title, sizeof(window_title), ini_path);
	    }
	}
    }
    /* create window */
    img->hwnd = CreateWindow(szImgName2, window_title,
	      WS_OVERLAPPEDWINDOW,
	      img->x, img->y, img->cx, img->cy, 
	      NULL, NULL, GetModuleHandle(NULL), (void *)img);
    if (img->device == NULL && img->x != CW_USEDEFAULT &&
			       img->y != CW_USEDEFAULT &&
			       img->cx != CW_USEDEFAULT &&
			       img->cy != CW_USEDEFAULT)
        MoveWindow(img->hwnd, img->x, img->y, img->cx, img->cy, FALSE);
    ShowWindow(img->hwnd, (img->device != NULL ? SW_SHOWMINNOACTIVE : SW_SHOW));

    /* modify the menu to have the new items we want */
    sysmenu = GetSystemMenu(img->hwnd, 0);	/* get the sysmenu */
    AppendMenu(sysmenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(sysmenu, MF_STRING, M_COPY_CLIP, "Copy to Clip&board");
    AppendMenu(sysmenu, MF_STRING, M_DEVICEN_GRAY, "Show as Gray");
    AppendMenu(sysmenu, MF_SEPARATOR, 0, NULL);

    image_separations(img);
}

	
void
image_poll(IMAGE *img)
{
    if ((img->bmih.biWidth == 0) || (img->bmih.biHeight == 0))
	return;
    img->pending_update = 1;
    if (img->update_timer == 0) {
	img->update_timer = 1;
	img->update_count = 0;
	SetTimer(img->hwnd, img->update_timer, img->update_tick, NULL);
    }
}

/* Redraw the window, making sure that periodic updates don't take too long. */
void
image_update_now(IMAGE *img)
{
    SYSTEMTIME t1;
    SYSTEMTIME t2;
    int delta;
    if ( !IsWindow(img->hwnd) )	/* some clod closed the window */
	create_window(img);

    if ( !IsIconic(img->hwnd) ) {  /* redraw window */
	GetSystemTime(&t1);
	InvalidateRect(img->hwnd, NULL, 1);
	UpdateWindow(img->hwnd);
	GetSystemTime(&t2);
	/* Make sure the update interval is at least 10 times
	 * what it takes to paint the window
	 */
	delta = (t2.wSecond - t1.wSecond)*1000 + 
		(t2.wMilliseconds - t1.wMilliseconds);
	if (delta < 0)
	    delta += 60000;
	delta = 10 * delta / img->update_tick + 1;
	if (delta > img->update_interval)
	    img->update_interval = delta;
	else if ((delta >= 2) &&
		 (delta < img->update_interval / 4))
	    img->update_interval = delta/2;
    }
    img->update_count = 0;
}


void
image_sync(IMAGE *img)
{
    if (img->update_timer) {
	/* stop timer when nothing is happening */
	KillTimer(img->hwnd, img->update_timer);
        img->update_timer = 0;
    }
    img->pending_sync = 0;
    image_update_now(img);
    image_separations(img);
    img->pending_update = 0;
}


void
image_page(IMAGE *img)
{
    if (IsIconic(img->hwnd))    /* useless as an Icon so fix it */
	ShowWindow(img->hwnd, SW_SHOWNORMAL);
    BringWindowToTop(img->hwnd);

    image_sync(img);
}


/* GUI thread */
void
image_updatesize(IMAGE *img)
{
    RECT rect;
    int nSizeType;
    image_separations(img);
    /* update scroll bars */
    if (!IsIconic(img->hwnd)) {
	if (IsZoomed(img->hwnd))
	    nSizeType = SIZE_MAXIMIZED;
	else
	    nSizeType = SIZE_RESTORED;
	GetClientRect(img->hwnd, &rect);
	SendMessage(img->hwnd, WM_SIZE, nSizeType, 
	    MAKELONG(rect.right, rect.bottom));
    }
}

void
image_color(unsigned int format, int index, 
    unsigned char *r, unsigned char *g, unsigned char *b)
{
    switch (format & DISPLAY_COLORS_MASK) {
	case DISPLAY_COLORS_NATIVE:
	    switch (format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		    *r = *g = *b = (index ? 0 : 255);
		    break;
		case DISPLAY_DEPTH_4:
		    if (index == 7)
			*r = *g = *b = 170;
		    else if (index == 8)
			*r = *g = *b = 85;
		    else {
			int one = index & 8 ? 255 : 128;
			*r = (index & 4 ? one : 0);
			*g = (index & 2 ? one : 0);
			*b = (index & 1 ? one : 0);
		    }
		    break;
		case DISPLAY_DEPTH_8:
		    /* palette of 96 colors */
		    /* 0->63 = 00RRGGBB, 64->95 = 010YYYYY */
		    if (index < 64) {
			int one = 255 / 3;
			*r = ((index & 0x30) >> 4) * one;
			*g = ((index & 0x0c) >> 2) * one;
			*b =  (index & 0x03) * one;
		    }
		    else {
			int val = index & 0x1f;
			*r = *g = *b = (val << 3) + (val >> 2);
		    }
		    break;
	    }
	    break;
	case DISPLAY_COLORS_GRAY:
	    switch (format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		    *r = *g = *b = (index ? 255 : 0);
		    break;
		case DISPLAY_DEPTH_4:
		    *r = *g = *b = (unsigned char)((index<<4) + index);
		    break;
		case DISPLAY_DEPTH_8:
		    *r = *g = *b = (unsigned char)index;
		    break;
	    }
	    break;
    }
}


/* convert one line of 16BGR555 to 24BGR */
/* byte0=GGGBBBBB byte1=0RRRRRGG */
void
image_16BGR555_to_24BGR(int width, unsigned char *dest, unsigned char *source)
{
    int i;
    WORD w;
    unsigned char value;
    for (i=0; i<width; i++) {
	w = source[0] + (source[1] << 8);
	value = w & 0x1f;		/* blue */
	*dest++ = (value << 3) + (value >> 2);
	value = (w >> 5) & 0x1f;	/* green */
	*dest++ = (value << 3) + (value >> 2);
	value = (w >> 10) & 0x1f;	/* red */
	*dest++ = (value << 3) + (value >> 2);
	source += 2;
    }
}

/* convert one line of 16BGR565 to 24BGR */
/* byte0=GGGBBBBB byte1=RRRRRGGG */
void
image_16BGR565_to_24BGR(int width, unsigned char *dest, unsigned char *source)
{
    int i;
    WORD w;
    unsigned char value;
    for (i=0; i<width; i++) {
	w = source[0] + (source[1] << 8);
	value = w & 0x1f;		/* blue */
	*dest++ = (value << 3) + (value >> 2);
	value = (w >> 5) & 0x3f;	/* green */
	*dest++ = (value << 2) + (value >> 4);
	value = (w >> 11) & 0x1f;	/* red */
	*dest++ = (value << 3) + (value >> 2);
	source += 2;
    }
}

/* convert one line of 16RGB555 to 24BGR */
/* byte0=0RRRRRGG byte1=GGGBBBBB */
void
image_16RGB555_to_24BGR(int width, unsigned char *dest, unsigned char *source)
{
    int i;
    WORD w;
    unsigned char value;
    for (i=0; i<width; i++) {
	w = (source[0] << 8) + source[1];
	value = w & 0x1f;		/* blue */
	*dest++ = (value << 3) + (value >> 2);
	value = (w >> 5) & 0x1f;	/* green */
	*dest++ = (value << 3) + (value >> 2);
	value = (w >> 10) & 0x1f;	/* red */
	*dest++ = (value << 3) + (value >> 2);
	source += 2;
    }
}

/* convert one line of 16RGB565 to 24BGR */
/* byte0=RRRRRGGG byte1=GGGBBBBB */
void
image_16RGB565_to_24BGR(int width, unsigned char *dest, unsigned char *source)
{
    int i;
    WORD w;
    unsigned char value;
    for (i=0; i<width; i++) {
	w = (source[0] << 8) + source[1];
	value = w & 0x1f;		/* blue */
	*dest++ = (value << 3) + (value >> 2);
	value = (w >> 5) & 0x3f;	/* green */
	*dest++ = (value << 2) + (value >> 4);
	value = (w >> 11) & 0x1f;	/* red */
	*dest++ = (value << 3) + (value >> 2);
	source += 2;
    }
}

void
image_4CMYK_to_24BGR(int width, unsigned char *dest, unsigned char *source,
    IMAGE_DEVICEN *devicen, int devicen_gray)
{
    int i;
    int cyan, magenta, yellow, black;
    int vc = devicen[0].visible;
    int vm = devicen[1].visible;
    int vy = devicen[2].visible;
    int vk = devicen[3].visible;
    int vall = vc && vm && vy && vk;
    int show_gray = (vc + vm + vy + vk == 1) && devicen_gray;
    int value;
    for (i=0; i<width; i++) {
        value = source[i/2];
	if (i & 0)
	    value >>= 4;
	cyan = ((value >> 3) & 1) * 255;
	magenta = ((value >> 2) & 1) * 255;
	yellow = ((value >> 1) & 1) * 255;
	black = (value & 1) * 255;
	if (!vall) {
	    if (!vc)
		cyan = 0;
	    if (!vm)
		magenta = 0;
	    if (!vy)
		yellow = 0;
	    if (!vk)
		black = 0;
	    if (show_gray) {
		black += cyan + magenta + yellow;
		cyan = magenta = yellow = 0;
	    }
	}
	*dest++ = (255 - yellow)  * (255 - black)/255; /* blue */
	*dest++ = (255 - magenta) * (255 - black)/255; /* green */
	*dest++ = (255 - cyan)    * (255 - black)/255; /* red */
    }
}

/* convert one line of 32CMYK to 24BGR */
void
image_32CMYK_to_24BGR(int width, unsigned char *dest, unsigned char *source,
    IMAGE_DEVICEN *devicen, int devicen_gray)
{
    int i;
    int cyan, magenta, yellow, black;
    int vc = devicen[0].visible;
    int vm = devicen[1].visible;
    int vy = devicen[2].visible;
    int vk = devicen[3].visible;
    int vall = vc && vm && vy && vk;
    int show_gray = (vc + vm + vy + vk == 1) && devicen_gray;
    for (i=0; i<width; i++) {
	cyan = source[0];
	magenta = source[1];
	yellow = source[2];
	black = source[3];
	if (!vall) {
	    if (!vc)
		cyan = 0;
	    if (!vm)
		magenta = 0;
	    if (!vy)
		yellow = 0;
	    if (!vk)
		black = 0;
	    if (show_gray) {
		black += cyan + magenta + yellow;
		cyan = magenta = yellow = 0;
	    }
	}
	*dest++ = (255 - yellow)  * (255 - black)/255; /* blue */
	*dest++ = (255 - magenta) * (255 - black)/255; /* green */
	*dest++ = (255 - cyan)    * (255 - black)/255; /* red */
	source += 4;
    }
}

void
image_devicen_to_24BGR(int width, unsigned char *dest, unsigned char *source, 
    IMAGE_DEVICEN *devicen, int devicen_gray)
{
    int i, j;
    int cyan, magenta, yellow, black;
    int num_comp = 0;
    int value;
    int num_visible = 0;
    int show_gray = 0;
    for (j=0; j<IMAGE_DEVICEN_MAX; j++) {
	if (devicen[j].used) {
	   num_comp = j+1;
	   if (devicen[j].visible)
		num_visible++;
	}
    }
    if ((num_visible == 1) && devicen_gray)
	show_gray = 1;

    for (i=0; i<width; i++) {
	cyan = magenta = yellow = black = 0;
	for (j=0; j<num_comp; j++) {
	    if (devicen[j].visible && devicen[j].used) {
		value = source[j];
		if (show_gray)
		    black += value;
		else {
		    cyan    += value * devicen[j].cyan    / 65535;
		    magenta += value * devicen[j].magenta / 65535;
		    yellow  += value * devicen[j].yellow  / 65535;
		    black   += value * devicen[j].black / 65535;
		}
	    }
	}
	if (cyan > 255)
	   cyan = 255;
	if (magenta > 255)
	   magenta = 255;
	if (yellow > 255)
	   yellow = 255;
	if (black > 255)
	   black = 255;
	*dest++ = (255 - yellow)  * (255 - black)/255; /* blue */
	*dest++ = (255 - magenta) * (255 - black)/255; /* green */
	*dest++ = (255 - cyan)    * (255 - black)/255; /* red */
	source += 8;
    }
}

void
image_convert_line(IMAGE *img, unsigned char *dest, unsigned char *source)
{
    unsigned char *d = dest;
    unsigned char *s = source;
    int width = img->bmih.biWidth; 
    unsigned int alpha = img->format & DISPLAY_ALPHA_MASK;
    BOOL bigendian = (img->format & DISPLAY_ENDIAN_MASK) == DISPLAY_BIGENDIAN;
    int i;

    switch (img->format & DISPLAY_COLORS_MASK) {
	case DISPLAY_COLORS_NATIVE:
	    if ((img->format & DISPLAY_DEPTH_MASK) == DISPLAY_DEPTH_16) {
		if (bigendian) {
		    if ((img->format & DISPLAY_555_MASK)
			== DISPLAY_NATIVE_555)
			image_16RGB555_to_24BGR(img->bmih.biWidth, 
			    dest, source);
		    else
			image_16RGB565_to_24BGR(img->bmih.biWidth, 
			    dest, source);
		}
		else {
		    if ((img->format & DISPLAY_555_MASK)
			== DISPLAY_NATIVE_555) {
			image_16BGR555_to_24BGR(img->bmih.biWidth, 
			    dest, source);
		    }
		    else
			image_16BGR565_to_24BGR(img->bmih.biWidth, 
			    dest, source);
		}
	    }
	    break;
	case DISPLAY_COLORS_RGB:
	    if ((img->format & DISPLAY_DEPTH_MASK) != DISPLAY_DEPTH_8)
		return;
	    for (i=0; i<width; i++) {
		if ((alpha == DISPLAY_ALPHA_FIRST) || 
		    (alpha == DISPLAY_UNUSED_FIRST))
		    s++;
		if (bigendian) {
		    *d++ = s[2];
		    *d++ = s[1];
		    *d++ = s[0];
		    s+=3;
		}
		else {
		    *d++ = *s++;
		    *d++ = *s++;
		    *d++ = *s++;
		}
		if ((alpha == DISPLAY_ALPHA_LAST) || 
		    (alpha == DISPLAY_UNUSED_LAST))
		    s++;
	    }
/*
printf("rgb, width=%d alpha=%d d=0x%x s=0x%x\n", width, alpha, (int)d, (int)s);
printf("   d=0x%x s=0x%x\n", (int)d, (int)s);
*/
	    break;
	case DISPLAY_COLORS_CMYK:
	    if ((img->format & DISPLAY_DEPTH_MASK) == DISPLAY_DEPTH_8)
		image_32CMYK_to_24BGR(width, dest, source, 
		    img->devicen, img->devicen_gray);
	    else if ((img->format & DISPLAY_DEPTH_MASK) == DISPLAY_DEPTH_1) {
		image_4CMYK_to_24BGR(width, dest, source, 
		    img->devicen, img->devicen_gray);
	    }
	    else
		return;
	    break;
	case DISPLAY_COLORS_SEPARATION:
	    if ((img->format & DISPLAY_DEPTH_MASK) != DISPLAY_DEPTH_8)
		return;
	    image_devicen_to_24BGR(width, dest, source, 
		img->devicen, img->devicen_gray);
	    break;
    }
}

/* This makes a copy of the bitmap in global memory, suitable for clipboard */
/* Do not put 16 or 32-bit per pixels on the clipboard because */
/* ClipBook Viewer (NT4) can't display them */
static HGLOBAL 
copy_dib(IMAGE *img)
{
    int bitsperpixel;
    int bytewidth;
    int bitmapsize;
    int palcount;
    HGLOBAL hglobal;
    BYTE *pBits;
    BYTE *pLine;
    BYTE *pDIB;
    BITMAPINFOHEADER *pbmih;
    RGBQUAD *pColors;
    int i;
    BOOL directcopy = FALSE;

    /* Allocates memory for the clipboard bitmap */
    if (img->bmih.biBitCount <= 1)
	bitsperpixel = 1;
    else if (img->bmih.biBitCount <= 4)
	bitsperpixel = 4;
    else if (img->bmih.biBitCount <= 8)
	bitsperpixel = 8;
    else
	bitsperpixel = 24;
    bytewidth = ((img->bmih.biWidth * bitsperpixel + 31 ) & ~31) >> 3;
    bitmapsize = bytewidth * img->bmih.biHeight;
    if (bitsperpixel > 8)
	palcount = 0;	/* 24-bit BGR */
    else
	palcount = img->bmih.biClrUsed;

    hglobal = GlobalAlloc(GHND | GMEM_SHARE, sizeof(BITMAPINFOHEADER)
			  + sizeof(RGBQUAD) * palcount + bitmapsize);
    if (hglobal == (HGLOBAL) NULL)
	return (HGLOBAL) NULL;
    pDIB = (BYTE *) GlobalLock(hglobal);
    if (pDIB == (BYTE *) NULL)
	return (HGLOBAL) NULL;


    /* initialize the clipboard bitmap */
    pbmih = (BITMAPINFOHEADER *) (pDIB);
    pColors = (RGBQUAD *) (pDIB + sizeof(BITMAPINFOHEADER));
    pBits = (BYTE *) (pDIB + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * palcount);
    pbmih->biSize = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth = img->bmih.biWidth;
    pbmih->biHeight = img->bmih.biHeight;
    pbmih->biPlanes = 1;
    pbmih->biBitCount = bitsperpixel;
    pbmih->biCompression = 0;
    pbmih->biSizeImage = 0;	/* default */
    pbmih->biXPelsPerMeter = 0;
    pbmih->biYPelsPerMeter = 0;
    pbmih->biClrUsed = palcount;
    pbmih->biClrImportant = palcount;

    for (i = 0; i < palcount; i++) {
	image_color(img->format, i, &pColors[i].rgbRed, 
	    &pColors[i].rgbGreen, &pColors[i].rgbBlue);
	pColors[i].rgbReserved = 0;
    }

    /* find out if the format needs to be converted */
    switch (img->format & DISPLAY_COLORS_MASK) {
	case DISPLAY_COLORS_NATIVE:
	    switch (img->format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		case DISPLAY_DEPTH_4:
		case DISPLAY_DEPTH_8:
		    directcopy = TRUE;
	    }
	    break;
	case DISPLAY_COLORS_GRAY:
	    switch (img->format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		case DISPLAY_DEPTH_4:
		case DISPLAY_DEPTH_8:
		    directcopy = TRUE;
	    }
	    break;
	case DISPLAY_COLORS_RGB:
	    if (((img->format & DISPLAY_DEPTH_MASK) == DISPLAY_DEPTH_8) &&
		((img->format & DISPLAY_ALPHA_MASK) == DISPLAY_ALPHA_NONE) &&
		((img->format & DISPLAY_ENDIAN_MASK) == DISPLAY_LITTLEENDIAN))
		directcopy = TRUE;
    }

    pLine = pBits;
    if (directcopy) {
	for (i = 0; i < img->bmih.biHeight; i++) {
	    memcpy(pLine, img->image + i * img->raster, bytewidth);
	    pLine += bytewidth;
	}
    }
    else {
	/* we need to convert the format to 24BGR */
	for (i = 0; i < img->bmih.biHeight; i++) {
	    image_convert_line(img, pLine, img->image + i * img->raster);
	    pLine += bytewidth;
	}
    }

    GlobalUnlock(hglobal);

    return hglobal;
}

static HPALETTE 
create_palette(IMAGE *img)
{
    int i;
    int nColors;
    HPALETTE palette = NULL;

    nColors = img->bmih.biClrUsed;
    if (nColors) {
	LPLOGPALETTE logpalette;
	logpalette = (LPLOGPALETTE) malloc(sizeof(LOGPALETTE) + 
	    nColors * sizeof(PALETTEENTRY));
	if (logpalette == (LPLOGPALETTE) NULL)
	    return (HPALETTE)0;
	logpalette->palVersion = 0x300;
	logpalette->palNumEntries = img->bmih.biClrUsed;
	for (i = 0; i < nColors; i++) {
	    logpalette->palPalEntry[i].peFlags = 0;
	    image_color(img->format, i,
		&logpalette->palPalEntry[i].peRed,
		&logpalette->palPalEntry[i].peGreen,
		&logpalette->palPalEntry[i].peBlue);
	}
	palette = CreatePalette(logpalette);
	free(logpalette);
    }
    return palette;
}

/* image window */
/* All accesses to img->image or dimensions must be protected by mutex */
LRESULT CALLBACK
WndImg2Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    int nVscrollInc, nHscrollInc;
    IMAGE *img;

    if (message == WM_CREATE) {
	/* Object is stored in window extra data.
	 * Nothing must try to use the object before WM_CREATE
	 * initializes it here.
	 */
	img = (IMAGE *)(((CREATESTRUCT *)lParam)->lpCreateParams);
	SetWindowLong(hwnd, 0, (LONG)img);
    }
    img = (IMAGE *)GetWindowLong(hwnd, 0);


    switch(message) {
	case WM_SYSCOMMAND:
	    /* copy to clipboard */
	    if (LOWORD(wParam) == M_COPY_CLIP) {
		HGLOBAL hglobal;
		HPALETTE hpalette;
		if (img->hmutex != INVALID_HANDLE_VALUE)
		    WaitForSingleObject(img->hmutex, 120000);
		hglobal = copy_dib(img);
		if (hglobal == (HGLOBAL)NULL) {
		    if (img->hmutex != INVALID_HANDLE_VALUE)
			ReleaseMutex(img->hmutex);
		    MessageBox(hwnd, "Not enough memory to Copy to Clipboard", 
			szImgName2, MB_OK | MB_ICONEXCLAMATION);
		    return 0;
		}
		OpenClipboard(hwnd);
		EmptyClipboard();
		SetClipboardData(CF_DIB, hglobal);
		hpalette = create_palette(img);
		if (hpalette)
		    SetClipboardData(CF_PALETTE, hpalette);
		CloseClipboard();
		if (img->hmutex != INVALID_HANDLE_VALUE)
		    ReleaseMutex(img->hmutex);
		return 0;
	    }
	    else if ((LOWORD(wParam) >= M_SEPARATION) &&
	             (LOWORD(wParam) < M_SEPARATION+IMAGE_DEVICEN_MAX)) {
		sep_menu(img, LOWORD(wParam) - M_SEPARATION);
	    }
	    else if (LOWORD(wParam) == M_DEVICEN_GRAY) {
		img->devicen_gray = !img->devicen_gray;
		CheckMenuItem(GetSystemMenu(img->hwnd, FALSE), M_DEVICEN_GRAY, 
		    (img->devicen_gray ? MF_CHECKED : MF_UNCHECKED));
		InvalidateRect(img->hwnd, NULL, 0);
		UpdateWindow(img->hwnd);
	    }
	    break;
	case WM_CREATE:
	    /* enable drag-drop */
	    DragAcceptFiles(hwnd, TRUE);
	    break;
	case WM_MOVE:
	    if (!IsIconic(hwnd) && !IsZoomed(hwnd)) {
		GetWindowRect(hwnd, &rect);
		img->x = rect.left;
		img->y = rect.top;
	    }
	    break;
	case WM_SIZE:
	    if (wParam == SIZE_MINIMIZED)
		    return(0);

	    /* remember current window size */
	    if (wParam != SIZE_MAXIMIZED) {
		GetWindowRect(hwnd, &rect);
		img->cx = rect.right - rect.left;
		img->cy = rect.bottom - rect.top;
		img->x = rect.left;
		img->y = rect.top;
	    }

	    if (img->hmutex != INVALID_HANDLE_VALUE)
		WaitForSingleObject(img->hmutex, 120000);
	    img->cyClient = HIWORD(lParam);
	    img->cxClient = LOWORD(lParam);

	    img->cyAdjust = min(img->bmih.biHeight, img->cyClient) - img->cyClient;
	    img->cyClient += img->cyAdjust;

	    img->nVscrollMax = max(0, img->bmih.biHeight - img->cyClient);
	    img->nVscrollPos = min(img->nVscrollPos, img->nVscrollMax);

	    SetScrollRange(hwnd, SB_VERT, 0, img->nVscrollMax, FALSE);
	    SetScrollPos(hwnd, SB_VERT, img->nVscrollPos, TRUE);

	    img->cxAdjust = min(img->bmih.biWidth,  img->cxClient) - img->cxClient;
	    img->cxClient += img->cxAdjust;

	    img->nHscrollMax = max(0, img->bmih.biWidth - img->cxClient);
	    img->nHscrollPos = min(img->nHscrollPos, img->nHscrollMax);

	    SetScrollRange(hwnd, SB_HORZ, 0, img->nHscrollMax, FALSE);
	    SetScrollPos(hwnd, SB_HORZ, img->nHscrollPos, TRUE);

	    if ((wParam==SIZENORMAL) 
		&& (img->cxAdjust!=0 || img->cyAdjust!=0)) {
		GetWindowRect(GetParent(hwnd),&rect);
		MoveWindow(GetParent(hwnd),rect.left,rect.top,
		    rect.right-rect.left+img->cxAdjust,
		    rect.bottom-rect.top+img->cyAdjust, TRUE);
		img->cxAdjust = img->cyAdjust = 0;
	    }
	    if (img->hmutex != INVALID_HANDLE_VALUE)
		ReleaseMutex(img->hmutex);
	    return(0);
	case WM_VSCROLL:
	    switch(LOWORD(wParam)) {
		case SB_TOP:
		    nVscrollInc = -img->nVscrollPos;
		    break;
		case SB_BOTTOM:
		    nVscrollInc = img->nVscrollMax - img->nVscrollPos;
		    break;
		case SB_LINEUP:
		    nVscrollInc = -img->cyClient/16;
		    break;
		case SB_LINEDOWN:
		    nVscrollInc = img->cyClient/16;
		    break;
		case SB_PAGEUP:
		    nVscrollInc = min(-1,-img->cyClient);
		    break;
		case SB_PAGEDOWN:
		    nVscrollInc = max(1,img->cyClient);
		    break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
		    nVscrollInc = HIWORD(wParam) - img->nVscrollPos;
		    break;
		default:
		    nVscrollInc = 0;
	    }
	    if ((nVscrollInc = max(-img->nVscrollPos, 
		min(nVscrollInc, img->nVscrollMax - img->nVscrollPos)))!=0) {
		img->nVscrollPos += nVscrollInc;
		ScrollWindow(hwnd,0,-nVscrollInc,NULL,NULL);
		SetScrollPos(hwnd,SB_VERT,img->nVscrollPos,TRUE);
		UpdateWindow(hwnd);
	    }
	    return(0);
	case WM_HSCROLL:
	    switch(LOWORD(wParam)) {
		case SB_LINEUP:
		    nHscrollInc = -img->cxClient/16;
		    break;
		case SB_LINEDOWN:
		    nHscrollInc = img->cyClient/16;
		    break;
		case SB_PAGEUP:
		    nHscrollInc = min(-1,-img->cxClient);
		    break;
		case SB_PAGEDOWN:
		    nHscrollInc = max(1,img->cxClient);
		    break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
		    nHscrollInc = HIWORD(wParam) - img->nHscrollPos;
		    break;
		default:
		    nHscrollInc = 0;
	    }
	    if ((nHscrollInc = max(-img->nHscrollPos, 
		min(nHscrollInc, img->nHscrollMax - img->nHscrollPos)))!=0) {
		img->nHscrollPos += nHscrollInc;
		ScrollWindow(hwnd,-nHscrollInc,0,NULL,NULL);
		SetScrollPos(hwnd,SB_HORZ,img->nHscrollPos,TRUE);
		UpdateWindow(hwnd);
	    }
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
		    if (hwndtext)
			BringWindowToTop(hwndtext);
		    break;
	    }
	    return(0);
	case WM_CHAR:
	    /* send on all characters to text window */
	    if (hwndtext)
		SendMessage(hwndtext, message, wParam, lParam);
	    else {
		/* assume we have a console */
		INPUT_RECORD ir;
		HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
		DWORD dwWritten = 0;
		DWORD cks = 0;
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.bKeyDown = TRUE;
		ir.Event.KeyEvent.wRepeatCount = lParam & 0xffff;
		ir.Event.KeyEvent.wVirtualKeyCode = VkKeyScan((TCHAR)wParam) & 0xff;
		ir.Event.KeyEvent.wVirtualScanCode = 
		    (lParam >> 16) & 0xff;
		ir.Event.KeyEvent.uChar.AsciiChar = wParam;
		if (GetKeyState(VK_CAPITAL))
		   cks |= CAPSLOCK_ON;
		/* ENHANCED_KEY unimplemented */
		if (GetKeyState(VK_LMENU))
		   cks |= LEFT_ALT_PRESSED;
		if (GetKeyState(VK_LCONTROL))
		   cks |= LEFT_CTRL_PRESSED;
		if (GetKeyState(VK_NUMLOCK))
		   cks |= NUMLOCK_ON;
		if (GetKeyState(VK_RMENU))
		   cks |= RIGHT_ALT_PRESSED;
		if (GetKeyState(VK_RCONTROL))
		   cks |= RIGHT_CTRL_PRESSED;
		if (GetKeyState(VK_SCROLL))
		   cks |= SCROLLLOCK_ON;
		if (GetKeyState(VK_SHIFT))
		   cks |= SHIFT_PRESSED;
		ir.Event.KeyEvent.dwControlKeyState = cks;
		if (ir.Event.KeyEvent.uChar.AsciiChar == 3)
		    GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0L);
		else if (hStdin != INVALID_HANDLE_VALUE)
		    WriteConsoleInput(hStdin, &ir, 1, &dwWritten); 
	    }
	    return 0;
	case WM_TIMER:
	    img->update_count++;
	    if (img->update_count >= img->update_interval)
		image_update_now(img);
	    return 0;
	case WM_PAINT:
	    {
	    int sx,sy,wx,wy,dx,dy;
	    RECT fillrect;
	    hdc = BeginPaint(hwnd, &ps);
	    if (img->hmutex != INVALID_HANDLE_VALUE)
		WaitForSingleObject(img->hmutex, 120000);
	    SetMapMode(hdc, MM_TEXT);
	    SetBkMode(hdc,OPAQUE);
	    rect = ps.rcPaint;
	    dx = rect.left;	/* destination */
	    dy = rect.top;
	    wx = rect.right-rect.left; /* width */
	    wy = rect.bottom-rect.top;
	    sx = rect.left;	/* source */
	    sy = rect.top;
	    sx += img->nHscrollPos; /* scrollbars */
	    sy += img->nVscrollPos;	
	    if (sx+wx > img->bmih.biWidth)
		    wx = img->bmih.biWidth - sx;
	    if (sy+wy > img->bmih.biHeight)
		    wy = img->bmih.biHeight - sy;

	    draw(img, hdc, dx, dy, wx, wy, sx, sy);

	    /* fill areas around page */ 
	    if (rect.right > img->bmih.biWidth) {
		fillrect.top = rect.top;
		fillrect.left = img->bmih.biWidth;
		fillrect.bottom = rect.bottom;
		fillrect.right = rect.right;
		FillRect(hdc, &fillrect, img->hBrush);
	    }
	    if (rect.bottom > img->bmih.biHeight) {
		fillrect.top = img->bmih.biHeight;
		fillrect.left = rect.left;
		fillrect.bottom = rect.bottom;
		fillrect.right = rect.right;
		FillRect(hdc, &fillrect, img->hBrush);
	    }

	    if (img->hmutex != INVALID_HANDLE_VALUE)
		ReleaseMutex(img->hmutex);
	    EndPaint(hwnd, &ps);
	    return 0;
	    }
	case WM_DROPFILES:
	    if (hwndtext)
		SendMessage(hwndtext, message, wParam, lParam);
	    else {
		char szFile[256];
		int i, cFiles;
		const char *p;
		const char *szDragPre = "\r(";
		const char *szDragPost = ") run\r";
		HDROP hdrop = (HDROP)wParam;
		cFiles = DragQueryFile(hdrop, (UINT)(-1), (LPSTR)NULL, 0);
		for (i=0; i<cFiles; i++) {
		    DragQueryFile(hdrop, i, szFile, 80);
		    for (p=szDragPre; *p; p++)
			    SendMessage(hwnd,WM_CHAR,*p,1L);
		    for (p=szFile; *p; p++) {
			if (*p == '\\')
			    SendMessage(hwnd,WM_CHAR,'/',1L);
			else 
			    SendMessage(hwnd,WM_CHAR,*p,1L);
		    }
		    for (p=szDragPost; *p; p++)
			    SendMessage(hwnd,WM_CHAR,*p,1L);
		}
		DragFinish(hdrop);
	    }
	    break;
	case WM_DESTROY:
	    {   /* Save the text window size */
		char winposbuf[64];
		sprintf(winposbuf, "%d %d %d %d", img->x, img->y, 
		    img->cx, img->cy);
		win_set_reg_value((img->device != NULL ? "Image" : "Tracer"), winposbuf);
	    }
	    DragAcceptFiles(hwnd, FALSE);
	    break;

    }

	return DefWindowProc(hwnd, message, wParam, lParam);
}


/* Repaint a section of the window. */
static void
draw(IMAGE *img, HDC hdc, int dx, int dy, int wx, int wy,
		int sx, int sy)
{
    HPALETTE oldpalette;
    struct bmi_s {
	BITMAPINFOHEADER h;
	unsigned short pal_index[256];
    } bmi;
    int i;
    UINT which_colors;
    unsigned char *line = NULL;
    long ny;
    unsigned char *bits;
    BOOL directcopy = FALSE;

    if (img->device == NULL)
        return;

    memset(&bmi.h, 0, sizeof(bmi.h));
    
    bmi.h.biSize = sizeof(bmi.h);
    bmi.h.biWidth = img->bmih.biWidth;
    bmi.h.biHeight = wy;
    bmi.h.biPlanes = 1;
    bmi.h.biBitCount = img->bmih.biBitCount;
    bmi.h.biCompression = 0;
    bmi.h.biSizeImage = 0;	/* default */
    bmi.h.biXPelsPerMeter = 0;	/* default */
    bmi.h.biYPelsPerMeter = 0;	/* default */
    bmi.h.biClrUsed = img->bmih.biClrUsed;
    bmi.h.biClrImportant = img->bmih.biClrImportant;
    
    if (img->bmih.biClrUsed) {
	/* palette colors */
	for (i = 0; i < img->bmih.biClrUsed; i++)
	    bmi.pal_index[i] = i;
	which_colors = DIB_PAL_COLORS;
    } 
    else if (bmi.h.biBitCount == 16) {
	DWORD* bmi_colors = (DWORD*)(&bmi.pal_index[0]);
	bmi.h.biCompression = BI_BITFIELDS;
	which_colors = DIB_RGB_COLORS;
	if ((img->format & DISPLAY_555_MASK) == DISPLAY_NATIVE_555) {
	    /* 5-5-5 RGB mode */
	    bmi_colors[0] = 0x7c00;
	    bmi_colors[1] = 0x03e0;
	    bmi_colors[2] = 0x001f;
	}
	else {
	    /* 5-6-5 RGB mode */
	    bmi_colors[0] = 0xf800;
	    bmi_colors[1] = 0x07e0;
	    bmi_colors[2] = 0x001f;
	}
    }
    else if (bmi.h.biBitCount == 32) {
	unsigned int alpha = img->format & DISPLAY_ALPHA_MASK;
	DWORD* bmi_colors = (DWORD*)(&bmi.pal_index[0]);
	bmi.h.biCompression = BI_BITFIELDS;
	which_colors = DIB_RGB_COLORS;
	if ((img->format & DISPLAY_ENDIAN_MASK) == DISPLAY_BIGENDIAN) {
	    if ((alpha == DISPLAY_ALPHA_FIRST) || 
		(alpha == DISPLAY_UNUSED_FIRST)) {
		/* Mac mode */
		bmi_colors[0] = 0x0000ff00;
		bmi_colors[1] = 0x00ff0000;
		bmi_colors[2] = 0xff000000;
	    }
	    else {
		bmi_colors[0] = 0x000000ff;
		bmi_colors[1] = 0x0000ff00;
		bmi_colors[2] = 0x00ff0000;
	    }
	}
	else {
	    if ((alpha == DISPLAY_ALPHA_FIRST) || 
		(alpha == DISPLAY_UNUSED_FIRST)) {
		/* ignore alpha */
		bmi_colors[0] = 0xff000000;
		bmi_colors[1] = 0x00ff0000;
		bmi_colors[2] = 0x0000ff00;
	    }
	    else {
		/* Windows mode */
		/* ignore alpha */
		bmi_colors[0] = 0x00ff0000;
		bmi_colors[1] = 0x0000ff00;
		bmi_colors[2] = 0x000000ff;
	    }
	}
    } else {
	bmi.h.biClrUsed = 0;
	bmi.h.biClrImportant = 0;
	which_colors = DIB_RGB_COLORS;
    }

    if (img->raster <= 0)
	return;
    if (img->bytewidth <= 0)
	return;

    /* Determine if the format is native and we can do a direct copy */
    switch (img->format & DISPLAY_COLORS_MASK) {
	case DISPLAY_COLORS_NATIVE:
	    switch (img->format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		case DISPLAY_DEPTH_4:
		case DISPLAY_DEPTH_8:
		    directcopy = TRUE;
		    break;
		case DISPLAY_DEPTH_16:
		    if ((img->format & DISPLAY_ENDIAN_MASK)
			== DISPLAY_LITTLEENDIAN)
			directcopy = TRUE;
		    break;
	    }
	    break;
	case DISPLAY_COLORS_GRAY:
	    switch (img->format & DISPLAY_DEPTH_MASK) {
		case DISPLAY_DEPTH_1:
		case DISPLAY_DEPTH_4:
		case DISPLAY_DEPTH_8:
		    directcopy = TRUE;
	    }
	    break;
	case DISPLAY_COLORS_RGB:
	    if (((img->format & DISPLAY_DEPTH_MASK) == DISPLAY_DEPTH_8) &&
	        ((img->format & DISPLAY_ENDIAN_MASK) == DISPLAY_LITTLEENDIAN) &&
	        ((img->format & DISPLAY_ALPHA_MASK) == DISPLAY_ALPHA_NONE))
		directcopy = TRUE;	/* BGR24 */
	    if (((img->format & DISPLAY_DEPTH_MASK) == DISPLAY_DEPTH_8) &&
	        ((img->format & DISPLAY_ENDIAN_MASK) == DISPLAY_LITTLEENDIAN) &&
	        ((img->format & DISPLAY_ALPHA_MASK) == DISPLAY_UNUSED_LAST))
		directcopy = TRUE;	/* 32-bit */
	    break;
    }


    if (which_colors == DIB_PAL_COLORS) {
	oldpalette = SelectPalette(hdc, img->palette, FALSE);
	RealizePalette(hdc);
    }


    /*
     * Windows apparently limits the size of a single transfer
     * to 2 Mb, which can be exceeded on 24-bit displays.
     */
    ny = 2000000 / img->raster;

    if (img->raster != img->bytewidth)	/* not 32-bit architecture */
	ny = 1;

    /* If color format not native, convert it line by line */
    /* This is slow, but these formats aren't normally used */
    if (!directcopy) {
	ny = 1;
	line = (unsigned char *)malloc(img->bytewidth);
	if (line == NULL)
	    return;
    }

    for (; wy; dy += ny, wy -= ny, sy += ny) {
	ny = min(ny, wy);
	if (directcopy) {
	    bits = img->image + img->raster * (img->bmih.biHeight - (sy + ny));
	}
	else {
	    image_convert_line(img, line,
		img->image + img->raster * (img->bmih.biHeight - (sy + ny)));
	    bits = line;
	}
	SetDIBitsToDevice(hdc, dx, dy, wx, ny, sx, 0, 0, ny, bits,
		  (BITMAPINFO *) & bmi, which_colors);
    }
    
    if (which_colors == DIB_PAL_COLORS)
	SelectPalette(hdc, oldpalette, FALSE);

    if (line)
	free(line);
}

