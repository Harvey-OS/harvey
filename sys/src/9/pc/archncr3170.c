#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

enum
{
	Pindex=	0x198,
	Pdata=	0x199,
};

static Lock pmulock;
static int
pmuwrbit(int index, int bit, int pos)
{
	uchar x;

	lock(&pmulock);
	outb(Pindex, index);
	x = inb(Pdata);
	if(bit)
		x |= 1<<pos;
	else
		x &= ~(1<<pos);
	outb(Pindex, index);
	outb(Pdata, x);
	unlock(&pmulock);
	return 0;
}

static int
ncr3170serialpower(int onoff)
{
	return pmuwrbit(0x85, 1^onoff, 4);
}

PCArch ncr3170 =
{
	"NCRD.0",
	i8042reset,
	0,
	0,
	0,
	ncr3170serialpower,
	0,
	0,
};
