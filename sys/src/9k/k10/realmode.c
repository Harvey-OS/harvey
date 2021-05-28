#include "u.h"
#include "tos.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

#define LORMBUF (0x9000)

static long
rmemrw(int isr, void *a, long n, vlong off)
{
	if(off < 0 || n < 0)
		error("bad offset/count");
	if(isr){
		if(off >= MiB)
			return 0;
		if(off+n >= MiB)
			n = MiB - off;
		memmove(a, KADDR((uintptr)off), n);
	}else{
		/* realmode buf page ok, allow vga framebuf's access */
		if(off >= MiB || off+n > MiB ||
		    (off < LORMBUF || off+n > LORMBUF+4*KiB) &&
		    (off < 0xA0000 || off+n > 0xB0000+0x10000))
			error("bad offset/count in write");
		memmove(KADDR((uintptr)off), a, n);
	}
	return n;
}

static long
rmemread(Chan*, void *a, long n, vlong off)
{
	return rmemrw(1, a, n, off);
}

static long
rmemwrite(Chan*, void *a, long n, vlong off)
{
	return rmemrw(0, a, n, off);
}

void
realmodelink(void)
{
	addarchfile("realmodemem", 0660, rmemread, rmemwrite);
}
