/*
 * mips reboot trampoline code
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

#define csr8r(r)	(((ulong *)PHYSCONS)[r])
#define csr8o(r, v)	(((ulong *)PHYSCONS)[r] = (v))

enum {					/* i8250 registers */
	Thr		= 0,		/* Transmitter Holding (WO) */
	Lsr		= 5,		/* Line Status */
};
enum {					/* Lsr */
	Thre		= 0x20,		/* Thr Empty */
};

void	putc(int);

/*
 * Copy the new kernel to its correct location in physical memory,
 * flush caches, ignore TLBs (we're in KSEG0 space), and jump to
 * the start of the kernel.
 */
void
main(ulong aentry, ulong acode, ulong asize)
{
	void (*kernel)(void);
	static ulong entry, code, size;

	putc('B'); putc('o'); putc('o'); putc('t');
	/* copy args to heap before moving stack to before a.out header */
	entry = aentry;
	code = acode;
	size = asize;
	setsp(entry-0x20-4);

	memmove((void *)entry, (void *)code, size);

	cleancache();
	coherence();

	/*
	 * jump to kernel entry point.
	 */
	putc(' ');
	kernel = (void*)entry;
	(*kernel)();			/* off we go - never to return */

	putc('?');
	putc('!');
	for(;;)
		;
}

void
putc(int c)
{
	int i;

	for(i = 0; !(csr8r(Lsr) & Thre) && i < 1000000; i++)
		;
	csr8o(Thr, (uchar)c);
	for(i = 0; !(csr8r(Lsr) & Thre) && i < 1000000; i++)
		;
}

long
syscall(Ureg*)
{
	return -1;
}

void
trap(Ureg *)
{
}
