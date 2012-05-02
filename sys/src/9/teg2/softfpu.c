#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

int
fpudevprocio(Proc* proc, void* a, long n, uintptr offset, int write)
{
	/*
	 * Called from procdevtab.read and procdevtab.write
	 * allow user process access to the FPU registers.
	 * This is the only FPU routine which is called directly
	 * from the port code; it would be nice to have dynamic
	 * creation of entries in the device file trees...
	 */
	USED(proc, a, n, offset, write);

	return 0;
}

void
fpunotify(Ureg*)
{
	/*
	 * Called when a note is about to be delivered to a
	 * user process, usually at the end of a system call.
	 * Note handlers are not allowed to use the FPU so
	 * the state is marked (after saving if necessary) and
	 * checked in the Device Not Available handler.
	 */
}

void
fpunoted(void)
{
	/*
	 * Called from sysnoted() via the machine-dependent
	 * noted() routine.
	 * Clear the flag set above in fpunotify().
	 */
}

void
fpusysrfork(Ureg*)
{
	/*
	 * Called early in the non-interruptible path of
	 * sysrfork() via the machine-dependent syscall() routine.
	 * Save the state so that it can be easily copied
	 * to the child process later.
	 */
}

void
fpusysrforkchild(Proc*, Ureg *, Proc*)
{
	/*
	 * Called later in sysrfork() via the machine-dependent
	 * sysrforkchild() routine.
	 * Copy the parent FPU state to the child.
	 */
}

void
fpuprocsave(Proc*)
{
	/*
	 * Called from sched() and sleep() via the machine-dependent
	 * procsave() routine.
	 * About to go in to the scheduler.
	 * If the process wasn't using the FPU
	 * there's nothing to do.
	 */
}

void
fpuprocrestore(Proc*)
{
	/*
	 * The process has been rescheduled and is about to run.
	 * Nothing to do here right now. If the process tries to use
	 * the FPU again it will cause a Device Not Available
	 * exception and the state will then be restored.
	 */
}

void
fpusysprocsetup(Proc*)
{
	/*
	 * Disable the FPU.
	 * Called from sysexec() via sysprocsetup() to
	 * set the FPU for the new process.
	 */
}

void
fpuinit(void)
{
}

int
fpuemu(Ureg* ureg)
{
	int nfp;

	if(waserror()){
		splhi();
		postnote(up, 1, up->errstr, NDebug);
		return 1;
	}
	spllo();
	nfp = fpiarm(ureg);
	splhi();
	poperror();

	return nfp;
}

void
fpon(void)
{
}

void
fpoff(void)
{
}
