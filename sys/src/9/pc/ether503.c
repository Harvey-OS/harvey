#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "devtab.h"

#include "ether.h"

static int
reset(Ctlr *ctlr)
{
	USED(ctlr);

	return -1;
}

Board ether503 = {
	reset,
	0,			/* init */
	0,			/* attach */
	0,			/* mode */
	0,			/* receive */
	0,			/* transmit */
	0,			/* interrupt */
	0,			/* watch */

	0,			/* addr */
	0,			/* irq */
	0,			/* bit16 */

	0,			/* ram */
	0,			/* ramstart */
	0,			/* ramstop */

	0,			/* dp8390 */
	0,			/* data */
	0,			/* tstart */
	0,			/* pstart */
	0,			/* pstop */
};
