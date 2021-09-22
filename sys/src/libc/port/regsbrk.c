/*
 * sbrk regression test
 */
#include <u.h>
#include <libc.h>

enum {
	Pagesize = 4096,
};

void
main(void)
{
	int i;
	char *start, *dbreak, *addr;
	void *allocs[4];
	volatile int junk;

	dbreak = start = sbrk(0);
	for (i = 0; i < nelem(allocs); i++) {
		dbreak += 1500*1024*1024;
		print("brk(%#p)...", dbreak);
		if (brk(dbreak) < 0)
			sysfatal("brk(%#p = %,llud) failed: %r", dbreak,
				(uvlong)dbreak);
		dbreak[-1] = '\0';
	}
	print("\n");

	/* touch each page */
	for (addr = start; addr + Pagesize < dbreak; addr += Pagesize)
		junk = *addr;
	print("touched %#p through %#p (%#,llud)\n",
		start, addr, (uintptr)addr);
	exits(0);
}
