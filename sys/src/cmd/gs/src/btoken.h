/* Copyright (C) 1990, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: btoken.h,v 1.1 2000/03/09 08:40:40 lpd Exp $ */
/* Definitions for Level 2 binary tokens */

#ifndef btoken_INCLUDED
#  define btoken_INCLUDED

/* Define accessors for pointers to the system and user name tables. */
extern ref binary_token_names;	/* array of size 2 */

#define system_names_p (binary_token_names.value.refs)
#define user_names_p (binary_token_names.value.refs + 1)

/* Convert an object to its representation in a binary object sequence. */
int encode_binary_token(P5(i_ctx_t *i_ctx_p, const ref *obj, long *ref_offset,
			   long *char_offset, byte *str));

/* Define the current binary object format for operators. */
/* This is a ref so that it can be managed properly by save/restore. */
#define ref_binary_object_format_container i_ctx_p
#define ref_binary_object_format\
  (ref_binary_object_format_container->binary_object_format)

#endif /* btoken_INCLUDED */
