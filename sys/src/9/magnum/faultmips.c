#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"../port/error.h"

void	kernfault(Ureg*, int);
/*
 *  find out fault address and type of access.
 *  Call common fault handler.
 */
void
faultmips(Ureg *ur, int user, int code)
{
	ulong addr;
	extern char *excname[];
	int read;
	char buf[ERRLEN];

	addr = ur->badvaddr;
	addr &= ~(BY2PG-1);
	read = !(code==CTLBM || code==CTLBS);

/* print("fault %s pc=0x%lux addr=0x%lux", read? "read" : "write", ur->pc, ur->badvaddr); /**/

	if(fault(addr, read) < 0){
		if(user){
			sprint(buf, "sys: trap: fault %s addr=0x%lux",
				read? "read" : "write", ur->badvaddr);
			postnote(u->p, 1, buf, NDebug);
			return;
		}
		kernfault(ur, code);
	}
}

/*
 * called in sysfile.c
 */
void
evenaddr(ulong addr)
{
	if(addr & 3){
		postnote(u->p, 1, "sys: odd address", NDebug);
		error(Ebadarg);
	}
}
