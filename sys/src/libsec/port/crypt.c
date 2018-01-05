/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *	Data Encryption Standard
 *	D.P.Mitchell  83/06/08.
 *
 *	block_cipher(key, block, decrypting)
 *
 *	these routines use the non-standard 7 byte format
 *	for DES keys.
 */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <mp.h>
#include <libsec.h>

/*
 * destructively encrypt the buffer, which
 * must be at least 8 characters long.
 */
int
encrypt(void *key, void *vbuf, int n)
{
	uint32_t ekey[32];
	uint8_t *buf;
	int i, r;

	if(n < 8)
		return 0;
	key_setup(key, ekey);
	buf = vbuf;
	n--;
	r = n % 7;
	n /= 7;
	for(i = 0; i < n; i++){
		block_cipher(ekey, buf, 0);
		buf += 7;
	}
	if(r)
		block_cipher(ekey, buf - 7 + r, 0);
	return 1;
}

/*
 * destructively decrypt the buffer, which
 * must be at least 8 characters long.
 */
int
decrypt(void *key, void *vbuf, int n)
{
	uint32_t ekey[128];
	uint8_t *buf;
	int i, r;

	if(n < 8)
		return 0;
	key_setup(key, ekey);
	buf = vbuf;
	n--;
	r = n % 7;
	n /= 7;
	buf += n * 7;
	if(r)
		block_cipher(ekey, buf - 7 + r, 1);
	for(i = 0; i < n; i++){
		buf -= 7;
		block_cipher(ekey, buf, 1);
	}
	return 1;
}
