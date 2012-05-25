#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

enum {
	Ax,
	Bx,
	Cx,
	Dx,

	/*
	 * common to intel & amd
	 */
	Extfunc	= 0x80000000,
	Procsig,

	/* Procsig bits */
	Dxlongmode	= 1<<29,
};

typedef unsigned long long u64intptr;

static int
havelongmode(void)
{
	ulong regs[4];

	memset(regs, 0, sizeof regs);
	cpuid(Extfunc, regs);
	if(regs[Ax] < Extfunc)
		return 0;

	memset(regs, 0, sizeof regs);
	cpuid(Procsig, regs);
	return (regs[Dx] & Dxlongmode) != 0;
}

void
warp64(uvlong entry)
{
	u64intptr kzero64 = 0xfffffffff0000000ull;
	extern void _warp64(ulong);

	print("warp64(%#llux) %#llux %d\n", entry, entry & ~kzero64, nmmap);
	if(!havelongmode()) {
		print("can't run 64-bit kernel on 32-bit cpu\n");
		delay(5000);
		exit(0);
	}
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
