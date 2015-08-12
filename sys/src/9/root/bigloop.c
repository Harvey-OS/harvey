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
main(int argc, char *argv[])
{
	uint32_t num = 1;
	int i;

	if(argc > 1)
		num = strtoul(argv[1], 0, 0);
	print("Try to malloc %ulld bytes in %ld loops\n", num * 0x200000ULL, num);
	for(i = 0; i < num; i++)
		if(sbrk(0x200000) == nil) {
			print("%d sbrk failed\n", i);
			break;
		}
	print("Did it\n");
	while(1)
		;
}

/* 6c bigloop.c; 6l -o bigloop bigloop.6 */
