/*
 * process device configuration descriptions.
 * notably, vmap soc devices to keep them accessible once user processes
 * are mapped.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "riscv.h"

enum {
	Debug = 0,
};

/* see kernel config "conf" section for soc, ioconfs, socconf, iophysaddrset */
Ioconf ioconfs[];
Ioconf socconf[];

void	iophysaddrset(void);

int
ioconsec(Ioconf *io)
{
	return io->consec? io->consec: 1;
}

Ioconf *
ioarrconf(Ioconf *array, char *name, int unit)
{
	Ioconf *io;

	for (io = array; io->name; io++)
		if (strcmp(name, io->name) == 0 && unit >= io->baseunit &&
		    unit < io->baseunit + ioconsec(io))
			return io;
	return nil;
}

Ioconf *
ioconf(char *name, int unit)
{
	Ioconf *io;

	io = ioarrconf(ioconfs, name, unit);
	if (io == nil)
		io = ioarrconf(socconf, name, unit);
	return io;
}

/*
 * map low device registers to KSEG0 addresses to keep them
 * visible to the kernel when user processes are mapped.
 * only map devices here that lack drivers that vmap their registers.
 */
void
iovmapsoc(void)
{
	int i, consec;
	Ioconf *io;

	uartsetregs(0, (uintptr)soc.uart);

	iophysaddrset();
	for (io = socconf; io->name; io++) {
		/* if ioregs is supplied only in socaddr[0], use it */
		if (io->ioregs == 0 && io->socaddr[0] != 0)
			io->ioregs = (uintptr)io->socaddr[0];
		if (io->ioregs == 0)		/* somebody's oversight? */
			continue;
		consec = ioconsec(io);
		if (Debug)
			iprint("vmap %s @ %#p <- ", io->name, io->ioregs);
		io->ioregs = (uintptr)evmap(io->ioregs, io->regssize*consec);
		if (Debug)
			iprint("va %#p\n", io->ioregs);
		for (i = 0; consec-- > 0 && io->socaddr; i++)
			io->socaddr[i] = (uchar *)io->ioregs + io->regssize*i;
	}
	uartsetregs(0, (uintptr)soc.uart);
}
