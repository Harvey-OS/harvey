#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"../port/error.h"
#include	"io.h"

/*
 *  find out fault address and type of access.
 *  Call common fault handler.
 */
void
faultmips(Ureg *ur, int user, int code)
{
	ulong addr;
	extern char *excname[];
	char buf[ERRLEN];
	int read;

	LEDON(LEDfault);
	addr = ur->badvaddr;
	if((addr & KZERO) && (0xffff0000 & addr) != USERADDR)
		LEDON(LEDkfault);
	addr &= ~(BY2PG-1);
	read = !(code==CTLBM || code==CTLBS);

	if(fault(addr, read) < 0){
		if(user){
			sprint(buf, "sys: trap: fault %s addr=0x%lux",
				read? "read" : "write", ur->badvaddr);
			postnote(u->p, 1, buf, NDebug);
			LEDOFF(LEDfault);
			return;
		}
		print("kernel %s badvaddr=0x%lux\n", excname[code], ur->badvaddr);
		print("status=0x%lux pc=0x%lux sp=0x%lux\n", ur->status, ur->pc, ur->sp);
		dumpregs(ur);
		panic("fault");
	}
	LEDOFF(LEDfault|LEDkfault);
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
