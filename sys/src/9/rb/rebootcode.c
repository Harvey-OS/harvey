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
void	next_kernel(void *, ulong, char **, char **, ulong);

/*
 * Copy the new kernel to its correct location in memory,
 * flush caches, ignore TLBs (we're in KSEG0 space), and jump to
 * the start of the kernel.  Argument addresses are virtual (KSEG0).
 */
void
main(Rbconf *rbconf, ulong aentry, ulong acode, ulong asize)
{
	static ulong entry, code, size, argc;
	static char **argv;

	putc('B'); putc('o');
	/* copy args to heap before moving stack to before a.out header */
	argv = (char **)rbconf;
	entry = aentry;
	code = acode;
	size = asize;
	setsp(entry-0x20-4);
	putc('o');

	memmove((void *)entry, (void *)code, size);
	putc('t');
	cleancache();
	coherence();
	putc(' ');

	/*
	 * jump to new kernel's entry point.
	 * pass routerboot arg vector in R4-R5.
	 * off we go - never to return.
	 */
	next_kernel((void*)entry, 4, argv, 0, 0);

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
