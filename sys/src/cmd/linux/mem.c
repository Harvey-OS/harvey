#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

/* keep in order, lowest base address first */
enum {
	SEGDATA,
	SEGPRIVATE,
	SEGSHARED,
	SEGSTACK,
	SEGMAX,
};

static char *segname[SEGMAX] = { "data", "private", "shared", "stack" };

uintptr
pagealign(uintptr addr)
{
	uintptr m;

	/* oh hell */
	m = (1<<21)-1;
	return (addr + m) & ~m;
}

struct linux_mmap_args {
	u64int addr;
	u64int len;
	u64int prot;
	u64int flags;
	u64int fd;
	u64int offset;
};

uintptr
linux_mmap(u64int addr, u64int len, u64int prot, u64int flags, 
	u64int fd, u64int pgoff)
{
	uintptr o, top;
	int e, n;
	uintptr base;

	trace("sys_mmap(%lux, %lux, %d, %d, %d, %lux)", addr, len, prot, flags, fd, pgoff);

	o = pgoff;

	if (fd > -1 && seek(fd, 0, o))
		return -ENOENT;

	/* not yet! */
	if (addr)
		return (uintptr)-EINVAL;

	if(pagealign(addr) != addr)
		return (uintptr)-EINVAL;

	n = pagealign(len);
	/* find the current break */
	base = (uintptr)segbrk(nil, nil);
	/* fucking stupid bug in segbrk */
	top = (uintptr)segbrk((void *)(base-8), (void *)(base + n));
	if (top == (uintptr)-1)
		return (uintptr)-ENOMEM;
	if (fd < 0)
		return base;

	trace("map %d [%lux-%lux] at [%lux-%lux]", fd, o, o + n, base, 
			base+n);

	addr = base;
	while(addr < top){
		n = pread(fd, (void*)addr, top - addr, o);
		if(n == 0)
			break;
		if(n < 0){
			//trace("read failed at offset %lux for address %lux failed: %r", o, addr);
			break;
		}
		addr += n;
		o += n;
	}

	return base;
}

uintptr linux_munmap(uintptr base, uintptr size)
{
	return 0;

}

