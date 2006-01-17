/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gscrypt1.c,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* Adobe Type 1 encryption/decryption. */
#include "stdpre.h"
#include "gstypes.h"
#include "gscrypt1.h"

/* Encrypt a string. */
int
gs_type1_encrypt(byte * dest, const byte * src, uint len, crypt_state * pstate)
{
    crypt_state state = *pstate;
    const byte *from = src;
    byte *to = dest;
    uint count = len;

    while (count) {
	encrypt_next(*from, state, *to);
	from++, to++, count--;
    }
    *pstate = state;
    return 0;
}
/* Decrypt a string. */
int
gs_type1_decrypt(byte * dest, const byte * src, uint len, crypt_state * pstate)
{
    crypt_state state = *pstate;
    const byte *from = src;
    byte *to = dest;
    uint count = len;

    while (count) {
	/* If from == to, we can't use the obvious */
	/*      decrypt_next(*from, state, *to);        */
	byte ch = *from++;

	decrypt_next(ch, state, *to);
	to++, count--;
    }
    *pstate = state;
    return 0;
}
