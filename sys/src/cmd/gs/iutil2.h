/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* iutil2.h */
/* Interface to procedures in iutil2.c */

/* ------ Password utilities ------ */

/* Define the password structure. */
#define max_password 64			/* must be at least 11 */
typedef struct password_s {
	uint size;
	byte data[max_password];
} password;
# define NULL_PASSWORD {0, {0}}

/* Define the system password(s). */
extern password SystemParamsPassword;

/* Transmit a password to or from a parameter list. */
int param_read_password(P3(gs_param_list *, const char _ds *, password *));
int param_write_password(P3(gs_param_list *, const char _ds *, const password *));

/* Check a password from a parameter list. */
/* Return 0 if OK, 1 if not OK, or an error code. */
int param_check_password(P2(gs_param_list *, const password *));
