#include	"all.h"
#include	<ureg.h>

/*
 * CAUSE register
 */

#define	EXCCODE(c)	((c>>2)&0x0F)
#define INTCLEAR	IO(ulong, 0x400000)

ulong	ticks;

void
trap(Ureg *ur)
{
	uchar isr;
	int ecode = EXCCODE(ur->cause);
	int cause = ur->cause&(INTR5|INTR2|INTR1|INTR0);

	if(ecode != CINT)
		panic("ecode = 0x%lux", ecode);
	if(cause & INTR5){
		isr = *ISR;
		if(!(isr & ISRslot))
			print("expansion slot interrupt\n");
		else
			print("parity error\n");
		cause &= ~INTR5;
	}
	if(cause & INTR2){
		*TBREAK += COUNT; /* just let it overflow and wrap around */
		wbflush();
		ticks++;
		cause &= ~INTR2;
	}
	if(cause & INTR1){
		scsiintr();
		cause &= ~INTR1;
	}
	if(cause & INTR0){
		isr = *ISR;
		if(!(isr & ISRkybd))
			kbdintr();
		if(!(isr & ISRlance))
			print("lance intr\n");
		if(!(isr & ISRslot)){
			print("slot intr\n");
			*INTCLEAR = 1;
		}
		if(!(isr & ISRscc))
			sccintr();
		cause &= ~INTR0;
	}
	if(cause)
		panic("cause %lux isr %lux\n", cause, *ISR);
	splhi();
}
