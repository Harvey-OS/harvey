#include <u.h>
#include <libc.h>
#include <ureg.h>
#include <tos.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Elfhdr		Elfhdr;
typedef struct Proghdr	Proghdr;
typedef struct ElfEx	ElfEx;

struct Elfhdr {
	uchar	ident[16];
	ushort	type;
	ushort	machine;
	ulong	version;
	ulong	entry;
	ulong	phoff;
	ulong	shoff;
	ulong	flags;
	ushort	ehsize;
	ushort	phentsize;
	ushort	phnum;
	ushort	shentsize;
	ushort	shnum;
	ushort	shstrndx;
};

struct Proghdr {
	ulong	type;
	ulong	offset;
	ulong	vaddr;
	ulong	paddr;
	ulong	filesz;
	ulong	memsz;
	ulong	flags;
	ulong	align;	
};

struct ElfEx
{
	ulong	ientry;
	ulong	ibase;

	ulong	entry;
	ulong	base;

	ulong	phdr;
	ulong	phnum;
	ulong	phent;
};

static void
padzero(ulong addr)
{
	ulong n;

	if(n = (pagealign(addr) - addr))
		memset((void*)addr, 0, n);
}

enum {
	/* file types */
	ElfTNone = 0,
	ElfTReloc = 1,
	ElfTExec = 2,
	ElfTShared = 3,
	ElfTCore = 4,
	ElfTMax = 5,

	/* machine architectures */
	ElfMNone = 0,
	ElfM32 = 1,
	ElfMSparc = 2,
	ElfM386 = 3,
	ElfM68 = 4,
	ElfM88 = 5,
	ElfM860 = 7,
	ElfMMips = 8,
	ElfMMax = 9,

	/* program segment types */
	ElfPNull = 0,
	ElfPLoad = 1,
	ElfPDynamic = 2,
	ElfPInterp = 3,
	ElfPNote = 4,
	ElfPShlib = 5,
	ElfPPhdr = 6,
	ElfPMax = 7,

	/* program segment flags */
	ElfPFX = 1,
	ElfPFW = 2,
	ElfPFR = 4,
};

static int
loadelf(char *file, ElfEx *ex, int depth)
{
	int fd;
	int i, l;
	int mapprot;
	int mapflags;
	ulong mapbase;
	ulong loadaddr;
	ulong bss;

	Elfhdr hdr;
	Proghdr *phdr;
	char *interpreter;

	interpreter = nil;
	phdr = nil;

	if((fd = sys_open(file, O_RDONLY, 0)) < 0){
		werrstr("cant open %s", file);
		goto errout;
	}

	if(sys_read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)){
		werrstr("cant read elf header");
		goto errout;
	}

	if(memcmp(hdr.ident, "\x7fELF", 4)!=0){
		werrstr("no elf magic");
		goto errout;
	}

	l = hdr.phnum * hdr.phentsize;
	phdr = kmalloc(l);
	sys_lseek(fd, hdr.phoff, 0);
	if(sys_read(fd, phdr, l) != l){
		werrstr("cant read program headers");
		goto errout;
	}

	loadaddr = 0;
	mapbase = 0;
	mapflags = MAP_PRIVATE;
	if(hdr.type != ElfTShared)
		mapflags |= MAP_FIXED;

	trace("loadelf(): phnum=%d", hdr.phnum);

	bss = 0;
	for(i=0; i<hdr.phnum; i++){
		Proghdr *p;

		p = &phdr[i];
		if(p->type == ElfPInterp){
			if(interpreter){
				werrstr("multiple interpeter sections");
				goto errout;
			}
			l = p->filesz;

			interpreter = kmalloc(l+1);
			sys_lseek(fd, p->offset, 0);
			if(sys_read(fd, interpreter, l)!=l){
				werrstr("cant read interpreter section");
				goto errout;
			}
			interpreter[l] = '\0';
		}

		if(p->type == ElfPLoad){
			ulong a;
			int diff;

			trace("loadelf(): phdr %d: vaddr=%lux memsz=%lux filesz=%lux offset=%lux flags=%lux",
				i,
				p->vaddr,
				p->memsz,
				p->filesz,
				p->offset,
				p->flags);

			mapprot = 0;
			if(p->flags & ElfPFR)
				mapprot |= PROT_READ;
			if(p->flags & ElfPFW)
				mapprot |= PROT_WRITE;
			if(p->flags & ElfPFX)
				mapprot |= PROT_EXEC;

			if(hdr.entry >= p->vaddr && hdr.entry < p->vaddr + p->memsz)
				mapprot |= PROT_EXEC;

			diff = p->vaddr - (p->vaddr & ~(PAGESIZE-1));

			/* have to call mapdata() before we do the first mmap */
			if(loadaddr == 0 && depth == 0){
				if(hdr.type == ElfTShared){
					mapbase = pagealign((ulong)end + 0x4000000);
					mapflags |= MAP_FIXED;
				}
				mapdata((mapbase + p->vaddr) - diff);
			}

			a = sys_mmap(
				(mapbase + p->vaddr) - diff, 
				p->filesz + diff,
				mapprot,
				mapflags,
				fd,
				(p->offset - diff)/PAGESIZE);

			if(((int)a < 0) && ((int)a > -EMAX)){
				werrstr("mmap failed: %E", (int)a);
				goto errout;
			}
			if(loadaddr == 0)
				loadaddr = a;
			if(hdr.type == ElfTShared && mapbase == 0){
				mapbase = a + diff;
				mapflags |= MAP_FIXED;
			}
			if(mapprot & PROT_WRITE)
				padzero(mapbase + p->vaddr + p->filesz);
			if(depth == 0)
				if(mapbase + p->vaddr + p->memsz > bss)
					bss = mapbase + p->vaddr + p->memsz;
		} else {
			trace("loadelf(): phdr %d: type=%lux", i, p->type);
		}
	}

	ex->base = loadaddr;
	ex->entry = hdr.entry + ((hdr.type == ElfTShared) ? loadaddr : 0);

	ex->phdr = loadaddr + hdr.phoff;
	ex->phent = hdr.phentsize;
	ex->phnum = hdr.phnum;

	if(depth == 0){
		sys_brk(pagealign(bss));

		current->codestart = loadaddr;
		current->codeend = bss;
	}

	if(interpreter){
		ElfEx interpex;

		if(loadelf(interpreter, &interpex, depth+1) < 0){
			werrstr("cant load interpreter: %r");
			goto errout;
		}
		free(interpreter);

		ex->ientry = interpex.entry;
		ex->ibase = interpex.base;
	} else {
		ex->ientry = ex->entry;
		ex->ibase = ex->base;
	}

	sys_close(fd);
	free(phdr);
	return 0;

errout:
	if(fd >= 0)
		sys_close(fd);
	free(interpreter);
	free(phdr);
	return -1;
}


enum {
	AT_NULL,
	AT_IGNORE,
	AT_EXECFD,
	AT_PHDR,
	AT_PHENT,
	AT_PHNUM,
	AT_PAGESZ,
	AT_BASE,
	AT_FLAGS,
	AT_ENTRY,
	AT_NOTELF,
	AT_UID,
	AT_EUID,
	AT_GID,
	AT_EGID,
	AT_PLATFORM,
	AT_HWCAP,
	AT_CLKTCK,
	AT_SECURE = 23,

	AT_SYSINFO = 32,
	AT_SYSINFO_EHDR = 33,
};

static void*
setupstack(ElfEx *ex, char *argv[], char *envp[])
{
	int envc;
	int argc;

	char **dargv;
	char **denv;

	ulong *stack;
	ulong *p;
	char *x;
	int i, n;

	/*
	 * calculate the size we need on stack
	 */
	argc=0;
	while(argv && argv[argc]) argc++;

	envc=0;
	while(envp && envp[envc]) envc++;

	n = 0;
	n += sizeof(ulong);			// argc
	n += (argc+1)*sizeof(char*);	// argv + nil
	n += (envc+1)*sizeof(char*);	// envp + nil
	n += 16*(2*sizeof(ulong));	// aux

	for(i=0; i<argc; i++)
		n += (strlen(argv[i])+1);
	for(i=0; i<envc; i++)
		n += (strlen(envp[i])+1);

	if(USTACK - n < PAGESIZE){
		werrstr("too many arguments passed on stack");
		return nil;
	}
		
	stack = mapstack(USTACK);

	if(((int)stack < 0) && ((int)stack > -EMAX)){
		werrstr("mapstack failed: %E", (int)stack);
		return nil;
	}
	stack = (ulong*)(((ulong)stack - n) & ~7);

	current->stackstart = (ulong)stack;

	p = stack;

	*p++ = argc;

	dargv = (char**)p;
	p += (argc + 1);

	denv = (char**)p;
	p += (envc + 1);

#define AUXENT(k, v)  {p[0]=k; p[1]=v; p+=2;}
	AUXENT(AT_PAGESZ, PAGESIZE);
	AUXENT(AT_CLKTCK, HZ);
	AUXENT(AT_PHDR, ex->phdr);
	AUXENT(AT_PHENT, ex->phent);
	AUXENT(AT_PHNUM, ex->phnum);
	AUXENT(AT_BASE, ex->ibase);
	AUXENT(AT_FLAGS, 0);
	AUXENT(AT_ENTRY, ex->entry);
	AUXENT(AT_UID, current->uid);
	AUXENT(AT_EUID, current->uid);
	AUXENT(AT_GID, current->gid);
	AUXENT(AT_EGID, current->gid);
	AUXENT(AT_NULL, 0);
	AUXENT(AT_NULL, 0);
	AUXENT(AT_NULL, 0);
	AUXENT(AT_NULL, 0);
#undef AUXENT

	x = (char*)p;

	for(i=0; i<argc; i++)
		x += (strlen(dargv[i] = strcpy(x, argv[i])) + 1);
	dargv[argc] = 0;
	for(i=0; i<envc; i++)
		x += (strlen(denv[i] = strcpy(x, envp[i])) + 1);
	denv[envc] = 0;

	return stack;
}

static char**
copystrings(char *a[])
{
	char **r;
	char *p;
	int i, n;

	if(a == nil)
		return nil;
	i = 0;
	n = sizeof(a[0]);
	while(a[i]){
		n += sizeof(a[0]) + (strlen(a[i]) + 1);
		i++;
	}
	r = kmalloc(n);
	n = i;
	p = (char*)&r[n+1];
	for(i=0; i<n; i++)
		p += strlen(r[i] = strcpy(p, a[i]))+1;
	r[n] = 0;
	return r;
}

static void
setcomm(char *exe, char *name, char *argv[])
{
	char *buf, *p;
	int i, n;

	n = strlen(exe) + strlen(name) +2;
	for(i=0; argv[i]; i++)
		n += strlen(argv[i])+1;

	buf = kmalloc(n);

	p = buf;
	p += strlen(strcpy(p, name));
	for(i=0; argv[i]; i++){
		p += strlen(strcpy(p, " "));
		p += strlen(strcpy(p, argv[i]));
	}
	setprocname(buf);

	/* comm contains the full exe name + argv */
	p = buf;
	p += strlen(strcpy(p, exe));
	*p++ = 0;
	for(i=0; argv[i]; i++){
		p += strlen(strcpy(p, argv[i]));
		*p++ = 0;
	}
	*p++ = 0;

	free(current->comm);
	current->comm = buf;
	current->ncomm = p - buf;
}

struct kexecveargs
{
	char		*name;
	char		**argv;
	char		**envp;
};

#pragma profile off

static int
kexecve(void *arg)
{
	struct kexecveargs *args;
	Ufile *f;
	ElfEx ex;
	Ureg u;
	int r, n;
	char *b, *p, *e, *x, **a;
	void *stack;
	char *name, *exe;
	char **argv;
	char **envp;
	int phase;

	args =  arg;
	name = args->name;
	argv = args->argv;
	envp = args->envp;

	phase = 0;
	n = 8192;
	b = kmalloc(n);
	p = b;
	e = b + n;
again:
	if(r = sys_access(name, 05)){
		if(r > 0)
			r = -EACCES;
		goto errout;
	}
	if((r = sys_open(name, O_RDONLY, 0)) < 0)
		goto errout;
	exe = "/dev/null";
	if(f = fdgetfile(r)){
		if(f->path != nil){
			strncpy(p, f->path, e-p);
			p += strlen(exe = p)+1;
		}
		putfile(f);
	}
	n = sys_read(r, p, (e-p)-1);
	sys_close(r);

	r = -ENOEXEC;
	if(n < 4)
		goto errout;

	if(memcmp(p, "#!", 2) == 0){
		p[n] = 0;

		r = -ENAMETOOLONG;
		if((x = strchr(p, '\n')) == nil)
			goto errout;
		*x = 0;

		a = (char**)&x[1];
		n = (e - (char*)a) / sizeof(a[0]);
		if(n < 2)
			goto errout;
		n = getfields(&p[2], a, n, 1, "\t\r\n ");
		if(n < 1)
			goto errout;
		r = -E2BIG;
		if(&a[n+1] >= (char**)e)
			goto errout;
		a[n++] = name; 
		if(argv != nil){
			argv++;
			while(*argv){
				if(&a[n+1] >= (char**)e)
					goto errout;
				a[n++] = *argv++;
			}
		}
		a[n++] = 0;
		p = (char*)&a[n];
		if(e - p < 4)
			goto errout;
		argv = a;
		name = argv[0];

		goto again;
	}

	if(memcmp(p, "\x7fELF", 4)!=0)
		goto errout;

	/*
	 * the contents on envp[] or argv[] maybe stored in b[], stack or bss of the calling linux
	 * process that is destroyed on free(b) and exitmem()... so we need to temporary
	 * copy them.
	 */
	r = -ENOMEM;
	name = kstrdup(name);
	phase++;
	if(argv)
		argv = copystrings(argv);
	phase++;
	if(envp)
		envp = copystrings(envp);
	phase++;

	/* get out of the note before we destroy user stack */
	if(current->innote){
		clinote(current->ureg);
		current->innote = 0;
	}

	/* this is the point of no return! */
	qlock(&proctab);
	zapthreads();
	exitmem();
	exitsignal();

	initmem();
	initsignal();
	inittls();
	qunlock(&proctab);

	closexfds();

	setcomm(exe, name, argv);

	if(loadelf(name, &ex, 0) < 0){
		trace("kexecve(): loadelf failed: %r");
		goto errout;
	}
	if((stack = setupstack(&ex, argv, envp)) == nil){
		trace("kexecve(): setupstack failed: %r");
		goto errout;
	}

	memset(&u, 0, sizeof(u));
	u.sp = (ulong)stack;
	u.ip = (ulong)ex.ientry;
	current->ureg = &u;
	current->syscall = nil;
	phase++;

	trace("kexecve(): startup pc=%lux sp=%lux", current->ureg->ip, current->ureg->sp);

errout:
	switch(phase){
	default:	free(envp);
	case 2:	free(argv);
	case 1:	free(name);
	case 0:	free(b);
	}
	switch(phase){
	case 4:	retuser();
	case 3:	exitproc(current, SIGKILL, 1);
	}
	return r;
}

int sys_execve(char *name, char *argv[], char *envp[])
{
	struct kexecveargs args;

	trace("sys_execve(%s, %p, %p)", name, argv, envp);

	args.name = name;
	args.argv = argv;
	args.envp = envp;

	return onstack(kstack, kexecve, &args);
}

#pragma profile on
