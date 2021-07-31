/* Copyright (C) 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gpcheck.h */
/* Interrupt check interface */

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

#ifdef CHECK_INTERRUPTS
int gp_check_interrupts(P0());
#  define process_interrupts() discard(gp_check_interrupts())
#  define return_if_interrupt()\
	if ( gp_check_interrupts() )\
		return gs_error_interrupt
#  define return_check_interrupt(code)\
    return (code < 0 ? code :\
	    gp_check_interrupts() ? gs_error_interrupt :\
	    code)
#else
#  define gp_check_interrupts() 0
#  define process_interrupts() DO_NOTHING
#  define return_if_interrupt()	DO_NOTHING
#  define return_check_interrupt(code)\
    return (code)
#endif
