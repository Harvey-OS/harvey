/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "os.h"
#include <libsec.h>

/*
 *  these routines use the 64bit format for
 *  DES keys.
 */

void
setupDESstate(DESstate *s, uchar key[8], uchar *ivec)
{
	memset(s, 0, sizeof(*s));
	memmove(s->key, key, sizeof(s->key));
	des_key_setup(key, s->expanded);
	if(ivec)
		memmove(s->ivec, ivec, 8);
	s->setup = 0xdeadbeef;
}

void
setupDES3state(DES3state *s, uchar key[3][8], uchar *ivec)
{
	memset(s, 0, sizeof(*s));
	memmove(s->key, key, sizeof(s->key));
	des_key_setup(key[0], s->expanded[0]);
	des_key_setup(key[1], s->expanded[1]);
	des_key_setup(key[2], s->expanded[2]);
	if(ivec)
		memmove(s->ivec, ivec, 8);
	s->setup = 0xdeadbeef;
}
