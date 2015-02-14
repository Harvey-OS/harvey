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

void
main(int argc, int8_t *argv[])
{
	uint32_t num = 1;
	uint64_t size;
	uint8_t *c;

	if (argc > 1)
		num = strtoul(argv[1], 0, 0);
	size = num * 0x200000ULL;
	print("Try to malloc %ulld bytes\n", size);
	c = mallocz(size, 1);
	print("Did it\n");
	while(1);
}

/* 6c big.c; 6l -o big big.6 */
