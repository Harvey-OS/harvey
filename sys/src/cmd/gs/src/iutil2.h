/* Copyright (C) 1993, 1994, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: iutil2.h,v 1.6 2002/06/16 04:47:10 lpd Exp $ */
/* Interface to procedures in iutil2.c */

#ifndef iutil2_INCLUDED
#  define iutil2_INCLUDED

/* ------ Password utilities ------ */

/* Define the password structure. */
/* NOTE: MAX_PASSWORD must match the initial password lengths in gs_lev2.ps. */
#define MAX_PASSWORD 64		/* must be at least 11 */
typedef struct password_s {
    uint size;
    byte data[MAX_PASSWORD];
} password;

# define NULL_PASSWORD {0, {0}}

/* Transmit a password to or from a parameter list. */
int param_read_password(gs_param_list *, const char *, password *);
int param_write_password(gs_param_list *, const char *, const password *);

/* Check a password from a parameter list. */
/* Return 0 if OK, 1 if not OK, or an error code. */
int param_check_password(gs_param_list *, const password *);

/* Read a password from, or write a password into, a dictionary */
/* (presumably systemdict). */
int dict_read_password(password *, const ref *, const char *);
int dict_write_password(const password *, ref *, const char *, bool);

#endif /* iutil2_INCLUDED */
