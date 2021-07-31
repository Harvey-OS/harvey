#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

typedef unsigned long long u64intptr;

void
warp64(uvlong entry)
{
	u64intptr kzero64 = 0xfffffffff0000000ull;
	extern void _warp64(ulong);

	print("warp64(%#llux) %#llux %d\n", entry, entry & ~kzero64, nmmap);
	if(v_flag)
		print("mkmultiboot\n");
	mkmultiboot();
	if(v_flag)
		print("impulse\n");
	/*
	 * No output after impulse().
	 */
	if(v_flag)
		print("_warp64\n");
	impulse();
	_warp64(entry & ~kzero64);
}
