/*
 * code to dump syscall arguments
 */

#include "defs.h"
#include "fns.h"
#undef	CHDIR	/* BUG */
#include "/sys/src/libc/9syscall/sys.h"

char *scalltab[] = {
	[SYSR1]		"sysr1()",
	[ERRSTR]	"errstr(%lux)",
	[BIND]		"bind(%lux, %lux, %lux)",
	[CHDIR]		"chdir(%lux)",
	[CLOSE]		"close(%lud)",
	[DUP]		"dup(%lud, %lud)",
	[ALARM]		"alarm(%lud)",
	[EXEC]		"exec(%lud, %lud)",
	[EXITS]		"exits(%lux)",
	[_X1]		"deprecated()",
	[_X2]		"deprecated()",
	[FSTAT]		"fstat(%lud, %lux)",
	[SEGBRK]	"segbrk(%lux, %lux)",
	[MOUNT]		"mount(%lud, %lux, %lud, %lux, %lux)",
	[OPEN]		"open(%lux, %luo)",
	[READ]		"read(%lud, %lux, %lud)",
	[SEEK]		"seek(%lud, %lud, %lud)",
	[SLEEP]		"sleep(%lud)",
	[STAT]		"stat(%lux, %lux)",
	[RFORK]		"rfork(%lux)",
	[WRITE]		"write(%lud, %lux, %lud)",
	[PIPE]		"pipe(%lux)",
	[CREATE]	"create(%lux, %lux, %luo)",
	[_X3]		"deprecated()",
	[BRK_]		"brk(%lux)",
	[REMOVE]	"remove(%lux)",
	[WSTAT]		"wstat(%lux, %lux)",
	[FWSTAT]	"fwstat(%lud, %lux)",
	[NOTIFY]	"notify(%lux)",
	[NOTED]		"noted(%lud)",
	[SEGATTACH]	"segattach(%lud, %lux, %lux, %lud)",
	[SEGDETACH]	"segdetach(%lux)",
	[SEGFREE]	"segfree(%lux, %lud)",
	[SEGFLUSH]	"segflush(%lux, %lud)",
	[RENDEZVOUS]	"rendezvous(%lux, %lud)",
	[UNMOUNT]	"unmount(%lux, %lux)",
	[WAIT]		"wait(%lux)",
};

	
void
printsyscall(void)
{
	long callnr;
	long sargs[5];
	ulong args;
	int i;

	if (get4(cormap, mach->scalloff, SEGREGS, &callnr) == 0)
		return;
	if (callnr >= sizeof(scalltab)/sizeof(scalltab[0]))
			return;
	for (args = mach->scalloff+4, i = 0; i < 5; i++, args += sizeof(ulong))
		if (get4(cormap, args, SEGREGS, &sargs[i]) == 0)
			return;
	dprint(scalltab[callnr], sargs[0], sargs[1], sargs[2], sargs[3], sargs[4]);
	dprint("\n");
}
