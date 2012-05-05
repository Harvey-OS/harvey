/*
 * Linux 386 fpu support
 * Mimic Plan9 floating point support
 */

static void
setfcr(ulong fcr)
{
	__asm__(	"xorb	$0x3f, %%al\n\t"
			"pushw	%%ax\n\t"
			"fwait\n\t"
			"fldcw	(%%esp)\n\t"
			"popw	%%ax\n\t"
			: /* no output */
			: "al" (fcr)
	);
}

static ulong
getfcr(void)
{
	ulong fcr = 0;

	__asm__(	"pushl	%%eax\n\t"
			"fwait\n\t"
			"fstcw	(%%esp)\n\t"
			"popl	%%eax\n\t"
			"xorb	$0x3f, %%al\n\t"
			: "=a"  (fcr)
			: "eax"	(fcr)
	);
	return fcr; 
}

static ulong
getfsr(void)
{
	ulong fsr = -1;

	__asm__(	"fwait\n\t"
			"fstsw	(%%eax)\n\t"
			"movl	(%%eax), %%eax\n\t"
			"andl	$0xffff, %%eax\n\t"
			: "=a"  (fsr)
			: "eax" (&fsr)
	);
	return fsr;
}

static void
setfsr(ulong fsr)
{
	__asm__("fclex\n\t");
}

/* FCR */
#define	FPINEX	(1<<5)
#define	FPUNFL	((1<<4)|(1<<1))
#define	FPOVFL	(1<<3)
#define	FPZDIV	(1<<2)
#define	FPINVAL	(1<<0)
#define	FPRNR	(0<<10)
#define	FPRZ	(3<<10)
#define	FPRPINF	(2<<10)
#define	FPRNINF	(1<<10)
#define	FPRMASK	(3<<10)
#define	FPPEXT	(3<<8)
#define	FPPSGL	(0<<8)
#define	FPPDBL	(2<<8)
#define	FPPMASK	(3<<8)
/* FSR */
#define	FPAINEX	FPINEX
#define	FPAOVFL	FPOVFL
#define	FPAUNFL	FPUNFL
#define	FPAZDIV	FPZDIV
#define	FPAINVAL	FPINVAL
