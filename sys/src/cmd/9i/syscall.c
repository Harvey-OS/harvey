#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "sim.h"
#undef CHDIR

ulong	nofunc;

#define	SYSR1		0
#define	ERRSTR		1
#define	BIND		2
#define	CHDIR		3
#define	CLOSE		4
#define	DUP		5
#define	ALARM		6
#define	EXEC		7
#define	EXITS		8
#define	FORK		9
#define	FORKPGRP	10
#define	FSTAT		11
#define	SEGBRK		12
#define	MOUNT		13
#define	OPEN		14
#define	READ		15
#define	SEEK		16
#define	SLEEP		17
#define	STAT		18
#define	WAIT		19
#define	WRITE		20
#define	PIPE		21
#define	CREATE		22
#define	RFORK		23
#define	BRK_		24
#define	REMOVE		25
#define	WSTAT		26
#define	FWSTAT		27
#define	NOTIFY		28
#define	NOTED		29
#define SEGATTACH 	30
#define SEGDETACH 	31
#define SEGFREE   	32
#define SEGFLUSH	33
#define RENDEZVOUS	34
#define UNMOUNT		35

char *sysctab[]={
	[SYSR1]		"Running",
	[ERRSTR]	"Errstr",
	[BIND]		"Bind",
	[CHDIR]		"Chdir",
	[CLOSE]		"Close",
	[DUP]		"Dup",
	[ALARM]		"Alarm",
	[EXEC]		"Exec",
	[EXITS]		"Exits",
	[FORK]		"Fork",
	[FORKPGRP]	"Forkpgrp",
	[FSTAT]		"Fstat",
	[SEGBRK]	"Segbrk",
	[MOUNT]		"Mount",
	[OPEN]		"Open",
	[READ]		"Read",
	[SEEK]		"Seek",
	[SLEEP]		"Sleep",
	[STAT]		"Stat",
	[WAIT]		"Wait",
	[WRITE]		"Write",
	[PIPE]		"Pipe",
	[CREATE]	"Create",
	[RFORK]		"Rfork",
	[BRK_]		"Brk",
	[REMOVE]	"Remove",
	[WSTAT]		"Wstat",
	[FWSTAT]	"Fwstat",
	[NOTIFY]	"Notify",
	[NOTED]		"Noted",
	[SEGATTACH]	"Segattach",
	[SEGDETACH]	"Segdetach",
	[SEGFREE]	"Segfree",
	[SEGFLUSH]	"Segflush",
	[RENDEZVOUS]	"Rendez",
	[UNMOUNT]	"Unmount",
};

void sys1(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0); }

void
syserrstr(void)
{
	ulong str;

	str = getmem_w(reg.r[REGSP]+4);
	if(sysdbg)
		itrace("errstr(0x%lux)", str);

	memio(errbuf, str, ERRLEN, MemWrite);
	strcpy(errbuf, "no error");
	reg.r[REGRET] = 0;
	
}
void
sysbind(void)
{ 
	ulong pname, pold, flags;
	char name[1024], old[1024];
	int n;

	pname = getmem_w(reg.r[REGSP]+4);
	pold = getmem_w(reg.r[REGSP]+8);
	flags = getmem_w(reg.r[REGSP]+12);
	memio(name, pname, sizeof(name), MemReadstring);
	memio(old, pold, sizeof(old), MemReadstring);
	if(sysdbg)
		itrace("bind(0x%lux='%s', 0x%lux='%s', 0x%lux)", name, old, flags);

	n = bind(name, old, flags);
	if(n < 0)
		errstr(errbuf);

	reg.r[REGRET] = n;
}

void
syschdir(void)
{ 
	char file[1024];
	int n;
	ulong name;

	name = getmem_w(reg.r[REGSP]+4);
	memio(file, name, sizeof(file), MemReadstring);
	if(sysdbg)
		itrace("chdir(0x%lux='%s', 0x%lux)", name, file);
	
	n = chdir(file);
	if(n < 0)
		errstr(errbuf);

	reg.r[REGRET] = n;
}

void
sysclose(void)
{
	int n;
	ulong fd;

	fd = getmem_w(reg.r[REGSP]+4);
	if(sysdbg)
		itrace("close(%d)", fd);

	n = close(fd);
	if(n < 0)
		errstr(errbuf);
	reg.r[REGRET] = n;
}

void
sysdup(void)
{
	int oldfd, newfd;
	int n;

	oldfd = getmem_w(reg.r[REGSP]+4);
	newfd = getmem_w(reg.r[REGSP]+8);
	if(sysdbg)
		itrace("dup(%d, %d)", oldfd, newfd);

	n = dup(oldfd, newfd);
	if(n < 0)
		errstr(errbuf);
	reg.r[REGRET] = n;
}

void
sysexits(void)
{
	char buf[ERRLEN];
	ulong str;

	str = getmem_w(reg.r[REGSP]+4);
	if(sysdbg)
		itrace("exits(0x%lux)", str);

	count = 1;
	if(str != 0) {
		memio(buf, str, ERRLEN, MemRead);
		Bprint(bioout, "exits(%s)\n", buf);
	}
	else
		Bprint(bioout, "exits(0)\n");
}

void
sysopen(void)
{
	char file[1024];
	int n;
	ulong mode, name;

	name = getmem_w(reg.r[REGSP]+4);
	mode = getmem_w(reg.r[REGSP]+8);
	memio(file, name, sizeof(file), MemReadstring);
	if(sysdbg)
		itrace("open(0x%lux='%s', 0x%lux)", name, file, mode);
	
	n = open(file, mode);
	if(n < 0)
		errstr(errbuf);

	reg.r[REGRET] = n;
};

void
sysread(void)
{
	int fd;
	ulong size, a;
	char *buf, *p;
	int n, cnt, c;

	fd = getmem_w(reg.r[REGSP]+4);
	a = getmem_w(reg.r[REGSP]+8);
	size = getmem_w(reg.r[REGSP]+12);

	buf = emalloc(size);
	if(fd == 0) {
		print("\nstdin>>");
		p = buf;
		n = 0;
		cnt = size;
		while(cnt) {
			c = Bgetc(bin);
			if(c <= 0)
				break;
			*p++ = c;
			n++;
			cnt--;
			if(c == '\n')
				break;
		}
	}
	else
		n = read(fd, buf, size);

	if(n < 0)
		errstr(errbuf);
	else
		memio(buf, a, n, MemWrite);

	if(sysdbg)
		itrace("read(%d, 0x%lux, %d) = %d", fd, a, size, n);

	free(buf);
	reg.r[REGRET] = n;
}

void
sysseek(void)
{
	int fd, n;
	ulong off, mode;

	fd = getmem_w(reg.r[REGSP]+4);
	off = getmem_w(reg.r[REGSP]+8);
	mode = getmem_w(reg.r[REGSP]+12);
	if(sysdbg)
		itrace("seek(%d, %lud, %d)", fd, off, mode);

	n = seek(fd, off, mode);
	if(n < 0)
		errstr(errbuf);	

	reg.r[REGRET] = n;
}

void
syssleep(void)
{
	ulong len;
	int n;

	len = getmem_w(reg.r[REGSP]+4);
	if(sysdbg)
		itrace("sleep(%d)", len);

	n = sleep(len);
	if(n < 0)
		errstr(errbuf);	

	reg.r[REGRET] = n;
}

void
sysstat(void)
{
	char nambuf[1024];
	char buf[DIRLEN];
	ulong edir, name;
	int n;

	name = getmem_w(reg.r[REGSP]+4);
	edir = getmem_w(reg.r[REGSP]+8);
	memio(nambuf, name, sizeof(nambuf), MemReadstring);
	if(sysdbg)
		itrace("stat(0x%lux='%s', 0x%lux)", name, nambuf, edir);

	n = stat(nambuf, buf);
	if(n < 0)
		errstr(errbuf);
	else
		memio(buf, edir, DIRLEN, MemWrite);

	reg.r[REGRET] = n;
}

void
sysfstat(void)
{
	char buf[DIRLEN];
	ulong edir;
	int n, fd;

	fd = getmem_w(reg.r[REGSP]+4);
	edir = getmem_w(reg.r[REGSP]+8);
	if(sysdbg)
		itrace("fstat(%d, 0x%lux)", fd, edir);

	n = fstat(fd, buf);
	if(n < 0)
		errstr(errbuf);
	else
		memio(buf, edir, DIRLEN, MemWrite);

	reg.r[REGRET] = n;
}

void
syswrite(void)
{
	int fd;
	ulong size, a;
	char *buf;
	int n;

	fd = getmem_w(reg.r[REGSP]+4);
	a = getmem_w(reg.r[REGSP]+8);
	size = getmem_w(reg.r[REGSP]+12);
	if(sysdbg)
		itrace("write(%d, %lux, %d)", fd, a, size);

	buf = memio(0, a, size, MemRead);
	n = write(fd, buf, size);
	if(n < 0)
		errstr(errbuf);	
	free(buf);

	reg.r[REGRET] = n;
}

void
syspipe(void)
{
	int n, p[2];
	ulong fd;

	fd = getmem_w(reg.r[REGSP]+4);
	if(sysdbg)
		itrace("pipe(%lux)", fd);

	n = pipe(p);
	if(n < 0)
		errstr(errbuf);
	else {
		putmem_w(fd, p[0]);
		putmem_w(fd+4, p[1]);
	}
	reg.r[REGRET] = n;
}

void
syscreate(void)
{
	char file[1024];
	int n;
	ulong mode, name, perm;

	name = getmem_w(reg.r[REGSP]+4);
	mode = getmem_w(reg.r[REGSP]+8);
	perm = getmem_w(reg.r[REGSP]+12);
	memio(file, name, sizeof(file), MemReadstring);
	if(sysdbg)
		itrace("create(0x%lux='%s', 0x%lux, 0x%lux)", name, file, mode, perm);
	
	n = create(file, mode, perm);
	if(n < 0)
		errstr(errbuf);

	reg.r[REGRET] = n;
}

void
sysbrk_(void)
{
	ulong addr;

	addr = getmem_w(reg.r[REGSP]+4);
	if(sysdbg)
		itrace("brk_(0x%lux)", addr);

	reg.r[REGRET] = membrk(addr);
}

void
sysremove(void)
{
	char nambuf[1024];
	ulong name;
	int n;

	name = getmem_w(reg.r[REGSP]+4);
	memio(nambuf, name, sizeof(nambuf), MemReadstring);
	if(sysdbg)
		itrace("remove(0x%lux='%s')", name, nambuf);

	n = remove(nambuf);
	if(n < 0)
		errstr(errbuf);
	reg.r[REGRET] = n;
}

void
sysnotify(void)
{
	nofunc = getmem_w(reg.r[REGSP]+4);
	if(sysdbg)
		itrace("notify(0x%lux)\n", nofunc);

	reg.r[REGRET] = 0;
}

void syswait(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0); }
void sysrfork(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void syswstat(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysfwstat(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysnoted(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void syssegattach(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void syssegdetach(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void syssegfree(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void syssegflush(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysrendezvous(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysunmount(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysfork(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysforkpgrp(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void syssegbrk(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysmount(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysalarm(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysexec(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}

void (*systab[])(void)	={
	[SYSR1]		sys1,
	[ERRSTR]	syserrstr,
	[BIND]		sysbind,
	[CHDIR]		syschdir,
	[CLOSE]		sysclose,
	[DUP]		sysdup,
	[ALARM]		sysalarm,
	[EXEC]		sysexec,
	[EXITS]		sysexits,
	[FORK]		sysfork,
	[FORKPGRP]	sysforkpgrp,
	[FSTAT]		sysfstat,
	[SEGBRK]	syssegbrk,
	[MOUNT]		sysmount,
	[OPEN]		sysopen,
	[READ]		sysread,
	[SEEK]		sysseek,
	[SLEEP]		syssleep,
	[STAT]		sysstat,
	[WAIT]		syswait,
	[WRITE]		syswrite,
	[PIPE]		syspipe,
	[CREATE]	syscreate,
	[RFORK]		sysrfork,
	[BRK_]		sysbrk_,
	[REMOVE]	sysremove,
	[WSTAT]		syswstat,
	[FWSTAT]	sysfwstat,
	[NOTIFY]	sysnotify,
	[NOTED]		sysnoted,
	[SEGATTACH]	syssegattach,
	[SEGDETACH]	syssegdetach,
	[SEGFREE]	syssegfree,
	[SEGFLUSH]	syssegflush,
	[RENDEZVOUS]	sysrendezvous,
	[UNMOUNT]	sysunmount,
};

void
Ssyscall(ulong inst)
{
	int call;

	USED(inst);
	call = reg.r[REGRET];
	if(call < 0 || call > UNMOUNT) {
		Bprint(bioout, "Bad system call\n");
		dumpreg();
	}
	if(trace)
		itrace("sysc\t%s", sysctab[call]);

	(*systab[call])();
	Bflush(bioout);
}
