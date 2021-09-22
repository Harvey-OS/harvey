#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "/sys/src/libc/9syscall/sys.h"

extern void sysr1(Ar0*, va_list);
extern void sys_errstr(Ar0*, va_list);
extern void sysbind(Ar0*, va_list);
extern void syschdir(Ar0*, va_list);
extern void sysclose(Ar0*, va_list);
extern void sysdup(Ar0*, va_list);
extern void sysalarm(Ar0*, va_list);
extern void sysexec(Ar0*, va_list);
extern void sysexits(Ar0*, va_list);
extern void sys_fsession(Ar0*, va_list);
extern void sysfauth(Ar0*, va_list);
extern void sys_fstat(Ar0*, va_list);
extern void syssegbrk(Ar0*, va_list);
extern void sys_mount(Ar0*, va_list);
extern void sysopen(Ar0*, va_list);
extern void sys_read(Ar0*, va_list);
extern void sysoseek(Ar0*, va_list);
extern void syssleep(Ar0*, va_list);
extern void sys_stat(Ar0*, va_list);
extern void sysrfork(Ar0*, va_list);
extern void sys_write(Ar0*, va_list);
extern void syspipe(Ar0*, va_list);
extern void syscreate(Ar0*, va_list);
extern void sysfd2path(Ar0*, va_list);
extern void sysbrk_(Ar0*, va_list);
extern void sysremove(Ar0*, va_list);
extern void sys_wstat(Ar0*, va_list);
extern void sys_fwstat(Ar0*, va_list);
extern void sysnotify(Ar0*, va_list);
extern void sysnoted(Ar0*, va_list);
extern void syssegattach(Ar0*, va_list);
extern void syssegdetach(Ar0*, va_list);
extern void syssegfree(Ar0*, va_list);
extern void syssegflush(Ar0*, va_list);
extern void sysrendezvous(Ar0*, va_list);
extern void sysunmount(Ar0*, va_list);
extern void sys_wait(Ar0*, va_list);
extern void syssemacquire(Ar0*, va_list);
extern void syssemrelease(Ar0*, va_list);
extern void sysseek(Ar0*, va_list);
extern void sysfversion(Ar0*, va_list);
extern void syserrstr(Ar0*, va_list);
extern void sysstat(Ar0*, va_list);
extern void sysfstat(Ar0*, va_list);
extern void syswstat(Ar0*, va_list);
extern void sysfwstat(Ar0*, va_list);
extern void sysmount(Ar0*, va_list);
extern void sysawait(Ar0*, va_list);
extern void syspread(Ar0*, va_list);
extern void syspwrite(Ar0*, va_list);
extern void systsemacquire(Ar0*, va_list);
extern void sysnsec(Ar0*, va_list);
struct {
	char*	n;
	void (*f)(Ar0*, va_list);
	Ar0	r;
} systab[] = {
	[SYSR1]		{ "Sysr1", sysr1, { .i = -1 } },
	[_ERRSTR]	{ "_errstr", sys_errstr, { .i = -1 } },
	[BIND]		{ "Bind", sysbind, { .i = -1 } },
	[CHDIR]		{ "Chdir", syschdir, { .i = -1 } },
	[CLOSE]		{ "Close", sysclose, { .i = -1 } },
	[DUP]		{ "Dup", sysdup, { .i = -1 } },
	[ALARM]		{ "Alarm", sysalarm, { .l = -1 } },
	[EXEC]		{ "Exec", sysexec, { .v = (void*)-1 } },
	[EXITS]		{ "Exits", sysexits, { .i = -1 } },
	[_FSESSION]	{ "_fsession", sys_fsession, { .i = -1 } },
	[FAUTH]		{ "Fauth", sysfauth, { .i = -1 } },
	[_FSTAT]	{ "_fstat", sys_fstat, { .i = -1 } },
	[SEGBRK]	{ "Segbrk", syssegbrk, { .v = (void*)-1 } },
	[_MOUNT]	{ "_mount", sys_mount, { .i = -1 } },
	[OPEN]		{ "Open", sysopen, { .i = -1 } },
	[_READ]		{ "_read", sys_read, { .l = -1 } },
	[OSEEK]		{ "Oseek", sysoseek, { .i = -1 } },
	[SLEEP]		{ "Sleep", syssleep, { .i = -1 } },
	[_STAT]		{ "_stat", sys_stat, { .i = -1 } },
	[RFORK]		{ "Rfork", sysrfork, { .i = -1 } },
	[_WRITE]	{ "_write", sys_write, { .l = -1 } },
	[PIPE]		{ "Pipe", syspipe, { .i = -1 } },
	[CREATE]	{ "Create", syscreate, { .i = -1 } },
	[FD2PATH]	{ "Fd2path", sysfd2path, { .i = -1 } },
	[BRK_]		{ "Brk", sysbrk_, { .i = -1 } },
	[REMOVE]	{ "Remove", sysremove, { .i = -1 } },
	[_WSTAT]	{ "_wstat", sys_wstat, { .i = -1 } },
	[_FWSTAT]	{ "_fwstat", sys_fwstat, { .i = -1 } },
	[NOTIFY]	{ "Notify", sysnotify, { .i = -1 } },
	[NOTED]		{ "Noted", sysnoted, { .i = -1 } },
	[SEGATTACH]	{ "Segattach", syssegattach, { .v = (void*)-1 } },
	[SEGDETACH]	{ "Segdetach", syssegdetach, { .i = -1 } },
	[SEGFREE]	{ "Segfree", syssegfree, { .i = -1 } },
	[SEGFLUSH]	{ "Segflush", syssegflush, { .i = -1 } },
	[RENDEZVOUS]	{ "Rendez", sysrendezvous, { .v = (void*)-1 } },
	[UNMOUNT]	{ "Unmount", sysunmount, { .i = -1 } },
	[_WAIT]		{ "_wait", sys_wait, { .i = -1 } },
	[SEMACQUIRE]	{ "Semacquire", syssemacquire, { .i = -1 } },
	[SEMRELEASE]	{ "Semrelease", syssemrelease, { .i = -1 } },
	[SEEK]		{ "Seek", sysseek, { .i = -1 } },
	[FVERSION]	{ "Fversion", sysfversion, { .i = -1 } },
	[ERRSTR]	{ "Errstr", syserrstr, { .i = -1 } },
	[STAT]		{ "Stat", sysstat, { .i = -1 } },
	[FSTAT]		{ "Fstat", sysfstat, { .i = -1 } },
	[WSTAT]		{ "Wstat", syswstat, { .i = -1 } },
	[FWSTAT]	{ "Fwstat", sysfwstat, { .i = -1 } },
	[MOUNT]		{ "Mount", sysmount, { .i = -1 } },
	[AWAIT]		{ "Await", sysawait, { .i = -1 } },
	[PREAD]		{ "Pread", syspread, { .l = -1 } },
	[PWRITE]	{ "Pwrite", syspwrite, { .l = -1 } },
	[TSEMACQUIRE]	{ "Tsemacquire", systsemacquire, { .i = -1 } },
	[NSEC]		{ "Nsec", sysnsec, { .i = -1 } },
};

int nsyscall = nelem(systab);
