/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>

/* from the kernel. Sorry. */
enum {
	Nipht = 521,				/* convenient prime */
};

u32 newiphash(u8 * sa, u16 sp, u8 * da, u16 dp)
{
	u32 kludge;
	kludge =
		((sa[IPaddrlen - 1] << 24) ^ (sp << 16) ^ (da[IPaddrlen - 1] << 8) ^
		 dp);
	return kludge % Nipht;
}

u32 oldiphash(u8 * sa, u16 sp, u8 * da, u16 dp)
{
	return ((sa[IPaddrlen - 1] << 24) ^ (sp << 16) ^ (da[IPaddrlen - 1] << 8) ^
			dp) % Nipht;
}

/* conventions.
 * informational messages go on fd 2.
 * PASS/FAIL go on fd 1 and there is only ever one of each.
 * The first four characters of passing tests are PASS
 * The first four characters of failing tests are FAIL
 * It is an error to print both PASS and FAIL
 * FAIL tests should exits() with a message
 * We may consider not printing PASS/FAIL and using exits instead.
 */
int main()
{
	static u8 sa[IPaddrlen] = { 0x80, };
	static u8 da[IPaddrlen];
	u16 sp = 4;
	u16 dp = 5;
	u32 ohash, nhash;
	sa[IPaddrlen - 1] = 0x80;
	ohash = oldiphash(sa, sp, da, dp);
	if (ohash > Nipht)
		fprint(2, "oldiphash returns bad value: 0x%lx, should be < 0x%lx\n",
			   ohash, Nipht);
	nhash = newiphash(sa, sp, da, dp);
	if (nhash > Nipht)
		fprint(2, "newiphash returns bad value: 0x%lx, should be < 0x%lx\n",
			   ohash, Nipht);
	fprint(2, "ohash is 0x%lx, nhash is 0x%lx\n", ohash, nhash);
	if (ohash == nhash) {
		exits("FAIL:iphash failed. ohash and nhash should differ\n");
	}
	/* Always print PASS at the end. */
	print("PASS\n");
	return 0;
}
