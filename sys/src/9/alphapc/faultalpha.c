#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"../port/error.h"

/*
 *  find out fault address and type of access.
 *  Call common fault handler.
 */
void
faultalpha(Ureg *ur)
{
	ulong addr, cause;
	int read, user;
	char buf[ERRMAX];
	uvlong x;

	x = ur->a0&0xffffffff80000000LL;
	if (x != 0LL && x != 0xffffffff80000000LL)
		iprint("faultalpha bad addr %llux pc %llux\n", ur->a0, ur->pc);

	addr = (ulong)ur->a0;
	cause = (ulong)ur->a2;
	addr &= ~(BY2PG-1);
	read = (cause !=1);
	user = (ulong)ur->status&UMODE;

/*	print("fault %s pc=0x%lux addr=0x%lux 0x%lux\n",
		read? (cause != 0) ? "ifetch" : "read" : "write", (ulong)ur->pc, addr, (ulong)ur->a1); /**/

	if(fault(addr, read) == 0)
		return;

	if(user){
		sprint(buf, "sys: trap: fault %s addr=0x%lux",
			read? (cause != 0) ? "ifetch" : "read" : "write", (ulong)ur->a0);
		postnote(up, 1, buf, NDebug);
		return;
	}

	iprint("kernel %s vaddr=0x%lux\n", read? (cause != 0) ? "ifetch" : "read" : "write", (ulong)ur->a0);
	if(0)
		mmudump();
	dumpregs(ur);
	_dumpstack(ur);
	exit(1);
}

/*
 * called in sysfile.c
 */
void
evenaddr(ulong addr)
{
	if(addr & 3){
		postnote(up, 1, "sys: odd address", NDebug);
		error(Ebadarg);
	}
}
