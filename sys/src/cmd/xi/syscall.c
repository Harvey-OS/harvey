#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "3210.h"
#undef CHDIR

char 	errbuf[ERRLEN];
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

int
syserrstr(long sp)
{
	ulong str;

	str = getmem_w(sp);
	if(sysdbg)
		itrace("errstr(0x%lux)", str);

	memio(errbuf, str, ERRLEN, MemWrite);
	strcpy(errbuf, "no error");
	return 0;
	
}

int
sysbind(long sp)
{ 
	ulong pname, pold, flags;
	char name[1024], old[1024];
	int n;

	pname = getmem_w(sp);
	pold = getmem_w(sp+4);
	flags = getmem_w(sp+8);
	memio(name, pname, sizeof(name), MemReadstring);
	memio(old, pold, sizeof(old), MemReadstring);
	if(sysdbg)
		itrace("bind(0x%lux='%s', 0x%lux='%s', 0x%lux)", name, old, flags);

	n = bind(name, old, flags);
	if(n < 0)
		errstr(errbuf);

	return n;
}

int
syschdir(long sp)
{ 
	char file[1024];
	int n;
	ulong name;

	name = getmem_w(sp);
	memio(file, name, sizeof(file), MemReadstring);
	if(sysdbg)
		itrace("chdir(0x%lux='%s', 0x%lux)", name, file);
	
	n = chdir(file);
	if(n < 0)
		errstr(errbuf);

	return n;
}

int
sysclose(long sp)
{
	int n;
	ulong fd;

	fd = getmem_w(sp);
	if(sysdbg)
		itrace("close(%d)", fd);

	n = close(fd);
	if(n < 0)
		errstr(errbuf);
	return n;
}

int
sysdup(long sp)
{
	int oldfd, newfd;
	int n;

	oldfd = getmem_w(sp);
	newfd = getmem_w(sp+4);
	if(sysdbg)
		itrace("dup(%d, %d)", oldfd, newfd);

	n = dup(oldfd, newfd);
	if(n < 0)
		errstr(errbuf);
	return n;
}

int
sysexits(long sp)
{
	char buf[ERRLEN];
	ulong str;

	str = getmem_w(sp);
	if(sysdbg)
		itrace("exits(0x%lux)", str);

	count = 1;
	if(str != 0) {
		memio(buf, str, ERRLEN, MemRead);
		Bprint(bioout, "exits(%s)\n", buf);
	}
	else
		Bprint(bioout, "exits(0)\n");
	return 0;
}

int
sysopen(long sp)
{
	char file[1024];
	int n;
	ulong mode, name;

	name = getmem_w(sp);
	mode = getmem_w(sp+4);
	memio(file, name, sizeof(file), MemReadstring);
	if(sysdbg)
		itrace("open(0x%lux='%s', 0x%lux)", name, file, mode);
	
	n = open(file, mode);
	if(n < 0)
		errstr(errbuf);

	return n;
};

int
sysread(long sp)
{
	int fd;
	ulong size, a;
	char *buf, *p;
	int n, cnt, c;

	fd = getmem_w(sp);
	a = getmem_w(sp+4);
	size = getmem_w(sp+8);

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
	return n;
}

int
sysseek(long sp)
{
	int fd, n;
	ulong off, mode;

	fd = getmem_w(sp);
	off = getmem_w(sp+4);
	mode = getmem_w(sp+8);
	if(sysdbg)
		itrace("seek(%d, %lud, %d)", fd, off, mode);

	n = seek(fd, off, mode);
	if(n < 0)
		errstr(errbuf);	

	return n;
}

int
syssleep(long sp)
{
	ulong len;
	int n;

	len = getmem_w(sp);
	if(sysdbg)
		itrace("sleep(%d)", len);

	n = sleep(len);
	if(n < 0)
		errstr(errbuf);	

	return n;
}

int
sysstat(long sp)
{
	char nambuf[1024];
	char buf[DIRLEN];
	ulong edir, name;
	int n;

	name = getmem_w(sp);
	edir = getmem_w(sp+4);
	memio(nambuf, name, sizeof(nambuf), MemReadstring);
	if(sysdbg)
		itrace("stat(0x%lux='%s', 0x%lux)", name, nambuf, edir);

	n = stat(nambuf, buf);
	if(n < 0)
		errstr(errbuf);
	else
		memio(buf, edir, DIRLEN, MemWrite);

	return n;
}

int
sysfstat(long sp)
{
	char buf[DIRLEN];
	ulong edir;
	int n, fd;

	fd = getmem_w(sp);
	edir = getmem_w(sp+4);
	if(sysdbg)
		itrace("fstat(%d, 0x%lux)", fd, edir);

	n = fstat(fd, buf);
	if(n < 0)
		errstr(errbuf);
	else
		memio(buf, edir, DIRLEN, MemWrite);

	return n;
}

int
syswrite(long sp)
{
	int fd;
	ulong size, a;
	char *buf;
	int n;

	fd = getmem_w(sp);
	a = getmem_w(sp+4);
	size = getmem_w(sp+8);
	if(sysdbg)
		itrace("write(%d, %lux, %d)", fd, a, size);

	buf = memio(0, a, size, MemRead);
	n = write(fd, buf, size);
	if(n < 0)
		errstr(errbuf);	
	free(buf);

	return n;
}

int
syspipe(long sp)
{
	int n, p[2];
	ulong fd;

	fd = getmem_w(sp);
	if(sysdbg)
		itrace("pipe(%lux)", fd);

	n = pipe(p);
	if(n < 0)
		errstr(errbuf);
	else {
		putmem_w(fd, p[0]);
		putmem_w(fd+4, p[1]);
	}
	return n;
}

int
syscreate(long sp)
{
	char file[1024];
	int n;
	ulong mode, name, perm;

	name = getmem_w(sp);
	mode = getmem_w(sp+4);
	perm = getmem_w(sp+8);
	memio(file, name, sizeof(file), MemReadstring);
	if(sysdbg)
		itrace("create(0x%lux='%s', 0x%lux, 0x%lux)", name, file, mode, perm);
	
	n = create(file, mode, perm);
	if(n < 0)
		errstr(errbuf);

	return n;
}

int
sysbrk_(long sp)
{
	ulong addr;

	addr = getmem_w(sp);
	if(sysdbg)
		itrace("brk_(0x%lux)", addr);

	return membrk(addr);
}

int
sysremove(long sp)
{
	char nambuf[1024];
	ulong name;
	int n;

	name = getmem_w(sp);
	memio(nambuf, name, sizeof(nambuf), MemReadstring);
	if(sysdbg)
		itrace("remove(0x%lux='%s')", name, nambuf);

	n = remove(nambuf);
	if(n < 0)
		errstr(errbuf);
	return n;
}

int
sysnotify(long sp)
{
	nofunc = getmem_w(sp);
	if(sysdbg)
		itrace("notify(0x%lux)\n", nofunc);

	return 0;
}

int
syssegflush(long sp)
{
	USED(sp);
	return 0;
}


int (*systab[])(long)	={
	[SYSR1]		0,
	[ERRSTR]	syserrstr,
	[BIND]		sysbind,
	[CHDIR]		syschdir,
	[CLOSE]		sysclose,
	[DUP]		sysdup,
	[ALARM]		0,
	[EXEC]		0,
	[EXITS]		sysexits,
	[FORK]		0,
	[FORKPGRP]	0,
	[FSTAT]		sysfstat,
	[SEGBRK]	0,
	[MOUNT]		0,
	[OPEN]		sysopen,
	[READ]		sysread,
	[SEEK]		sysseek,
	[SLEEP]		syssleep,
	[STAT]		sysstat,
	[WAIT]		0,
	[WRITE]		syswrite,
	[PIPE]		syspipe,
	[CREATE]	syscreate,
	[RFORK]		0,
	[BRK_]		sysbrk_,
	[REMOVE]	sysremove,
	[WSTAT]		0,
	[FWSTAT]	0,
	[NOTIFY]	sysnotify,
	[NOTED]		0,
	[SEGATTACH]	0,
	[SEGDETACH]	0,
	[SEGFREE]	0,
	[SEGFLUSH]	syssegflush,
	[RENDEZVOUS]	0,
	[UNMOUNT]	0,
};

int
syscall(ulong call, ulong sp)
{
	if(call < 0 || call > UNMOUNT) {
		Bprint(bioout, "Bad system call\n");
		dumpreg();
		longjmp(errjmp, 0);
	}
	if(!systab[call])
		error("No system call %s\n", sysctab[call]);
	return (*systab[call])(sp);
}
