#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

/*
 *  headland system controller (ht21)
 */
enum
{
	/*
	 *  system control port
	 */
	Head=		0x92,		/* control port */
	 Reset=		(1<<0),		/* reset the 386 */
	 A20ena=	(1<<1),		/* enable address line 20 */
};

/*
 *  enable address bit 20
 */
void
heada20(void)
{
	outb(Head, A20ena);
}

/*
 *  reset machine
 */
void
headreset(void)
{
	outb(Head, Reset);
}
