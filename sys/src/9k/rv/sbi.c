/*
 * sbi calls for rv64
 *
 * legacy extensions, function 0 (present):
 * ext 0 set_timer(uvlong);
 * 1	console_putchar(int);
 * 2	int console_getchar(void);
 * 3	clear_ipi(void);
 * 4	send_ipi(ulong *hartmaskp);
 * 5	remote_fence_i(ulong *hartmaskp);
 * 6	remote_sfence_vma(ulong *hartmaskp, ulong start, ulong size);
 * 7	remote_sfence_vma_asid(ulong *hartmaskp, ulong start, ulong size,
 *		ulong asid);
 * 8	shutdown(void);
 *
 * extension 0x10 is the new "base" extension, replacing the legacy ones.
 *
 * v0.2 extension 0x48534d (HSM) (missing from icicle)
 * func 0 hart_start(ulong hartid, ulong resume_addr, ulong resume_new_a1);
 *	at resume, hart will see: R10 is hartid, R11 is resume_new_a1, satp 0,
 *	sie 0
 * 1	hart_stop(void);
 * 2	hart_status(ulong hartid);
 *
 * extensions present in OpenSBI v0.6 (allegedly SBI v0.2):
 * 0-8, 0x10, 0x735049 (IPs), 0x52464e43 (RFNC)
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "riscv.h"

vlong
sbigetimplid(void)
{
	Sbiret sbiret;

	if (sbicall(0x10, 1, 0, &sbiret, nil) < 0)
		return -1;
	return sbiret.status;
}

vlong
sbigetimplvers(void)
{
	Sbiret sbiret;

	if (sbicall(0x10, 2, 0, &sbiret, nil) < 0)
		return -1;
	return sbiret.status;
}

/* returns 0 if ext is not supported */
vlong
sbiprobeext(uvlong ext)
{
	Sbiret sbiret;

	if (sbicall(0x10, 3, ext, &sbiret, nil) < 0)
		return 0;
	return sbiret.status;
}

void
sbiputc(uvlong c)
{
	sbicall(1, 0, c, nil, nil);
}

vlong
sbiclearipi(void)
{
	return sbicall(3, 0, 0, nil, nil);
}

vlong
sbisendipi(uvlong *hart)
{
	/* this is the legacy call, not the new sIP extension */
	return sbicall(4, 0, (uintptr)hart, nil, nil);
}

void
sbisettimer(uvlong tm)
{
	sbicall(0, 0, tm, nil, nil);
}

void
sbishutdown(void)			/* shutdown all cpus; no return */
{
	sbicall(8, 0, 0, nil, nil);
}

/*
 * v0.2 hsm extensions
 *	funcs: 0 start, 1 caller returns to SBI, 2 status, 3 suspend self
 */

vlong
sbihartstart(uvlong hartid, uvlong physstart, uvlong private)
{
	uvlong xargs[2];

	xargs[0] = physstart;		/* for r11 */
	xargs[1] = private;		/* for r12 */
	return sbicall(HSM, 0, hartid, nil, xargs);
}

/*
 * status results: 0 started, 1 stopped, 2 start pending, 3 stop pending
 */
vlong
sbihartstatus(uvlong hartid)
{
	Sbiret sbiret;

	if (sbicall(HSM, 2, hartid, &sbiret, nil) < 0)
		return sbiret.error;
	return sbiret.status;
}

vlong
sbihartsuspend(void)
{
	uvlong xargs[2];

	xargs[0] = 0;			/* for r11 */
	xargs[1] = 0;			/* for r12 */
	return sbicall(HSM, 3, 0, nil, xargs);
}
