/* Copyright (C) 2002 artofcode LLC. All rights reserved.
  
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

/* $Id: dwtrace.c,v 1.6 2004/02/14 08:44:30 igor Exp $ */
/* Graphical trace server for Windows */

/*  This module use Win32-specific API.
    For 16-bit Windows many of functions compile to stubs.
*/

/*  fixme : Restoring image on WM_PAINT is NOT implemented yet.
*/

#define STRICT
#include <windows.h>
#include <math.h>
#include "dwimg.h"

static COLORREF WindowsColor(unsigned long c)
{   /*  This body uses a Windows specific macro RGB, which is being
        redefined in GS include files. Please include them after
        this definition.
    */
    return RGB(c >> 16, (c >> 8) & 255, c & 255);
}

#include "gscdefs.h"
#include "stdpre.h"
#include "gsdll.h"
#include "vdtrace.h"
#include "dwtrace.h"

struct vd_trace_host_s {
    bool inited;
    IMAGE * tw;
    HDC hdc;
    int count_GetDC;
    int window_height;
    int line_width;
    COLORREF color;
    HPEN pen, pen0;
    HBRUSH brush, brush0;
    double bx, by;
};

static struct vd_trace_host_s host = {false, NULL, NULL};
vd_trace_interface visual_tracer = { &host, 1, 1, 0, 0, 0, 0 };
static const char *vdtrace_ini = "gs_vdtrace.ini";

private void get_window()
{   if (!host.inited) {
        host.tw = image_new(NULL, NULL);	/* create and add to list */
        if (host.tw) {
            image_open(host.tw);
            UpdateWindow(host.tw->hwnd);
        }
	host.hdc = NULL;
	host.count_GetDC = 0;
	host.window_height = 100;
	host.line_width = 1;
	host.color = 0;
	host.inited = true;
    }
}

private inline int ScaleX(struct vd_trace_host_s *h, double x)
{   return (int)(x + 0.5);
}

private inline int ScaleY(struct vd_trace_host_s *h, double y)
{   return h->window_height - (int)(y + 0.5);
}

#define SX(x) ScaleX(I->host,x)
#define SY(y) ScaleY(I->host,y)

private inline void delete_pen_brush(vd_trace_interface *I)
{   SelectObject(I->host->hdc, I->host->pen0);
    SelectObject(I->host->hdc, I->host->brush0);
    if(I->host->pen != NULL)
	DeleteObject(I->host->pen);
    I->host->pen = NULL;
    if(I->host->brush != NULL)
	DeleteObject(I->host->brush);
    I->host->brush = NULL;
}

private inline void new_pen_brush(vd_trace_interface *I)
{   delete_pen_brush(I);
    I->host->pen = CreatePen(PS_SOLID, I->host->line_width, WindowsColor(I->host->color));
    I->host->brush = CreateSolidBrush(WindowsColor(I->host->color));
    SelectObject(I->host->hdc, I->host->pen);
    SelectObject(I->host->hdc, I->host->brush);
}

private double dw_gt_get_size_x(vd_trace_interface *I)
{   RECT r;
    get_window(); 
    if (host.tw == NULL) 
        return(100);
    GetClientRect(I->host->tw->hwnd,&r);
    return r.right - r.left;
}

private double dw_gt_get_size_y(vd_trace_interface *I)
{   RECT r;
    get_window(); 
    if (host.tw == NULL) 
        return(100);
    GetClientRect(I->host->tw->hwnd,&r);
    return r.bottom - r.top;
}

private void dw_gt_get_dc(vd_trace_interface *I, vd_trace_interface **I1)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    if (I->host->hdc == NULL) {
	RECT r;
	I->host->hdc = GetDC(I->host->tw->hwnd);
	SetMapMode(I->host->hdc,MM_TEXT);
	GetClientRect(I->host->tw->hwnd, &r);
	I->host->window_height = r.bottom;
	SetBkMode(I->host->hdc,TRANSPARENT);
	I->host->pen0 = (HPEN)SelectObject(I->host->hdc, GetStockObject(BLACK_PEN));
	I->host->brush0 = (HBRUSH)SelectObject(I->host->hdc, GetStockObject(BLACK_BRUSH));
	I->host->color = 0;
	I->host->count_GetDC = 1;
        *I1 = I;
    } else
	++I->host->count_GetDC;
}

private void dw_gt_release_dc(vd_trace_interface *I, vd_trace_interface **I1)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    --I->host->count_GetDC;
    if(I->host->count_GetDC == 0) {
	ReleaseDC(I->host->tw->hwnd, I->host->hdc);
	I->host->hdc = NULL;
	delete_pen_brush(I);
        *I1 = NULL;
    } else if(I->host->count_GetDC < 0) {
        /* safety : */
	I->host->count_GetDC = 0;
        *I1 = NULL;
    }
}

private void dw_gt_erase(vd_trace_interface *I, unsigned long rgbcolor)
{   HWND hwnd;
    RECT r;
    HBRUSH hbr;
    get_window(); 
    if (host.tw == NULL) 
        return;
    hwnd = I->host->tw->hwnd;
    GetClientRect(hwnd, &r);
    hbr = CreateSolidBrush(rgbcolor);
    FillRect(I->host->hdc, &r, hbr);
    DeleteObject(hbr);
}

private void dw_gt_beg_path(vd_trace_interface *I)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    BeginPath(I->host->hdc);
}

private void dw_gt_end_path(vd_trace_interface *I)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    EndPath(I->host->hdc);
}

private void dw_gt_moveto(vd_trace_interface *I, double x, double y)
{   POINT p;
    get_window(); 
    if (host.tw == NULL) 
        return;
#ifdef __WIN32__
    MoveToEx(I->host->hdc, SX(x), SY(y), &p);
#else
    MoveTo(I->host->hdc, SX(x), SY(y));
#endif
    I->host->bx = x; I->host->by = y;
}

private void dw_gt_lineto(vd_trace_interface *I, double x, double y)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    LineTo(I->host->hdc, SX(x), SY(y));
}

private void dw_gt_curveto(vd_trace_interface *I, double x0, double y0, double x1, double y1, double x2, double y2)
{   POINT p[3];
    get_window(); 
    if (host.tw == NULL) 
        return;
    p[0].x = SX(x0), p[0].y = SY(y0);
    p[1].x = SX(x1), p[1].y = SY(y1);
    p[2].x = SX(x2), p[2].y = SY(y2);
    PolyBezierTo(I->host->hdc, p, 3);
}

private void dw_gt_closepath(vd_trace_interface *I)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    LineTo(I->host->hdc, SX(I->host->bx), SY(I->host->by));
    CloseFigure(I->host->hdc);
}

private void dw_gt_circle(vd_trace_interface *I, double x, double y, int r)
{   HBRUSH h;
    get_window(); 
    if (host.tw == NULL) 
        return;
    h = (HBRUSH)SelectObject(I->host->hdc, GetStockObject(NULL_BRUSH));
    Ellipse(I->host->hdc, SX(x)-r, SY(y)-r, SX(x)+r, SY(y)+r);
    SelectObject(I->host->hdc, h);
}

private void dw_gt_round(vd_trace_interface *I, double x, double y, int r)
{   HPEN h;
    get_window(); 
    if (host.tw == NULL) 
        return;
    h = (HPEN)SelectObject(I->host->hdc, GetStockObject(NULL_PEN));
    Ellipse(I->host->hdc, SX(x)-r, SY(y)-r, SX(x)+r, SY(y)+r);
    SelectObject(I->host->hdc, h);
}

private void dw_gt_fill(vd_trace_interface *I)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    FillPath(I->host->hdc);
}

private void dw_gt_stroke(vd_trace_interface *I)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    StrokePath(I->host->hdc);
}

private void dw_gt_setcolor(vd_trace_interface *I, unsigned long rgbcolor)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    if (I->host->color != rgbcolor) {
	I->host->color = rgbcolor;
	new_pen_brush(I);
	SetTextColor(I->host->hdc, rgbcolor);
    }
}

private void dw_gt_setlinewidth(vd_trace_interface *I, unsigned int width)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    if (I->host->line_width != width) {
	I->host->line_width = width;
	new_pen_brush(I);
    }
}

private void dw_gt_text(vd_trace_interface *I, double x, double y, char *ASCIIZ)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    TextOut(I->host->hdc, SX(x), SY(y), ASCIIZ, strlen(ASCIIZ));
}

private void dw_gt_wait(vd_trace_interface *I)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    /* fixme : not implemented yet. */
}

private void dw_gt_set_scale(vd_trace_interface *I)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    I->scale_x *= GetPrivateProfileInt("VDTRACE", "ScaleX", 1000, vdtrace_ini) / 1000.0;
    I->scale_y *= GetPrivateProfileInt("VDTRACE", "ScaleY", 1000, vdtrace_ini) / 1000.0;
}

private void dw_gt_set_shift(vd_trace_interface *I)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    I->shift_x += (int)GetPrivateProfileInt("VDTRACE", "ShiftX", 0, vdtrace_ini);
    I->shift_y += (int)GetPrivateProfileInt("VDTRACE", "ShiftY", 0, vdtrace_ini);
}

private void dw_gt_set_origin(vd_trace_interface *I)
{   get_window(); 
    if (host.tw == NULL) 
        return;
    I->orig_x += (int)GetPrivateProfileInt("VDTRACE", "OrigX", 0, vdtrace_ini);
    I->orig_y += (int)GetPrivateProfileInt("VDTRACE", "OrigY", 0, vdtrace_ini);
}

#ifdef __WIN32__
#    define SET_CALLBACK(I,a) I.a = dw_gt_##a
#else
#    define SET_CALLBACK(I,a) I.a = 0
#endif

void visual_tracer_init(void)
{   SET_CALLBACK(visual_tracer, get_dc);
    SET_CALLBACK(visual_tracer, release_dc);
    SET_CALLBACK(visual_tracer, erase);
    SET_CALLBACK(visual_tracer, get_size_x);
    SET_CALLBACK(visual_tracer, get_size_y);
    SET_CALLBACK(visual_tracer, erase);
    SET_CALLBACK(visual_tracer, beg_path);
    SET_CALLBACK(visual_tracer, end_path);
    SET_CALLBACK(visual_tracer, moveto);
    SET_CALLBACK(visual_tracer, lineto);
    SET_CALLBACK(visual_tracer, curveto); /* optional */
    SET_CALLBACK(visual_tracer, closepath);
    SET_CALLBACK(visual_tracer, circle);
    SET_CALLBACK(visual_tracer, round);
    SET_CALLBACK(visual_tracer, fill);
    SET_CALLBACK(visual_tracer, stroke);
    SET_CALLBACK(visual_tracer, setcolor);
    SET_CALLBACK(visual_tracer, setlinewidth);
    SET_CALLBACK(visual_tracer, text);
    SET_CALLBACK(visual_tracer, wait);
    SET_CALLBACK(visual_tracer, set_scale);
    SET_CALLBACK(visual_tracer, set_shift);
    SET_CALLBACK(visual_tracer, set_origin);
}
        
void visual_tracer_close(void)
{   if (host.tw != NULL) {
        image_delete(host.tw);
        image_close(host.tw);
    }
}





