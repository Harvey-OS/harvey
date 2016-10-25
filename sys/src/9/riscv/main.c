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

void msg(char *s)
{
	while (*s)
		testPrint(*s++);
}
void die(char *s)
{
	msg(s);
	while (1);
}

static int x = 0x123456;

/* mach struct for hart 0. */
/* in many plan 9 implementations this stuff is all reserved in early assembly.
 * we don't have to do that. */
static uint64_t m0stack[4096];
static Mach m0;

/* general purpose hart startup. We call this via startmach.
 * When we enter here, the machp() function is usable.
 */

void hart(void)
{
	//Mach *mach = machp();
	die("not yet");
}

void bsp(void)
{
	Mach *mach = machp();
	if (mach != &m0)
		die("MACH NOT MATCH");
	msg("memset mach\n");
	memset(mach, 0, sizeof(Mach));
	msg("done that\n");

	mach->self = (uintptr_t)mach;
	msg("SET SELF OK\n");
	mach->machno = 0;
	mach->online = 1;
	mach->NIX.nixtype = NIXTC;
	mach->stack = PTR2UINT(m0stack);
	*(uintptr_t*)mach->stack = STACKGUARD;
	mach->externup = nil;
	active.nonline = 1;
	active.exiting = 0;
	active.nbooting = 0;

	msg("call asminit\n");
	/* TODO: can the asm code be made portable? A lot of it looks like it can. */
	//asminit();
	die("Completed hart for bsp OK!\n");
}

void
main(uint32_t mbmagic, uint32_t mbaddress)
{

	testPrint('0');
	if (x != 0x123456)
		die("Data is not set up correctly\n");
	//memset(edata, 0, end - edata);
	msg("got somewhere");
	startmach(bsp, &m0);
}
