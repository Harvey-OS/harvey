/*
 * Bootstrap loader kernel or normal kernel decompressor.
 * Expand compressed kernel appended to this binary and execute it.
 * currently implements lunzip decompression; dropped gunzip.
 *
 * due to Russ Cox, rsc@swtch.com,
 * http://plan9.bell-labs.com/wiki/plan9/Replacing_9load
 */
#include <u.h>
#include <libc.h>
#include <a.out.h>
#include "mem.h"

#ifndef EXPAND
#define EXPAND
#endif

/* normally defined in ../port/portdat.h */
#define HOWMANY(x, y)	(((x)+((y)-1)) / (y))
#define MASK(w)		((1ul<<(w)) - 1)
#define PGROUND(s)	ROUNDUP(s, BY2PG)
#define ROUND(s, sz)	(((s) + ((sz)-1)) & ~((sz)-1))
#define ROUNDUP(x, y)	(HOWMANY((x), (y)) * (y))	/* ceiling */

#pragma varargck type "d" ulong
#pragma varargck type "x" ulong

enum {
	Neednotexpand, Mustexpand,
	Prbda	= 0,		/* flag: print low-memory layout */

	/* normal kernel decompresses to MB; boot kernel decompressed to 9*MB */
	Lowmemsz	= 640*KB,	/* includes ebda, however */
	Bootkernmax	= 4*MB,		/* max size of boot kernel */
	/* compressed normal kernel starts here, up to 15*MB */
	Unzipbuf	= 13*MB,
};

extern char edata[];

uintptr	finde820map(void);
int	lunzip(uchar*, int, uchar*, int);	/* lunzip.guts.c */
void	mkmultiboot(void);

/* expand.l.s */
int inb(int);
void outb(int, int);
void mb586(void);
void splhi(void);
void wbinvd(void);

static char *dbrk = (char*)Mallocbase;

void
_exits(char*)
{
	for(;;)
		;
}

void
panic(char *s, ...)
{
	print("panic: expand: %s\n", s);
	_exits("panic");
}

void*
malloc(ulong n)
{
	void *v;

	v = dbrk;
	dbrk += ROUND(n, BY2WD);
	return v;
}

void*
realloc(void *, ulong)
{
	panic("no realloc");
	return nil;
}

void
free(void*)
{
}

/* optional prbda chatter for information gathering */
void
prbda(void)
{
	int ebda, fbm;

	if (!Prbda)
		return;
	ebda = *(ushort *)(EBDAADDR-KZERO) << 4;
	fbm = *(ushort *)(FBMADDR-KZERO);	/* misaligned short */
	print("ebda 0x%x = %d, free base mem = %dK...", ebda, ebda, fbm);
}

ulong
swap(ulong p)
{
	return p<<24 | p>>24 | (p<<8)&0x00FF0000 | (p>>8)&0x0000FF00;
}

#include "lunzip.guts.c"
#include "a20.c"
#include "cga.c"
#include "print.c"
#include "../pc/multiboot.c"
#include "../pc/e820.c"

static void
badmagic(Exec *exec, char *type)
{
	panic("unknown %s kernel magic # 0x%x", type, swap(exec->magic));
}

/*
 * figure out type of compression in compkern and uncompress it to kernel.
 */
void
uncompress(uchar *kernel, uchar *compkern, ulong ksize, int must)
{
	if (compkern[0] == 0x1F && compkern[1] == 0x8B)
		panic("gzipped payloads unsupported");
	if(memcmp(compkern, "LZIP", 4) != 0)
		badmagic((Exec *)compkern, "comp'd");	/* no return */
	if(lunzip(kernel, Bootkernmax, compkern, ksize) < 0 && must)
		panic("lunzip failed");
}

int
isexec(void *v)
{
	Exec *exec;

	exec = v;
	return swap(exec->magic) == I_MAGIC || swap(exec->magic) == S_MAGIC;
}

void enternextkernel(ulong);

/* 386 entry address is past a.out header; amd64 is not. */
void
run(void *v)
{
	ulong entry, text, data, hdrtxtsz, magic;
	uchar *base;
	Exec *exec;

	base = v;
	exec = v;
	magic = swap(exec->magic);
	entry = swap(exec->entry) & ~KSEGM;
	text = swap(exec->text);
	data = swap(exec->data);
	hdrtxtsz = sizeof(Exec) + text;
	if (magic & HDR_MAGIC)
		hdrtxtsz += sizeof(vlong);
	/*
	 * align data segment on the expected page boundary by copying it up.
	 * hdrtxtsz is usual offset from base to data segment.
	 */
	memmove(base + PGROUND(hdrtxtsz), base + hdrtxtsz, data);

	/* amd64: move text down over a.out header to entry address */
	if (magic & HDR_MAGIC)
		memmove((char *)entry, base + sizeof(Exec) + sizeof(vlong), text);

	/* we were loaded by pxe or pbs, so there is no config yet */
	memset(BOOTARGS, 0, BOOTARGSLEN);

	print("starting protected-mode kernel at 0x%x: 0x%x.  mbi at 0x%x\n",
		entry, *(ulong *)entry, (ulong)multibootheader);
	enternextkernel(entry);			/* no return */
}

/* calling it _main avoids dragging in the atexit machinery */
void
_main(void)
{
	int ksize;
	ulong loadat;
	uintptr e820;
	uchar *kernel;
	Exec *exec;

	/*
	 * we can't touch data in odd megabytes until the A20 line is on,
	 * and we mustn't touch BSS until the compressed kernel
	 * is moved out of the way and our BSS is zeroed.
	 */
	splhi();
	/* don't wrap addresses at 1MB boundaries */
	if (a20init() < 0){
		cgainit();
		panic("a20 didn't come on!");
	}

	ksize = Lowmemsz - (ulong)edata;	/* payload size */
	/* copy compressed kernel well above 1MB */
	memmove((uchar *)Unzipbuf, edata, ksize);
	memset(edata, 0, end - edata);		/* zero my bss */

	/*
	 * we're now in a normal C environment.
	 */
	cgainit();
	prbda();

	if ((e820 = finde820map()) != 0)
		readlsconf(e820);	/* pick up memory map from bios.s */

	print("lz...");
	/* decompress at least exec header to get load address */
	memset((uchar *)MB, 0, 512);
	uncompress((uchar *)MB, (uchar *)Unzipbuf, 512, Neednotexpand);
	exec = (Exec *)MB;
	loadat = swap(exec->entry) & ~(KSEGM|MASK(PGSHIFT));
	kernel = (uchar *)loadat;
	if (loadat < MB)
		panic("insane entry 0x%x -> load 0x%p", exec->entry, kernel);
	print("uncompressing to 0x%p...", kernel);
	delay(50);
	uncompress(kernel, (uchar *)Unzipbuf, ksize, Mustexpand);
	print("\n");

	/* iff we recognise its magic number, jump to decompressed kernel */
	mkmultiboot();
	if(isexec(kernel))
		run(kernel);		/* no return */
	badmagic((Exec *)kernel, "");	/* no return */
}
