#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"pool.h"

/* Power management for the bitsy */

/* saved state during power down. 
 * it's only used up to 164/4.
 * it's only used by routines in l.s
 */
ulong	power_state[200/4];
Rendez	powerr;
ulong	powerflag = 0;	/* set to start power-off sequence */

extern void	power_resume(void);
extern int		setpowerlabel(void);
extern void	_start(void);
extern Uart	sa1110uart[];

GPIOregs savedgpioregs;
Intrregs savedintrregs;

#define R(p) ((Uartregs*)((p)->regs))

uchar *savedtext;

static void
checkflash(void)
{
	ulong *p;
	ulong s;

	s = 0;
	for (p = (ulong*)FLASHZERO; p < (ulong*)(FLASHZERO+8*1024*1024); p++)
		s += *p;
	iprint("flash checksum is 0x%lux\n", s);
}

static void
checkktext(void)
{
	ulong *p;
	ulong s;

	s = 0;
	for (p = (ulong*)_start; p < (ulong*)etext; p++){
		if(*p == 0)
			iprint("0x%lux->0\n", p);
		if (((ulong)p & 0x1fff) == 0){
			iprint("page 0x%lux checksum 0x%lux\n",
				(ulong)(p-1)&~0x1fff, s);
			s = 0;
		}
		s += *p;
	}
	iprint("page 0x%lux checksum 0x%lux\n", (ulong)(p-1)&~0x1fff, s);
}

static void
checkpagetab(void)
{
	extern ulong l1table;
	ulong *p;
	ulong s;

	s = 0;
	for (p = (ulong*)l1table; p < (ulong*)(l1table+16*1024); p++)
		s += *p;
	iprint("page table checksum is 0x%lux\n", s);
}


static void
dumpitall(void)
{
	extern ulong l1table;

	iprint("intr: icip %lux iclr %lux iccr %lux icmr %lux\n",
		intrregs->icip,
		intrregs->iclr, intrregs->iccr, intrregs->icmr );
	iprint("gpio: lvl %lux dir %lux, re %lux, fe %lux sts %lux alt %lux\n",
		gpioregs->level,
		gpioregs->direction, gpioregs->rising, gpioregs->falling,
		gpioregs->edgestatus, gpioregs->altfunc);
	iprint("uart1: %lux %lux %lux \nuart3: %lux %lux %lux\n", 
		R(&sa1110uart[0])->ctl[0], R(&sa1110uart[0])->status[0], R(&sa1110uart[0])->status[1], 
		R(&sa1110uart[1])->ctl[0], R(&sa1110uart[1])->status[0], R(&sa1110uart[1])->status[1]); 
	iprint("tmr: osmr %lux %lux %lux %lux oscr %lux ossr %lux oier %lux\n",
		timerregs->osmr[0], timerregs->osmr[1],
		timerregs->osmr[2], timerregs->osmr[3],
		timerregs->oscr, timerregs->ossr, timerregs->oier);
	iprint("dram: mdcnfg %lux mdrefr %lux cas %lux %lux %lux %lux %lux %lux\n",
		memconfregs->mdcnfg, memconfregs->mdrefr,
		memconfregs->mdcas00, memconfregs->mdcas01,memconfregs->mdcas02,
		memconfregs->mdcas20, memconfregs->mdcas21,memconfregs->mdcas22); 
	iprint("dram: mdcnfg msc %lux %lux %lux mecr %lux\n",
		memconfregs->msc0, memconfregs->msc1,memconfregs->msc2,
		memconfregs->mecr);
	iprint("mmu: CpControl %lux CpTTB %lux CpDAC %lux l1table 0x%lux\n",
		getcontrol(), getttb(), getdac(), l1table);
	iprint("powerregs: pmcr %lux pssr %lux pcfr %lux ppcr %lux pwer %lux pspr %lux pgsr %lux posr %lux\n",
		powerregs->pmcr, powerregs->pssr, powerregs->pcfr, powerregs->ppcr,
		powerregs->pwer, powerregs->pspr, powerregs->pgsr, powerregs->posr);
	checkpagetab();
	checkflash();
	checkktext();
	iprint("\n\n");
}

static void
intrcpy(Intrregs *to, Intrregs *from)
{
	to->iclr = from->iclr;
	to->iccr = from->iccr;
	to->icmr = from->icmr;	// interrupts enabled
}

static void
gpiosave(GPIOregs *to, GPIOregs *from)
{
	to->level = from->level;
	to->rising = from->rising;		// gpio intrs enabled
	to->falling= from->falling;		// gpio intrs enabled
	to->altfunc = from->altfunc;
	to->direction = from->direction;
}

static void
gpiorestore(GPIOregs *to, GPIOregs *from)
{
	to->direction = from->direction;
	to->altfunc = from->altfunc;
	to->set = from->level & 0x0fffffff;
	to->clear = ~from->level & 0x0fffffff;
	to->rising = from->rising;		// gpio intrs enabled
	to->falling= from->falling;		// gpio intrs enabled
}

void	(*restart)(void) = nil;

static int
bitno(ulong x)
{
	int i;

	for(i = 0; i < 8*sizeof(x); i++)
		if((1<<i) & x)
			break;
	return i;
}

int
powerdown(void *)
{
	return powerflag;
}

void
deepsleep(void) {
	static int power_pl;
	ulong xsp, xlink, mecr;

	power_pl = splhi();
	xlink = getcallerpc(&xlink);
	/* Power down */
	irpower(0);
	audiopower(0);
	screenpower(0);
	µcpower(0);
	iprint("entering suspend mode, sp = 0x%lux, pc = 0x%lux, psw = 0x%ux\n", &xsp, xlink, power_pl);
//	dumpitall();
	delay(1000);
	uartpower(0);
	rs232power(0);
	clockpower(0);
	gpiosave(&savedgpioregs, gpioregs);
	intrcpy(&savedintrregs, intrregs);
	cacheflush();
	delay(50);
	mecr = memconfregs->mecr;
	if(setpowerlabel()){
		/* return here with mmu back on */
		trapresume();

		/* Turn off memory auto power */
//		memconfregs->mdrefr &= ~0x30000000;
		memconfregs->mecr = mecr;

		gpiorestore(gpioregs, &savedgpioregs);
		delay(50);
		intrcpy(intrregs, &savedintrregs);
		if(intrregs->icip & (1<<IRQgpio0)){
			// don't want to sleep now. clear on/off irq.
			gpioregs->edgestatus = (1<<IRQgpio0);
			intrregs->icip = (1<<IRQgpio0);
		}
		clockpower(1);
		gpclkregs->r0 = 1<<0;
		rs232power(1);
		uartpower(1);
		delay(100);
		xlink = getcallerpc(&xlink);
		iprint("\nresuming execution, sp = 0x%lux, pc = 0x%lux, psw = 0x%ux\n", &xsp, xlink, splhi());
//		dumpitall();
		delay(1000);
//		irpower(1);
//		audiopower(1);
		µcpower(1);
		screenpower(1);
		splx(power_pl);
		return;
	}
	cacheflush();
	delay(100);
	power_down();
	/* no return */
}

void
powerkproc(void*)
{
	ulong xlink, xlink1;

	for(;;){
		while(powerflag == 0)
			sleep(&powerr, powerdown, 0);

		xlink = getcallerpc(&xlink);

//		iprint("call deepsleep, pc = 0x%lux, sp = 0x%lux\n", xlink, &xlink);
		deepsleep();
		xlink1 = getcallerpc(&xlink1);

//		iprint("deepsleep returned, pc = 0x%lux, sp = 0x%lux\n", xlink1, &xlink);

		delay(2000);

		powerflag = 0;
	}
}

void
onoffintr(Ureg* , void*)
{
	int i;

	/* Power down interrupt comes on power button release.
	 * Act only after button has been released a full 100 ms
	 */

	if (powerflag)
		return;

	for (i = 0; i < 100; i++) {
		delay(1);
		if ((gpioregs->level & GPIO_PWR_ON_i) == 0)
			return;	/* bounced */
	}

	powerflag = 1;
	wakeup(&powerr);
}

static void
blanktimer(void)
{
	drawactive(0);
}

void
powerinit(void)
{
	addclock0link(blanktimer);
	intrenable(GPIOrising, bitno(GPIO_PWR_ON_i), onoffintr, nil, "on/off");
}

void
idlehands(void)
{
	char *msgb = "idlehands called with splhi\n";
	char *msga = "doze returns with splhi\n";

	if(!islo()){
		uartputs(msga, strlen(msga));
		spllo();
	}
	doze();
	if(!islo()){
		uartputs(msgb, strlen(msgb));
		spllo();
	}
}

