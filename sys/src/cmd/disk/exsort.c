/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>

int	ulcmp(const void*, const void*);
void	swapem(u32*, i32);

enum
{
	Wormsize	= 157933,
};
int	wflag;

void
main(int argc, char *argv[])
{
	i32 i, l, x, lobits, hibits, tot;
	int f, j;
	char *file;
	u32 *b, a, lo, hi;

	ARGBEGIN {
	default:
		print("usage: disk/exsort [-w] [file]\n");
		exits("usage");
	case 'w':
		wflag++;
		break;
	} ARGEND;

	file = "/adm/cache";
	if(argc > 0)
		file = argv[0];

	if(wflag)
		f = open(file, ORDWR);
	else
		f = open(file, OREAD);
	if(f < 0) {
		print("cant open %s: %r\n", file);
		exits("open");
	}
	l = seek(f, 0, 2) / sizeof(i32);

	b = malloc(l*sizeof(i32));
	if(b == 0) {
		print("cant malloc %s: %r\n", file);
		exits("malloc");
	}
	seek(f, 0, 0);
	if(read(f, b, l*sizeof(i32)) != l*sizeof(i32)) {
		print("short read %s: %r\n", file);
		exits("read");
	}

	lobits = 0;
	hibits = 0;
	for(i=0; i<l; i++) {
		a = b[i];
		if(a & (1L<<7))
			lobits++;
		if(a & (1L<<31))
			hibits++;
	}

	print("lobits = %6ld\n", lobits);
	print("hibits = %6ld\n", hibits);

	if(hibits > lobits) {
		print("swapping\n");
		swapem(b, l);
	}

	qsort(b, l, sizeof(u32), ulcmp);

	tot = 0;
	for(j=0; j<100; j++) {
		lo = j*Wormsize;
		hi = lo + Wormsize;

		x = 0;
		for(i=0; i<l; i++) {
			a = b[i];
			if(a >= lo && a < hi)
				x++;
		}
		if(x) {
			print("disk %2d %6ld blocks\n", j, x);
			tot += x;
		}
	}
	print("total   %6ld blocks\n", tot);


	if(wflag) {
		if(hibits > lobits)
			swapem(b, l);
		seek(f, 0, 0);
		if(write(f, b, l*sizeof(i32)) != l*sizeof(i32)) {
			print("short write %s\n", file);
			exits("write");
		}
	}

	exits(0);
}

int
ulcmp(const void *va, const void *vb)
{
	const u32 *a = (const u32*)va;
	const u32 *b = (const u32*)vb;

	if(*a > *b)
		return 1;
	if(*a < *b)
		return -1;
	return 0;
}

void
swapem(u32 *b, i32 l)
{
	i32 i;
	u32 x, a;

	for(i=0; i<l; i++, b++) {
		a = *b;
		x = (((a>>0) & 0xff) << 24) |
			(((a>>8) & 0xff) << 16) |
			(((a>>16) & 0xff) << 8) |
			(((a>>24) & 0xff) << 0);
		*b = x;
	}
}
