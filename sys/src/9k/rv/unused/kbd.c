/*
 * keyboard input via uart or htif
 *
 * Modern (c. 2017) machines tend to not have 8042 keyboard controllers,
 * but get their console input from USB keyboards or serial ports.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

static Queue *kbdq;		/* is ignored as first arg to kbdputc */
static int nokbd = 1;		/* flag: no PS/2 keyboard */

void
kbdenable(void)
{
	if (nokbd)
		return;
	kbdq = qopen(4*1024, 0, 0, 0);
	if(kbdq == nil)
		panic("kbdinit");
	qnoblock(kbdq, 1);
	addkbdq(kbdq, -1);
}
