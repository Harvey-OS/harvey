/*
 * Interface to Advanced Power Management 1.2 BIOS
 *
 * This is, in many ways, a giant hack, and when things settle down 
 * a bit and standardize, hopefully we can write a driver that deals
 * more directly with the hardware and thus might be a bit cleaner.
 * 
 * ACPI might be the answer, but at the moment this is simpler
 * and more widespread.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

extern int apmfarcall(ushort, ulong, Ureg*);		/* apmjump.s */

static int
getreg(ulong *reg, ISAConf *isa, char *name)
{
	int i;
	int nl;

	nl = strlen(name);
	for(i=0; i<isa->nopt; i++){
		if(cistrncmp(isa->opt[i], name, nl)==0 && isa->opt[i][nl] == '='){
			*reg = strtoul(isa->opt[i]+nl+1, nil, 16);
			return 0;
		}
	}
	return -1;
}

/*
 * Segment descriptors look like this.
 *
 * d1: [base 31:24] [gran] [is32bit] [0] [unused] [limit 19:16] 
		[present] [privlev] [type 3:0] [base 23:16]
 * d0: [base 15:00] [limit 15:00]
 *
 * gran is 0 for 1-byte granularity, 1 for 4k granularity
 * type is 0 for system segment, 1 for code/data.
 *
 * clearly we know way too much about the memory unit.
 * however, knowing this much about the memory unit
 * means that the memory unit need not know anything
 * about us.
 *
 * what a crock.
 */
static void
setgdt(int sel, ulong base, ulong limit, int flag)
{
	if(sel < 0 || sel >= NGDT)
		panic("setgdt");

	base = (ulong)KADDR(base);
	m->gdt[sel].d0 = (base<<16) | (limit&0xFFFF);
	m->gdt[sel].d1 = (base&0xFF000000) | (limit&0x000F0000) |
			((base>>16)&0xFF) | SEGP | SEGPL(0) | flag;
}

static	ulong ax, cx, dx, di, ebx, esi;
static Ureg apmu;
static long
apmread(Chan*, void *a, long n, vlong off)
{
	if(off < 0)
		error("badarg");

	if(n+off > sizeof apmu)
		n = sizeof apmu - off;
	if(n <= 0)
		return 0;
	memmove(a, (char*)&apmu+off, n);
	return n;
}

static long
apmwrite(Chan*, void *a, long n, vlong off)
{
	int s;
	if(off || n != sizeof apmu)
		error("write a Ureg");

	memmove(&apmu, a, sizeof apmu);
	s = splhi();
	apmfarcall(APMCSEL, ebx, &apmu);
	splx(s);
	return n;
}

void
apmlink(void)
{
	ISAConf isa;
	char *s;

	if(isaconfig("apm", 0, &isa) == 0)
		return;

	/*
	 * APM info passed from boot loader.
	 * Now we need to set up the GDT entries for APM.
	 *
	 * AX = 32-bit code segment base address
	 * EBX = 32-bit code segment offset
	 * CX = 16-bit code segment base address
	 * DX = 32-bit data segment base address
	 * ESI = <16-bit code segment length> <32-bit code segment length> (hi then lo)
	 * DI = 32-bit data segment length
	 */

	if(getreg(&ax, &isa, s="ax") < 0
	|| getreg(&ebx, &isa, s="ebx") < 0
	|| getreg(&cx, &isa, s="cx") < 0
	|| getreg(&dx, &isa, s="dx") < 0
	|| getreg(&esi, &isa, s="esi") < 0
	|| getreg(&di, &isa, s="di") < 0){
		print("apm: missing register %s\n", s);
		return;
	}

	/*
	 * The NEC Versa SX bios does not report the correct 16-bit code
	 * segment length when loaded directly from mbr -> 9load (as compared
	 * with going through ld.com).  We'll make both code segments 64k-1 bytes.
	 */
	esi = 0xFFFFFFFF;

	/*
	 * We are required by the BIOS to set up three consecutive segments,
	 * one for the APM 32-bit code, one for the APM 16-bit code, and 
	 * one for the APM data.  The BIOS handler uses the code segment it
	 * get called with to determine the other two segment selector.
	 */
	setgdt(APMCSEG, ax<<4, ((esi&0xFFFF)-1)&0xFFFF, SEGEXEC|SEGR|SEGD);
	setgdt(APMCSEG16, cx<<4, ((esi>>16)-1)&0xFFFF, SEGEXEC|SEGR);
	setgdt(APMDSEG, dx<<4, (di-1)&0xFFFF, SEGDATA|SEGW|SEGD);

	addarchfile("apm", 0660, apmread, apmwrite);

print("apm0: configured cbase %.8lux off %.8lux\n", ax<<4, ebx);

	return;
}

