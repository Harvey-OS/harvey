/*
 * Raspberry Pi GPIO support
 */

#include "u.h"
#include "../port/lib.h"
#include "../port/error.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#define GPIOREGS	(VIRTIO+0x200000)

/* GPIO regs */
enum {
	Fsel0	= 0x00>>2,
		FuncMask= 0x7,
	Set0	= 0x1c>>2,
	Clr0	= 0x28>>2,
	Lev0	= 0x34>>2,
	PUD	= 0x94>>2,
		Off	= 0x0,
		Pulldown= 0x1,
		Pullup	= 0x2,
	PUDclk0	= 0x98>>2,
	PUDclk1	= 0x9c>>2,
	/* BCM2711 only */
	PUPPDN0	= 0xe4>>2,
	PUPPDN1	= 0xe8>>2,
	PUPPDN2	= 0xec>>2,
	PUPPDN3	= 0xf0>>2,
};

void
gpiosel(uint pin, int func)
{	
	u32int *gp, *fsel;
	int off;

	gp = (u32int*)GPIOREGS;
	fsel = &gp[Fsel0 + pin/10];
	off = (pin % 10) * 3;
	*fsel = (*fsel & ~(FuncMask<<off)) | func<<off;
}

static void
gpiopull(uint pin, int func)
{
	u32int *gp, *reg;
	u32int mask;
	int shift;
	static uchar map[4] = {0x00,0x02,0x01,0x00};

	gp = (u32int*)GPIOREGS;
	if(gp[PUPPDN3] == 0x6770696f){
		/* BCM2835, BCM2836, BCM2837 */
		reg = &gp[PUDclk0 + pin/32];
		mask = 1 << (pin % 32);
		gp[PUD] = func;
		microdelay(1);
		*reg = mask;
		microdelay(1);
		*reg = 0;
	} else {
		/* BCM2711 */
		reg = &gp[PUPPDN0 + pin/16];
		shift = 2*(pin % 16);
		*reg = (map[func] << shift) | (*reg & ~(3<<shift));
	}

}

void
gpiopulloff(uint pin)
{
	gpiopull(pin, Off);
}

void
gpiopullup(uint pin)
{
	gpiopull(pin, Pullup);
}

void
gpiopulldown(uint pin)
{
	gpiopull(pin, Pulldown);
}

void
gpioout(uint pin, int set)
{
	u32int *gp;
	int v;

	gp = (u32int*)GPIOREGS;
	v = set? Set0 : Clr0;
	gp[v + pin/32] = 1 << (pin % 32);
}

int
gpioin(uint pin)
{
	u32int *gp;

	gp = (u32int*)GPIOREGS;
	return (gp[Lev0 + pin/32] & (1 << (pin % 32))) != 0;
}

