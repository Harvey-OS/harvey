/*
 * pool allocator regression test
 */
#include <u.h>
#include <libc.h>
#include <pool.h>

void
main(void)
{
	int i;
	void *allocs[4];

	mainmem->flags = POOL_LOGGING | POOL_DEBUGGING;
	malloc(8);		/* without this, memory gets corrupted */
	for (i = 0; i < nelem(allocs); i++) {
		print("malloc %d: ", i);
		werrstr("");
		allocs[i] = malloc(1500*1024*1024);
		if (allocs[i] == nil)
			sysfatal("malloc %d failed: %r", i);
		print("%#p...", allocs[i]);
	}
	for (i = 0; i < nelem(allocs); i++) {
		print("free %d...", i);
		free(allocs[i]);
	}
	print("\n");
	exits(0);
}
