#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "arm.h"

void	unimp(ulong);
void	Ifcmp(ulong);
void	Ifdiv(ulong);
void	Ifmul(ulong);
void	Ifadd(ulong);
void	Ifsub(ulong);
void	Ifmov(ulong);
void	Icvtd(ulong);
void	Icvtw(ulong);
void	Icvts(ulong);
void	Ifabs(ulong);
void	Ifneg(ulong);

Inst cop1[] = {
	{ Ifadd,	"add.f", Ifloat },
	{ Ifsub,	"sub.f", Ifloat },
	{ Ifmul,	"mul.f", Ifloat },
	{ Ifdiv,	"div.f", Ifloat },
	{ unimp,	"", },
	{ Ifabs,	"abs.f", Ifloat },
	{ Ifmov,	"mov.f", Ifloat },
	{ Ifneg,	"neg.f", Ifloat },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ Icvts,	"cvt.s", Ifloat },
	{ Icvtd,	"cvt.d", Ifloat },
	{ unimp,	"", },
	{ unimp,	"", },
	{ Icvtw,	"cvt.w", Ifloat },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ unimp,	"", },
	{ Ifcmp,	"c.f",	 Ifloat },
	{ Ifcmp,	"c.un",  Ifloat },
	{ Ifcmp,	"c.eq",  Ifloat },
	{ Ifcmp,	"c.ueq", Ifloat },
	{ Ifcmp,	"c.olt", Ifloat },
	{ Ifcmp,	"c.ult", Ifloat },
	{ Ifcmp,	"c.ole", Ifloat },
	{ Ifcmp,	"c.ule", Ifloat },
	{ Ifcmp,	"c,sf",  Ifloat },
	{ Ifcmp,	"c.ngle",Ifloat },
	{ Ifcmp,	"c.seq", Ifloat },
	{ Ifcmp,	"c.ngl", Ifloat },
	{ Ifcmp,	"c.lt",  Ifloat },
	{ Ifcmp,	"c.nge", Ifloat },
	{ Ifcmp,	"c.le",  Ifloat },
	{ Ifcmp,	"c.ngt", Ifloat },
	{ 0 }
};

void
unimp(ulong inst)
{
	print("op %d\n", inst&0x3f);
	Bprint(bioout, "Unimplemented floating point Trap IR %.8lux\n", inst);
	longjmp(errjmp, 0);
}

void
inval(ulong inst)
{
	Bprint(bioout, "Invalid Operation Exception IR %.8lux\n", inst);
	longjmp(errjmp, 0);
}

void
ifmt(int r)
{
	Bprint(bioout, "Invalid Floating Data Format f%d pc 0x%lux\n", r, reg.r[15]);
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
Iswc1(ulong inst)
{
}

void
Ifsub(ulong ir)
{
}

void
Ifmov(ulong ir)
{
}

void
Ifabs(ulong ir)
{
}

void
Ifneg(ulong ir)
{
}

void
Icvtd(ulong ir)
{
}

void
Icvts(ulong ir)
{
}

void
Icvtw(ulong ir)
{
}

void
Ifadd(ulong ir)
{
}

void
Ifmul(ulong ir)
{
}

void
Ifdiv(ulong ir)
{
}

void
Ilwc1(ulong inst)
{
}

void
Ibcfbct(ulong inst)
{
}

void
Imtct(ulong ir)
{
}

void
Imfcf(ulong ir)
{
}

void
Icop1(ulong ir)
{
}

void
Ifcmp(ulong ir)
{
}
