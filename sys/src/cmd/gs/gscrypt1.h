/* Copyright (C) 1990, 1992 Aladdin Enterprises.  All rights reserved.
  
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

/* gscrypt1.h */
/* Interface to Adobe Type 1 encryption/decryption. */

/* Normal public interface */
typedef ushort crypt_state;
int gs_type1_encrypt(P4(byte *dest, const byte *src, uint len, crypt_state *pstate));
int gs_type1_decrypt(P4(byte *dest, const byte *src, uint len, crypt_state *pstate));

/* Define the encryption parameters and procedures */
#define crypt_c1 ((ushort)52845)
#define crypt_c2 ((ushort)22719)
#define encrypt_next(ch, state, chvar)\
  chvar = ((ch) ^ (state >> 8)),\
  state = (chvar + state) * crypt_c1 + crypt_c2
#define decrypt_this(ch, state)\
  ((ch) ^ (state >> 8))
#define decrypt_next(ch, state, chvar)\
  chvar = decrypt_this(ch, state),\
  decrypt_skip_next(ch, state)
#define decrypt_skip_next(ch, state)\
  state = ((ch) + state) * crypt_c1 + crypt_c2
