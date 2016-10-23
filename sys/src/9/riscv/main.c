/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "init.h"
#include "io.h"

void testPrint(uint8_t c);

void fuck(char *s)
{
	while (*s)
		testPrint(*s++);
}

static int x = 0x123456;

void
main(uint32_t mbmagic, uint32_t mbaddress)
{

	testPrint('0');
	if (x != 0x123456)
		fuck("Data is not set up correctly\n");
	//memset(edata, 0, end - edata);
	fuck("got somewhere");
	while (1);

}
