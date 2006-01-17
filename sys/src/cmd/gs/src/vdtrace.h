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

/* $Id: vdtrace.h,v 1.14 2004/09/20 09:59:29 igor Exp $ */
/* Visual tracer service interface */

#ifndef vdtrace_INCLUDED
#  define vdtrace_INCLUDED

/*  Painting contract :
    
    First use vd_get_dc.
    Then paint with vd_* functionns.
    When completed, use vd_release_dc.

    The following functions paint immediately, without vd_beg_path, vd_end_path, vd_fill, vd_stroke :
    vd_circle, vd_round, vd_bar, vd_bar_w, vd_curve, vd_curve_w

    The following functions require vd_fill or vd_stroke only if enclosed with vd_beg_path, vd_end_path :
    vd_moveto, vd_lineto, vd_curveto, vd_closepath.
    Otherwise they paint directly (this is useful for step-by-step execution).
*/

#if !defined(VD_TRACE)
#define VD_TRACE 1
#endif

typedef struct vd_trace_host_s vd_trace_host;
typedef struct vd_trace_interface_s vd_trace_interface;
struct gs_fixed_point_s;

struct  vd_trace_interface_s {
    vd_trace_host *host;
    double scale_x, scale_y;
    double orig_x, orig_y;
    double shift_x, shift_y;
    double (*get_size_x)(vd_trace_interface *I);
    double (*get_size_y)(vd_trace_interface *I);
    void (*get_dc)(vd_trace_interface *I, vd_trace_interface **I1);
    void (*release_dc)(vd_trace_interface *I, vd_trace_interface **I1);
    void (*erase)(vd_trace_interface *I, unsigned long rgbcolor);
    void (*beg_path)(vd_trace_interface *I);
    void (*end_path)(vd_trace_interface *I);
    void (*moveto)(vd_trace_interface *I, double x, double y);
    void (*lineto)(vd_trace_interface *I, double x, double y);
    void (*curveto)(vd_trace_interface *I, double x0, double y0, double x1, double y1, double x2, double y2);
    void (*closepath)(vd_trace_interface *I);
    void (*circle)(vd_trace_interface *I, double x, double y, int r); /* Radius doesn't scale. */
    void (*round)(vd_trace_interface *I, double x, double y, int r); /* Radius doesn't scale. */
    void (*fill)(vd_trace_interface *I);
    void (*stroke)(vd_trace_interface *I);
    void (*setcolor)(vd_trace_interface *I, unsigned long rgbcolor);
    void (*setlinewidth)(vd_trace_interface *I, unsigned int width); /* Width doesn't scale. */
    void (*text)(vd_trace_interface *I, double x, double y, char *ASCIIZ); /* Font doesn't scale. */
    void (*wait)(vd_trace_interface *I);
    void (*set_scale)(vd_trace_interface *I);
    void (*set_shift)(vd_trace_interface *I);
    void (*set_origin)(vd_trace_interface *I);
};

extern vd_trace_interface * vd_trace0; /* Pointer to trace interface. */
extern vd_trace_interface * vd_trace1; /* A copy of vd_trace0, or NULL if trace is disabled. */
extern char vd_flags[];

void vd_impl_moveto(double x, double y);
void vd_impl_lineto(double x, double y);
void vd_impl_lineto_multi(const struct gs_fixed_point_s *p, int n);
void vd_impl_curveto(double x0, double y0, double x1, double y1, double x2, double y2);
void vd_impl_bar(double x0, double y0, double x1, double y1, int w, unsigned long c); /* unscaled width */
void vd_impl_square(double x0, double y0, int w, unsigned int c); /* unscaled width */
void vd_impl_rect(double x0, double y0, double x1, double y1, int w, unsigned int c);  /* unscaled width */
void vd_impl_quad(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int w, unsigned int c);  /* unscaled width */
void vd_impl_curve(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int w, unsigned long c); /* unscaled width */
void vd_impl_circle(double x, double y, int r, unsigned long c); /* unscaled radius */
void vd_impl_round(double x, double y, int r, unsigned long c);  /* unscaled radius */
void vd_impl_text(double x, double y, char *s, unsigned long c); /* unscaled font */
void vd_setflag(char f, char v);

#ifndef RGB
#    define RGB(r,g,b) ((unsigned long)(r) * 65536L + (unsigned long)(g) * 256L + (unsigned long)(b))
#endif

#if VD_TRACE && defined(DEBUG)
#    define vd_allowed(f)       (vd_flags[(f) & 127]) 
#    define vd_get_dc(f)        while (vd_trace0 && vd_allowed(f)) { vd_trace0->get_dc(vd_trace0, &vd_trace1); break; }
#    define vd_release_dc       BEGIN if (vd_trace1) vd_trace1->release_dc(vd_trace1, &vd_trace1); END
#    define vd_enabled          (vd_trace1 != 0) 
#    define vd_get_size_unscaled_x      (vd_trace1 ? vd_trace1->get_size_x(vd_trace1) : 100)
#    define vd_get_size_unscaled_y      (vd_trace1 ? vd_trace1->get_size_y(vd_trace1) : 100)
#    define vd_get_size_scaled_x        (vd_trace1 ? vd_trace1->get_size_x(vd_trace1) / vd_trace1->scale_x : 100)
#    define vd_get_size_scaled_y        (vd_trace1 ? vd_trace1->get_size_y(vd_trace1) / vd_trace1->scale_y : 100)
#    define vd_get_scale_x              (vd_trace1 ? vd_trace1->scale_x : 100)
#    define vd_get_scale_y              (vd_trace1 ? vd_trace1->scale_y : 100)
#    define vd_get_origin_x             (vd_trace1 ? vd_trace1->orig_x : 0)
#    define vd_get_origin_y             (vd_trace1 ? vd_trace1->orig_y : 0)
#    define vd_set_scale(s)     BEGIN if (vd_trace1) { vd_trace1->scale_x = vd_trace1->scale_y = s; vd_trace1->set_scale(vd_trace1); } END
#    define vd_set_scaleXY(sx,sy)       BEGIN if (vd_trace1) { vd_trace1->scale_x = sx, vd_trace1->scale_y = sy; vd_trace1->set_scale(vd_trace1); } END
#    define vd_set_origin(x,y)  BEGIN if (vd_trace1) { vd_trace1->orig_x  = x, vd_trace1->orig_y  = y; vd_trace1->set_origin(vd_trace1); } END
#    define vd_set_shift(x,y)   BEGIN if (vd_trace1) { vd_trace1->shift_x = x, vd_trace1->shift_y = y; vd_trace1->set_shift(vd_trace1); } END
#    define vd_set_central_shift        BEGIN if (vd_trace1) vd_trace1->shift_x = vd_trace1->get_size_x(vd_trace1)/2, vd_trace1->shift_y = vd_trace1->get_size_y(vd_trace1)/2; END
#    define vd_erase(c)         BEGIN if (vd_trace1)  vd_trace1->erase(vd_trace1,c); END
#    define vd_beg_path         BEGIN if (vd_trace1)  vd_trace1->beg_path(vd_trace1); END
#    define vd_end_path         BEGIN if (vd_trace1)  vd_trace1->end_path(vd_trace1); END
#    define vd_moveto(x,y)      BEGIN if (vd_trace1)  vd_impl_moveto(x,y); END
#    define vd_lineto(x,y)      BEGIN if (vd_trace1)  vd_impl_lineto(x,y); END
#    define vd_lineto_multi(p,n)        BEGIN if (vd_trace1)  vd_impl_lineto_multi(p,n); END
#    define vd_curveto(x0,y0,x1,y1,x2,y2) BEGIN if (vd_trace1)  vd_impl_curveto(x0,y0,x1,y1,x2,y2); END
#    define vd_closepath        BEGIN if (vd_trace1) vd_trace1->closepath(vd_trace1); END
#    define vd_bar(x0,y0,x1,y1,w,c)   BEGIN if (vd_trace1) vd_impl_bar(x0,y0,x1,y1,w,c); END
#    define vd_square(x0,y0,w,c)      BEGIN if (vd_trace1) vd_impl_square(x0,y0,w,c); END
#    define vd_rect(x0,y0,x1,y1,w,c)  BEGIN if (vd_trace1) vd_impl_rect(x0,y0,x1,y1,w,c); END
#    define vd_quad(x0,y0,x1,y1,x2,y2,x3,y3,w,c)  BEGIN if (vd_trace1) vd_impl_quad(x0,y0,x1,y1,x2,y2,x3,y3,w,c); END
#    define vd_curve(x0,y0,x1,y1,x2,y2,x3,y3,w,c) BEGIN if (vd_trace1) vd_impl_curve(x0,y0,x1,y1,x2,y2,x3,y3,w,c); END
#    define vd_circle(x,y,r,c)  BEGIN if (vd_trace1) vd_impl_circle(x,y,r,c); END
#    define vd_round(x,y,r,c)   BEGIN if (vd_trace1) vd_impl_round(x,y,r,c); END
#    define vd_fill             BEGIN if (vd_trace1) vd_trace1->fill(vd_trace1); END
#    define vd_stroke           BEGIN if (vd_trace1) vd_trace1->stroke(vd_trace1); END
#    define vd_setcolor(c)      BEGIN if (vd_trace1) vd_trace1->setcolor(vd_trace1,c); END
#    define vd_setlinewidth(w)  BEGIN if (vd_trace1) vd_trace1->setlinewidth(vd_trace1,w); END
#    define vd_text(x,y,s,c)    BEGIN if (vd_trace1) vd_impl_text(x,y,s,c); END
#    define vd_wait             BEGIN if (vd_trace1) vd_trace1->wait(vd_trace1); END
#    define vd_save	vd_trace_interface * vd_trace_save; vd_trace_save = vd_trace1; /* NOTE :
	    Use vd_save at end of the definition group of a  block,
	    because in release build it compiles to the empty operator.
	    We intentionally defined an operator here for a compatible syntax check.  */
#    define vd_restore	vd_trace1 = vd_trace_save
#    define vd_disable	vd_trace1 = NULL  
#else
#    define vd_allowed(f)   false 
#    define vd_get_dc(f)    DO_NOTHING
#    define vd_release_dc   DO_NOTHING
#    define vd_enabled			0
#    define vd_get_size_unscaled_x      100
#    define vd_get_size_unscaled_y      100
#    define vd_get_size_scaled_x        100
#    define vd_get_size_scaled_y        100
#    define vd_get_scale_x              100
#    define vd_get_scale_y              100
#    define vd_get_origin_x             0
#    define vd_get_origin_y             0
#    define vd_set_scale(sx)	    DO_NOTHING
#    define vd_set_scaleXY(sx,sy)   DO_NOTHING
#    define vd_set_origin(x,y)	    DO_NOTHING
#    define vd_set_shift(x,y)	    DO_NOTHING
#    define vd_set_central_shift    DO_NOTHING
#    define vd_erase(c)		    DO_NOTHING
#    define vd_beg_path		    DO_NOTHING
#    define vd_end_path		    DO_NOTHING
#    define vd_moveto(x,y)	    DO_NOTHING
#    define vd_lineto(x,y)	    DO_NOTHING
#    define vd_lineto_multi(p,n)    DO_NOTHING
#    define vd_curveto(x0,y0,x1,y1,x2,y2) DO_NOTHING
#    define vd_closepath	    DO_NOTHING
#    define vd_bar(x0,y0,x1,y1,w,c) DO_NOTHING
#    define vd_square(x0,y0,w,c)    DO_NOTHING
#    define vd_rect(x0,y0,x1,y1,w,c)	DO_NOTHING
#    define vd_quad(x0,y0,x1,y1,x2,y2,x3,y3,w,c)  DO_NOTHING
#    define vd_curve(x0,y0,x1,y1,x2,y2,x3,y3,w,c) DO_NOTHING
#    define vd_circle(x,y,r,c)	    DO_NOTHING
#    define vd_round(x,y,r,c)	    DO_NOTHING
#    define vd_fill		    DO_NOTHING
#    define vd_stroke		    DO_NOTHING
#    define vd_setcolor(c)	    DO_NOTHING
#    define vd_setlinewidth(w)	    DO_NOTHING
#    define vd_text(x,y,s,c)	    DO_NOTHING
#    define vd_wait		    DO_NOTHING
#    define vd_save		    DO_NOTHING
#    define vd_restore
#    define vd_disable
#endif

#endif /* vdtrace_INCLUDED */
