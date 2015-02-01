/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "os.h"
#include <mp.h>
#include <libsec.h>

uchar key[] = "Jefe";
uchar data[] = "what do ya want for nothing?";

void
main(void)
{
	int i;
	uchar hash[MD5dlen];

	hmac_md5(data, strlen((char*)data), key, 4, hash, nil);
	for(i=0; i<MD5dlen; i++)
		print("%2.2x", hash[i]);
	print("\n");
	print("750c783e6ab0b503eaa86e310a5db738\n");
}
