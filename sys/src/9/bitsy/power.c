#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"pool.h"

/* Power management for the bitsy */

#ifdef NOTDEF
enum {
	pmcr_sf		= 0,

	pcfr_opde	= 0,
	pcfr_fp		= 1,
	pcfr_fs		= 2,
	pcfr_fo		= 3,
};

jmp_buf power_resume;

static void
onoffintr(Ureg*, void *x)
{
	/* On/off button interrupt */
	int i;

	/* debounce, 50 ms*/
	for (i = 0; i < 50; i++) {
		delay(1);
		if ((gpioregs->level & GPIO_PWR_ON_i) == 0)
			return;	/* bounced */
	}
	if (i = setjmp(power_resume)) {
		/* power back up */
		rs232power(1);
		lcdpower(1);
		audiopower(1);
		irpower(1);
		return;
	}
	/* Power down */
	irpower(0);
	audiopower(0);
	lcdpower(0);
	rs232power(0);
	sa1100_power_off();
	/* no return */
}

static void
sa1100_power_off(void)
{
	delay(100);
	splhi();
	/* enable wakeup by µcontroller, on/off switch or real-time clock alarm */
	powerregs->pwer = 0x80000003;
	/* disable internal oscillator, float CS lines */
	powerregs->pcfr = 1<<pcfr_opde | 1<<pcfr_fp | 1<<pcfr_fs;
	/* set lowest clock */
	powerregs->ppcr = 0;
	/* set all GPIOs to input mode */
	gpioregs->direction = 0;
	/* enter sleep mode */
	powerregs->pmcr = 1<<pmcr_sf;
}

void
idlehands(void)
{
}
#endif

void
idlehands(void)
{
	char *msgb = "idlehands called with splhi\n";
	char *msga = "doze returns with splhi\n";

	if(!islo()){
		serialputs(msga, strlen(msga));
		spllo();
	}
	doze();
	if(!islo()){
		serialputs(msgb, strlen(msgb));
		spllo();
	}
}
