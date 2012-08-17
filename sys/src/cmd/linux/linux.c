/* derived from the hare machcnk program, heavily nodified.
 * This is a "shepherd". It lives in low user memory, code at 2M, data at 4M. 
 * It was inspired by the lguest program, which works in a similar way,
 * although lguest lives in very high memory.
 * Linux programs must be linked to run starting at 6M at least.
 * Stack is shared.
 * The command for linking programs on linux is:
 * cc -static -Xlinker -Ttext-segment=0x800000 code.c
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include <ureg.h>
#include "elf.h"
#include "dat.h"
#include "fns.h"
#include "linux.h"

enum {
	DumpCall = 16,		/* dump args and return (including errors) */
	DumpUregsBefore = 32,	/* dump uregs before system call */
	DumpUregsAfter = 64,	/* dump uregs after system call */
	PersyscallInfo = 128,	/* print per-system-call info */
};

enum {
	StackKB = 1024,
	KiB	= 1024,
};

#define RND(size,up) (((size)+(up)-1) & ~((up)-1))
#define RNDM(size) ((size+0xfffff)&0xfff00000)

int debug = 0;

int
verbose(void){
	return 1;
}
void errexit(char *s)
{
	fprint(2, smprint("%s\n", s));
	exits("life sucks");
}
/* aux vector structure. Can NOT be null.  */
typedef struct
{
  u64int a_type;              /* Entry type */
  union
    {
      u64int a_val;           /* Integer value */
      /* We used to have pointer elements added here.  We cannot do that,
         though, since it does not work when using 64-bit definitions
         on 32-bit platforms and vice versa.  */
    } a_un;
} Elf64_auxv_t;


/*
 *	All a.out header types.  The dummy entry allows canonical
 *	processing of the union as a sequence of longs
 */

typedef struct {
	union{
		Ehdr;			/* elf.h */
	} e;
	long dummy;			/* padding to ensure extra long */
} ExecHdr;

extern void *exechdr(void);
extern void *phdr(void);
/* we need an array of these. Cheese it out and just make 32 of them. 
 * they will start out empty (zero'd) and we'll just add what we need. 
 */
int auxc = 0;
Elf64_auxv_t aux[32];

/* oops! */
#define AT_BASE_PLATFORM 24		/* String identifying real platforms.*/
#define AT_RANDOM       25              /* Address of 16 random bytes.  */
#define AT_EXECFN       31              /* Filename of executable.  */


char *names[] = {
[AT_NULL] "AT_NULL",
[AT_IGNORE] "AT_IGNORE",
[AT_EXECFD] "AT_EXECFD",
[AT_PHDR] "AT_PHDR",
[AT_PHENT] "AT_PHENT",
[AT_PHNUM] "AT_PHNUM",
[AT_PAGESZ] "AT_PAGESZ",
[AT_BASE] "AT_BASE",
[AT_FLAGS] "AT_FLAGS",
[AT_ENTRY] "AT_ENTRY",
[AT_NOTELF] "AT_NOTELF",
[AT_UID] "AT_UID",
[AT_EUID] "AT_EUID",
[AT_GID] "AT_GID",
[AT_EGID] "AT_EGID",
[AT_CLKTCK] "AT_CLKTCK",
[AT_PLATFORM] "AT_PLATFORM",
[AT_HWCAP] "AT_HWCAP",
[AT_FPUCW] "AT_FPUCW",
[AT_DCACHEBSIZE] "AT_DCACHEBSIZE",
[AT_ICACHEBSIZE] "AT_ICACHEBSIZE",
[AT_UCACHEBSIZE] "AT_UCACHEBSIZE",
[AT_IGNOREPPC] "AT_IGNOREPPC",
[AT_BASE_PLATFORM] "AT_BASE_PLATFORM",
[AT_RANDOM] "AT_RANDOM",
[AT_EXECFN] "AT_EXECFN",
[AT_SYSINFO] "AT_SYSINFO",
[AT_SYSINFO_EHDR] "AT_SYSINFO_EHDR",
[AT_L1I_CACHESHAPE] "AT_L1I_CACHESHAPE",
[AT_L1D_CACHESHAPE] "AT_L1D_CACHESHAPE",
[AT_L2_CACHESHAPE] "AT_L2_CACHESHAPE",
[AT_L3_CACHESHAPE] "AT_L3_CACHESHAPE",
};


void naux(int type, u64int val) 
{
	if (debug)
		fprint(2,"NAUX: Set %d to %s/%#x\n", auxc, names[type], val);
	aux[auxc].a_type = type;
	aux[auxc].a_un.a_val = val;
	auxc++;
}

/*
 * Elf64 binaries.
 * Really, libmach is too limited. It can not handle 
 * more special stuff like more than 2 loadable segments,
 * TLS, etc. We're going to have to replace it with 
 * something else.
 */
Phdr64 *
hdr(int fd)
{
	Ehdr64 *ep;
	Phdr64 *ph;
	int i;
	static char random[16];

	ep = exechdr();
	ph = phdr();
	if (debug){
		fprint(2,"elf add %d entries at %p\n", ep->phnum, ph);
		for(i = 0; i < ep->phnum; i++) {
			fprint(2,"%d: type %#x va %p pa %p \n", i, ph[i].type, ph[i].vaddr, ph[i].paddr);
		}
	}
	/* GNU ELF is weird.
	 * GNUSTACK is the last phdr and it's confusing libmach.
	 */
	naux(AT_PHNUM, ep->phnum-1);
	naux(AT_PHDR, (u64int)ph);
	naux(AT_RANDOM, (u64int)random);

	return ph;
}

void
usage(void)
{
	errexit(smprint("usage: linux [-b] [breakpoint] [-p p==v base] [a]rgstosyscall [s]yscall [u]regsbefore [U]uregsafter [f]aultpower [F]aultpowerregs elf\n"));
}

void
handler(void *v, char *s)
{
        char *f[7];
	u64int parm[7];
	struct Ureg* u = v;
        int i, n, nf;
	char *cp;
#undef DEBUGHANDLER
#ifdef DEBUGHANDLER
	write(2, "HANDLER:", 8);
	write(2, s, strlen(s));
#endif
	if (strncmp(s, "linux:", 6))
		calllinuxnoted(NDFLT);
	s += 6;
	nf = tokenize(s, f, nelem(f));

	for(i = 0; i < nelem(parm); i++)
		parm[i] = strtoull(f[i], 0, 0);
	/* TODO soon: use linuxsystab.h */
	switch(parm[0]){
	case 4:
		u->ax = linuxstat((char*)(parm[1]), (Linuxstat*)(parm[2]));
		break;
	case 21:
		u->ax = access((char*)(parm[1]), (int)(parm[2]));
		break;
	case 22:
		u->ax = pipe((void*)(parm[1]));
		break;
	case 72:
		u->ax = linuxfcntl((int)(parm[1]), (int)(parm[2]), (void*)(parm[3]));
		break;
	case 79:
		u->ax = -ENOENT;
		cp = getwd((void *)(parm[1]), (int)(parm[2]));
		if (cp)
			u->ax = (uintptr) strlen(cp) + 1;
		/* the linux man page is bullshit. */
	case 80:
		u->ax = chdir((char*)(parm[1]));
		break;
	case 97:
		u->ax = sys_getrlimit((long)(parm[1]), (void *)parm[2]);
		break;
	case 160:
		u->ax = sys_setrlimit((long)(parm[1]), (void *)parm[2]);
		break;
	case 218:
		u->ax = sys_set_tid_address((int*)(parm[1]));
		break;
	case 273:
		u->ax = 0;
		break;
	default:
		fprint(2, "linux: unhandled syscall %d\n", parm[0]);
		break;
	}
	calllinuxnoted(NCONT);
}

main(int argc, char *argv[])
{
	/* keep our stack on the main stack. Avoids lots of
	 * validaddr calls
	 */
	unsigned char stack[StackKB*KiB];
	int fd, ctl;
	unsigned char  *textp, *datap, *bssp;
	ulong bsssize, bssbase;
	char *ctlpath;
	void (*f)(int argc, char *argv[]);
	void callmain(void *, void*);
	Fhdr fp;
	Map *map;
	int textseg, dataseg;
	int i;
	u32int breakpoint = 0;
	char **av;
	int debugctl = 2;
	char cmd[2];

	ARGBEGIN{
	case 'b':	breakpoint = strtoul(EARGF(usage()), 0, 0); break;
	/* weird. Symbolic names are no longer in the code? Lossage from the CNK project. */
	case 'f':	debugctl |= 4; break;
	case 'F':	debugctl |= 8; break;
	case 's':	debugctl |= DumpCall; break;
	case 'u':	debugctl |= DumpUregsBefore; break;
	case 'U':	debugctl |= DumpUregsAfter; break;
	case 'a':	debugctl |= PersyscallInfo; break;
	case 'A':	debugctl |= 0xfc; break;
	case 'S': 	bsssize = strtoul(EARGF(usage()), 0, 0); break;
	case 'v':	debug++; break;
	default:	usage(); break;
	}ARGEND

	if (argc < 1)
		errexit("usage: linux filename");

	fd = open(argv[0], OREAD);
	if (fd < 0)
		errexit(smprint("Can't open %s\n", argv[0]));

	if (crackhdr(fd, &fp) < 0)
		errexit("crackhdr failed");

	map = loadmap(nil, fd, &fp);

	if (! map)
		errexit("loadmap failed");

	textseg = findseg(map, "text");
	if (textseg < 0)
		errexit("no text segment");
	dataseg = findseg(map, "data");
	if (dataseg < 0)
		errexit("no data segment");

	naux(AT_DCACHEBSIZE, 0x20);
	naux(AT_PAGESZ, 0x1000);
	naux(AT_UID, 0x17be);
	naux(AT_EUID, 0x17be);
	naux(AT_GID, 0x64);
	naux(AT_EGID, 0x64);
	naux(AT_HWCAP, 0x4);

	if (debug) {
		fprint(2, "textseg is %d and dataseg is %d\n", textseg, dataseg);
		fprint(2, "base %#llx end %#llx off %#llx \n",
			map->seg[textseg].b, map->seg[textseg].e, map->seg[textseg].f);
		fprint(2, "base %#llx end %#llx off %#llx \n",
			map->seg[dataseg].b, map->seg[dataseg].e, map->seg[dataseg].f);
		fprint(2, "txtaddr %#llx dataaddr %#llx entry %#llx txtsz"
			"%#lx datasz %#lx bsssz %#lx\n", 
			fp.txtaddr, fp.dataddr, fp.entry, fp.txtsz, fp.datsz, fp.bsssz);
	}

	bssbase = fp.dataddr + RNDM( fp.datsz + fp.bsssz),

	textp = (void *)fp.txtaddr;
	datap = (void *)fp.dataddr;
	bssp = (void *)bssbase;
	if (debug)
		fprint(2,"bssp is %p\n", bssp);
	//hangpath = smprint("/proc/%d/ctl", getpid());
	hdr(fd); 
	naux(AT_NULL, 0);
	if (notify(handler)){
		fprint(2, "%r\n");
		exits("notify fails");
	}
	uproc.pid = getpid();

	free(malloc(1048576));

	/* NO print's past this point if you want to live. */
	if (brk(bssp + fp.bsssz) < 0){
		exits("no brk");
	}
	/* clear out bss ... */
	memset(bssp, 0, fp.bsssz);

	/* now the big fun. Just copy it out */
	pread(fd, textp, fp.txtsz, fp.txtoff);
	pread(fd, datap, fp.datsz, fp.datoff);
	/* set the breakpoint */
	if (breakpoint){
		*(u8int *)breakpoint = 0xf4; // hlt
	}
	/* DEBUGGING

	if (debug)
		fprint(2, "Open %s\n", hangpath);
	hang = open(hangpath, OWRITE);
	if (hang < 0){
		errexit(smprint("%s: %r", hangpath));
	}
	if (write(hang, "hang", 4) < 4){
		errexit(smprint("write cnk: %r"));
	}

	*/

	/* all this other junk goes in the last KiB */
	av = (char **)&stack[(StackKB-1)*KiB];
	/* gnu is odd. they start out knowing args are on stack (sort of) */
	/* av is going to become the start of the stack. */
	/* this is where argc goes. */
	av[0] = (char *)argc;
	for(i = 0; i < argc; i++){
		av[i+1] = argv[i];
	}
	i++;
	av[i++] = nil;
	av[i++] = "LANG=C";
	av[i++] = nil;
	/* now just copy the aux array over av */
	memcpy(&av[i], aux, sizeof(aux));
#ifdef NOT
	if (debug)
		fprint(2, "Open %s\n", ctlpath);
	ctl = open(ctlpath, OWRITE);
	if (ctl < 0){
		errexit(smprint("%s: %r", ctlpath));
	}
#endif
/*
	sprint(cmd, "%1x", cnk);
	fprint(2, "write cnk cmd (%s)\n", cmd);	
	if (write(ctl, cmd, 2) < 1){
		errexit(smprint("write cnk: %r"));
	}
*/
	f = (void *)fp.entry;
	//fprint(2,"Call entry point %p\n", (void *)f);
	callmain(f, av);
	return 0;

}
