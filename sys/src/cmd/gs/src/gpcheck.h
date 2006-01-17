/* Copyright (C) 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gpcheck.h,v 1.9 2004/08/13 12:59:03 stefan Exp $ */
/* Interrupt check interface */

#ifndef gpcheck_INCLUDED
#  define gpcheck_INCLUDED

/*
 * On some platforms, the interpreter must check periodically for user-
 * initiated actions.  (Eventually, this may be extended to all platforms,
 * to handle multi-tasking through the 'context' facility.)  Routines that
 * run for a long time must periodically call gp_check_interrupts(), and
 * if it returns true, must clean up whatever they are doing and return an
 * e_interrupted (or gs_error_interrupted) exceptional condition.
 * The return_if_interrupt macro provides a convenient way to do this.
 *
 * On platforms that require an interrupt check, the makefile defines
 * a symbol CHECK_INTERRUPTS.  Currently this is only the Microsoft
 * Windows platform.
 */
int gs_return_check_interrupt(const gs_memory_t *mem, int code);

#ifdef CHECK_INTERRUPTS
int gp_check_interrupts(const gs_memory_t *mem);
#  define process_interrupts(mem) discard(gp_check_interrupts(mem))
#  define return_if_interrupt(mem)\
    { int icode_ = gp_check_interrupts(mem);	\
      if ( icode_ )\
	return gs_note_error((icode_ > 0 ? gs_error_interrupt : icode_));\
    }
#  define return_check_interrupt(mem, code)	\
    return gs_return_check_interrupt(mem, code)
#  define set_code_on_interrupt(mem, pcode)	\
    if (*(pcode) == 0)\
     *(pcode) = (gp_check_interrupts(mem) != 0) ? gs_error_interrupt : 0;
#else
#  define gp_check_interrupts(mem) 0
#  define process_interrupts(mem) DO_NOTHING
#  define return_if_interrupt(mem)	DO_NOTHING
#  define return_check_interrupt(mem, code)	\
    return (code)
#  define set_code_on_interrupt(mem, code) DO_NOTHING
#endif

#endif /* gpcheck_INCLUDED */
