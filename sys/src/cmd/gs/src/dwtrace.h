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

/* $Id: dwtrace.h,v 1.7 2004/09/15 19:41:01 ray Exp $ */
/* The interface of Graphical trace server for Windows */

#ifndef dwtrace_INCLUDED
#  define dwtrace_INCLUDED

extern struct vd_trace_interface_s visual_tracer;
void visual_tracer_init(void);
void visual_tracer_close(void);

#endif /* dwtrace_INCLUDED */
