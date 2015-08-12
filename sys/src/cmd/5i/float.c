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
#include <bio.h>
#include <mach.h>
#include "arm.h"

void unimp(ulong);
void Ifcmp(ulong);
void Ifdiv(ulong);
void Ifmul(ulong);
void Ifadd(ulong);
void Ifsub(ulong);
void Ifmov(ulong);
void Icvtd(ulong);
void Icvtw(ulong);
void Icvts(ulong);
void Ifabs(ulong);
void Ifneg(ulong);

Inst cop1[] = {{Ifadd, "add.f", Ifloat},
               {Ifsub, "sub.f", Ifloat},
               {Ifmul, "mul.f", Ifloat},
               {Ifdiv, "div.f", Ifloat},
               {
                unimp, "",
               },
               {Ifabs, "abs.f", Ifloat},
               {Ifmov, "mov.f", Ifloat},
               {Ifneg, "neg.f", Ifloat},
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {Icvts, "cvt.s", Ifloat},
               {Icvtd, "cvt.d", Ifloat},
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {Icvtw, "cvt.w", Ifloat},
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {
                unimp, "",
               },
               {Ifcmp, "c.f", Ifloat},
               {Ifcmp, "c.un", Ifloat},
               {Ifcmp, "c.eq", Ifloat},
               {Ifcmp, "c.ueq", Ifloat},
               {Ifcmp, "c.olt", Ifloat},
               {Ifcmp, "c.ult", Ifloat},
               {Ifcmp, "c.ole", Ifloat},
               {Ifcmp, "c.ule", Ifloat},
               {Ifcmp, "c,sf", Ifloat},
               {Ifcmp, "c.ngle", Ifloat},
               {Ifcmp, "c.seq", Ifloat},
               {Ifcmp, "c.ngl", Ifloat},
               {Ifcmp, "c.lt", Ifloat},
               {Ifcmp, "c.nge", Ifloat},
               {Ifcmp, "c.le", Ifloat},
               {Ifcmp, "c.ngt", Ifloat},
               {0}};

void
unimp(uint32_t inst)
{
	print("op %d\n", inst & 0x3f);
	Bprint(bioout, "Unimplemented floating point Trap IR %.8lux\n", inst);
	longjmp(errjmp, 0);
}

void
inval(uint32_t inst)
{
	Bprint(bioout, "Invalid Operation Exception IR %.8lux\n", inst);
	longjmp(errjmp, 0);
}

void
ifmt(int r)
{
	Bprint(bioout, "Invalid Floating Data Format f%d pc 0x%lux\n", r,
	       reg.r[15]);
	longjmp(errjmp, 0);
}

void
floatop(int dst, int s1, int s2)
{
}

void
doubop(int dst, int s1, int s2)
{
}

void
Iswc1(uint32_t inst)
{
}

void
Ifsub(uint32_t ir)
{
}

void
Ifmov(uint32_t ir)
{
}

void
Ifabs(uint32_t ir)
{
}

void
Ifneg(uint32_t ir)
{
}

void
Icvtd(uint32_t ir)
{
}

void
Icvts(uint32_t ir)
{
}

void
Icvtw(uint32_t ir)
{
}

void
Ifadd(uint32_t ir)
{
}

void
Ifmul(uint32_t ir)
{
}

void
Ifdiv(uint32_t ir)
{
}

void
Ilwc1(uint32_t inst)
{
}

void
Ibcfbct(uint32_t inst)
{
}

void
Imtct(uint32_t ir)
{
}

void
Imfcf(uint32_t ir)
{
}

void
Icop1(uint32_t ir)
{
}

void
Ifcmp(uint32_t ir)
{
}
