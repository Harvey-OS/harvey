#include <u.h>
#include <libc.h>
#include <auth.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "power.h"


#define	REGSP	1
#define	REGRET	3

#define	ODIRLEN	116	/* compatibility; used in _stat etc. */
#define	OERRLEN	64	/* compatibility; used in _stat etc. */

char 	errbuf[ERRMAX];
ulong	nofunc;

#include "/sys/src/libc/9syscall/sys.h"

char *sysctab[]={
	[SYSR1]		"SYSR1",
	[_ERRSTR]	"_errstr",
	[BIND]		"Bind",
	[CHDIR]		"Chdir",
	[CLOSE]		"Close",
	[DUP]		"Dup",
	[ALARM]		"Alarm",
	[EXEC]		"Exec",
	[EXITS]		"Exits",
	[_FSESSION]	"_Fsession",
	[FAUTH]		"Fauth",
	[_FSTAT]	"_fstat",
	[SEGBRK]	"Segbrk",
	[_MOUNT]	"_Mount",
	[OPEN]		"Open",
	[_READ]		"_Read",
	[OSEEK]		"Oseek",
	[SLEEP]		"Sleep",
	[_STAT]		"_Stat",
	[RFORK]		"Rfork",
	[_WRITE]	"_Write",
	[PIPE]		"Pipe",
	[CREATE]	"Create",
	[FD2PATH]	"Fd2path",
	[BRK_]		"Brk_",
	[REMOVE]	"Remove",
	[_WSTAT]	"_Wstat",
	[_FWSTAT]	"_Fwstat",
	[NOTIFY]	"Notify",
	[NOTED]		"Noted",
	[SEGATTACH]	"Segattach",
	[SEGDETACH]	"Segdetach",
	[SEGFREE]	"Segfree",
	[SEGFLUSH]	"Segflush",
	[RENDEZVOUS]	"Rendezvous",
	[UNMOUNT]	"Unmount",
	[_WAIT]		"Wait",
	[SEEK]		"Seek",
	[FVERSION]	"Fversion",
	[ERRSTR]	"Errstr",
	[STAT]		"Stat",
	[FSTAT]		"Fstat",
	[WSTAT]		"Wstat",
	[FWSTAT]	"Fwstat",
	[MOUNT]		"Mount",
	[AWAIT]		"Await",
	[PREAD]		"Pread",
	[PWRITE]	"Pwrite",
};

void sys1(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0); }

void
sys_errstr(void)
{
	ulong str;
	char tmp[OERRLEN];

	str = getmem_w(reg.r[REGSP]+4);
	if(sysdbg)
		itrace("errstr(0x%lux)", str);

	memio(tmp, str, OERRLEN, MemRead);
	memio(errbuf, str, OERRLEN, MemWrite);
	memmove(errbuf, tmp, OERRLEN);
	errbuf[OERRLEN-1] = 0;
	reg.r[REGRET] = 0;
}

void
syserrstr(void)
{
	ulong str;
	uint n;
	char tmp[ERRMAX];

	str = getmem_w(reg.r[REGSP]+4);
	n = getmem_w(reg.r[REGSP]+8);
	if(sysdbg)
		itrace("errstr(0x%lux, 0x%lux)", str, n);

	if(n > strlen(errbuf)+1)
		n = strlen(errbuf)+1;
	if(n > ERRMAX)
		n = ERRMAX;
	memio(tmp, str, n, MemRead);
	memio(errbuf, str, n, MemWrite);
	memmove(errbuf, tmp, n);
	errbuf[ERRMAX-1] = 0;
	reg.r[REGRET] = n;
	
}

void
sysfd2path(void)
{
	int n;
	uint fd;
	ulong str;
	char buf[1024];

	fd = getmem_w(reg.r[REGSP]+4);
	str = getmem_w(reg.r[REGSP]+8);
	n = getmem_w(reg.r[REGSP]+12);
	if(sysdbg)
		itrace("fd2path(0x%lux, 0x%lux, 0x%lux)", fd, str, n);
	reg.r[REGRET] = -1;
	if(n > sizeof buf){
		strcpy(errbuf, "buffer too big");
		return;
	}
	n = fd2path(fd, buf, sizeof buf);
	if(n < 0)
		errstr(buf, sizeof buf);
	else
		memio(errbuf, str, n, MemWrite);
	reg.r[REGRET] = n;
	
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
		errstr(errbuf, sizeof errbuf);

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
		errstr(errbuf, sizeof errbuf);

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
		errstr(errbuf, sizeof errbuf);
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
		errstr(errbuf, sizeof errbuf);
	reg.r[REGRET] = n;
}

void
sysexits(void)
{
	char buf[ERRMAX];
	ulong str;

	str = getmem_w(reg.r[REGSP]+4);
	if(sysdbg)
		itrace("exits(0x%lux)", str);

	count = 1;
	if(str != 0) {
		memio(buf, str, sizeof buf, MemRead);
		buf[ERRMAX-1] = 0;
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
		errstr(errbuf, sizeof errbuf);

	reg.r[REGRET] = n;
};

void
sysread(vlong offset)
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
		n = pread(fd, buf, size, offset);

	if(n < 0)
		errstr(errbuf, sizeof errbuf);
	else
		memio(buf, a, n, MemWrite);

	if(sysdbg)
		itrace("read(%d, 0x%lux, %d, 0x%llx) = %d", fd, a, size, offset, n);

	free(buf);
	reg.r[REGRET] = n;
}

void
sys_read(void)
{
	sysread(-1LL);
}

void
syspread(void)
{
	union {
		vlong v;
		ulong u[2];
	} o;

	o.u[0] = getmem_w(reg.r[REGSP]+16);
	o.u[1] = getmem_w(reg.r[REGSP]+20);
	sysread(o.v);
}

void
sysseek(void)
{
	int fd;
	ulong mode;
	ulong retp;
	union {
		vlong v;
		ulong u[2];
	} o;

	retp = getmem_w(reg.r[REGSP]+4);
	fd = getmem_w(reg.r[REGSP]+8);
	o.u[0] = getmem_w(reg.r[REGSP]+12);
	o.u[1] = getmem_w(reg.r[REGSP]+16);
	mode = getmem_w(reg.r[REGSP]+20);
	if(sysdbg)
		itrace("seek(%d, %lld, %d)", fd, o.v, mode);

	o.v = seek(fd, o.v, mode);
	if(o.v < 0)
		errstr(errbuf, sizeof errbuf);	

	memio((char*)o.u, retp, sizeof(vlong), MemWrite);
}

void
sysoseek(void)
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
		errstr(errbuf, sizeof errbuf);	

	reg.r[REGRET] = n;
}

void
sysrfork(void)
{
	int flag;

	flag = getmem_w(reg.r[REGSP]+4);
	if(sysdbg)
		itrace("rfork(%d)", flag);
	if(flag & RFPROC) {
		Bprint(bioout, "rfork: cannot create process, rfork(0x%.8ux)\n", flag);
		exits(0);
	}
	reg.r[REGRET] = rfork(flag);
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
		errstr(errbuf, sizeof errbuf);	

	reg.r[REGRET] = n;
}

void
sys_stat(void)
{
	char nambuf[1024];
	char buf[ODIRLEN];
	ulong edir, name;
	extern int _stat(char*, char*);	/* old system call */
	int n;

	name = getmem_w(reg.r[REGSP]+4);
	edir = getmem_w(reg.r[REGSP]+8);
	memio(nambuf, name, sizeof(nambuf), MemReadstring);
	if(sysdbg)
		itrace("stat(0x%lux='%s', 0x%lux)", name, nambuf, edir);

	n = _stat(nambuf, buf);
	if(n < 0)
		errstr(errbuf, sizeof errbuf);
	else
		memio(buf, edir, ODIRLEN, MemWrite);

	reg.r[REGRET] = n;
}

void
sysstat(void)
{
	char nambuf[1024];
	uchar buf[STATMAX];
	ulong edir, name;
	int n;

	name = getmem_w(reg.r[REGSP]+4);
	edir = getmem_w(reg.r[REGSP]+8);
	n = getmem_w(reg.r[REGSP]+12);
	memio(nambuf, name, sizeof(nambuf), MemReadstring);
	if(sysdbg)
		itrace("stat(0x%lux='%s', 0x%lux, 0x%lux)", name, nambuf, edir, n);
	if(n > sizeof buf)
		errstr(errbuf, sizeof errbuf);
	else{	
		n = stat(nambuf, buf, n);
		if(n < 0)
			errstr(errbuf, sizeof errbuf);
		else
			memio((char*)buf, edir, n, MemWrite);
	}
	reg.r[REGRET] = n;
}

void
sys_fstat(void)
{
	char buf[ODIRLEN];
	ulong edir;
	extern int _fstat(int, char*);	/* old system call */
	int n, fd;

	fd = getmem_w(reg.r[REGSP]+4);
	edir = getmem_w(reg.r[REGSP]+8);
	if(sysdbg)
		itrace("fstat(%d, 0x%lux)", fd, edir);

	n = _fstat(fd, buf);
	if(n < 0)
		errstr(errbuf, sizeof errbuf);
	else
		memio(buf, edir, ODIRLEN, MemWrite);

	reg.r[REGRET] = n;
}

void
sysfstat(void)
{
	uchar buf[STATMAX];
	ulong edir;
	int n, fd;

	fd = getmem_w(reg.r[REGSP]+4);
	edir = getmem_w(reg.r[REGSP]+8);
	n = getmem_w(reg.r[REGSP]+12);
	if(sysdbg)
		itrace("fstat(%d, 0x%lux, 0x%lux)", fd, edir, n);

	reg.r[REGRET] = -1;
	if(n > sizeof buf){
		strcpy(errbuf, "stat buffer too big");
		return;
	}
	n = fstat(fd, buf, n);
	if(n < 0)
		errstr(errbuf, sizeof errbuf);
	else
		memio((char*)buf, edir, n, MemWrite);
	reg.r[REGRET] = n;
}

void
syswrite(vlong offset)
{
	int fd;
	ulong size, a;
	char *buf;
	int n;

	fd = getmem_w(reg.r[REGSP]+4);
	a = getmem_w(reg.r[REGSP]+8);
	size = getmem_w(reg.r[REGSP]+12);

	Bflush(bioout);
	buf = memio(0, a, size, MemRead);
	n = pwrite(fd, buf, size, offset);
	if(n < 0)
		errstr(errbuf, sizeof errbuf);	
	if(sysdbg)
		itrace("write(%d, %lux, %d, 0xllx) = %d", fd, a, size, offset, n);
	free(buf);

	reg.r[REGRET] = n;
}

void
sys_write(void)
{
	syswrite(-1LL);
}

void
syspwrite(void)
{
	union {
		vlong v;
		ulong u[2];
	} o;

	o.u[0] = getmem_w(reg.r[REGSP]+16);
	o.u[1] = getmem_w(reg.r[REGSP]+20);
	syswrite(o.v);
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
		errstr(errbuf, sizeof errbuf);
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
		errstr(errbuf, sizeof errbuf);

	reg.r[REGRET] = n;
}

void
sysbrk_(void)
{
	ulong addr, osize, nsize;
	Segment *s;

	addr = getmem_w(reg.r[REGSP]+4);
	if(sysdbg)
		itrace("brk_(0x%lux)", addr);

	reg.r[REGRET] = -1;
	if(addr < memory.seg[Data].base+datasize) {
		strcpy(errbuf, "address below segment");
		return;
	}
	if(addr > memory.seg[Stack].base) {
		strcpy(errbuf, "segment too big");
		return;
	}
	s = &memory.seg[Bss];
	if(addr > s->end) {
		osize = ((s->end-s->base)/BY2PG)*sizeof(uchar*);
		addr = ((addr)+(BY2PG-1))&~(BY2PG-1);
		s->end = addr;
		nsize = ((s->end-s->base)/BY2PG)*sizeof(uchar*);
		s->table = erealloc(s->table, osize, nsize);
	}	

	reg.r[REGRET] = 0;	
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
		errstr(errbuf, sizeof errbuf);
	reg.r[REGRET] = n;
}

void
sysnotify(void)
{
	nofunc = getmem_w(reg.r[REGSP]+4);
	if(sysdbg)
		itrace("notify(0x%lux)", nofunc);

	reg.r[REGRET] = 0;
}

void
syssegflush(void)
{
	ulong start, len;

	start = getmem_w(reg.r[REGSP]+4);
	len = getmem_w(reg.r[REGSP]+8);
	if(sysdbg)
		itrace("segflush(va=0x%lux, n=%lud)", start, len);
	reg.r[REGRET] = 0;
}

void sysfversion(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0); }
void sysfsession(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0); }
void sysfauth(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0); }
void syswait(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0); }
void syswstat(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sys_wstat(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysfwstat(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sys_fwstat(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysnoted(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void syssegattach(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void syssegdetach(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void syssegfree(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysrendezvous(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysunmount(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysfork(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysforkpgrp(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void syssegbrk(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void _sysmount(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysalarm(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysexec(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysmount(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}
void sysawait(void) { Bprint(bioout, "No system call %s\n", sysctab[reg.r[REGRET]]); exits(0);}

void (*systab[])(void)	={
	[SYSR1]		sys1,
	[_ERRSTR]	sys_errstr,
	[BIND]		sysbind,
	[CHDIR]		syschdir,
	[CLOSE]		sysclose,
	[DUP]		sysdup,
	[ALARM]		sysalarm,
	[EXEC]		sysexec,
	[EXITS]		sysexits,
	[_FSESSION]	sysfsession,
	[FAUTH]		sysfauth,
	[_FSTAT]	sys_fstat,
	[SEGBRK]	syssegbrk,
	[_MOUNT]	_sysmount,
	[OPEN]		sysopen,
	[_READ]		sys_read,
	[OSEEK]		sysoseek,
	[SLEEP]		syssleep,
	[_STAT]		sys_stat,
	[RFORK]		sysrfork,
	[_WRITE]	sys_write,
	[PIPE]		syspipe,
	[CREATE]	syscreate,
	[FD2PATH]	sysfd2path,
	[BRK_]		sysbrk_,
	[REMOVE]	sysremove,
	[_WSTAT]	sys_wstat,
	[_FWSTAT]	sys_fwstat,
	[NOTIFY]	sysnotify,
	[NOTED]		sysnoted,
	[SEGATTACH]	syssegattach,
	[SEGDETACH]	syssegdetach,
	[SEGFREE]	syssegfree,
	[SEGFLUSH]	syssegflush,
	[RENDEZVOUS]	sysrendezvous,
	[UNMOUNT]	sysunmount,
	[_WAIT]		syswait,
	[SEEK]		sysseek,
	[FVERSION]	sysfversion,
	[ERRSTR]	syserrstr,
	[STAT]		sysstat,
	[FSTAT]		sysfstat,
	[WSTAT]		syswstat,
	[FWSTAT]	sysfwstat,
	[MOUNT]		sysmount,
	[AWAIT]		sysawait,
	[PREAD]		syspread,
	[PWRITE]	syspwrite,
};

void
sc(ulong inst)
{
	int call;

	if(inst != ((17<<26)|2))
		undef(inst);
	call = reg.r[REGRET];
	if(call < 0 || call > PWRITE || systab[call] == nil) {
		Bprint(bioout, "Bad system call\n");
		dumpreg();
	}
	if(trace)
		itrace("sc\t(%s)", sysctab[call]);

	(*systab[call])();
	Bflush(bioout);
}
