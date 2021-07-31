#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"io.h"

int faulting;

void
fault386(Ureg *ur)
{
	ulong addr;
	int read;
	int user;
	int n;
	int insyscall;
	char buf[ERRLEN];

	insyscall = u->p->insyscall;
	u->p->insyscall = 1;
	addr = getcr2();
	read = !(ur->ecode & 2);
	user = (ur->cs&0xffff) == UESEL;
	spllo();
	n = fault(addr, read);
	if(n < 0){
		if(user){
			sprint(buf, "sys: trap: fault %s addr=0x%lux",
				read? "read" : "write", addr);
			postnote(u->p, 1, buf, NDebug);
			return;
		}
		dumpregs(ur);
		panic("fault: 0x%lux", addr);
	}
	u->p->insyscall = insyscall;
}

void
faultinit(void)
{
	setvec(Faultvec, fault386);
}
