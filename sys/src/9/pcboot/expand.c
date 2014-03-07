/*
 * expand gzipped boot loader appended to this binary and execute it.
 *
 * due to Russ Cox, rsc@swtch.com.
 * see http://plan9.bell-labs.com/wiki/plan9/Replacing_9load
 */
#include <u.h>
#include <libc.h>
#include <a.out.h>
#include <flate.h>
#include "mem.h"
#include "expand.h"

#include "inflate.guts.c"

#define KB		1024
#define MB		(1024*1024)

extern char edata[];

/* ldecomp.s */
void mb586(void);
void splhi(void);
void wbinvd(void);

/* inflate.guts.c */
int gunzip(uchar*, int, uchar*, int);

int isexec(void*);
int isgzip(uchar*);
void run(void*);

#pragma varargck type "d" ulong
#pragma varargck type "x" ulong

static uchar *kernel = (uchar*)Bootkernaddr;
static char *dbrk = (char*)Mallocbase;

ulong
swap(ulong p)
{
	return p<<24 | p>>24 | (p<<8)&0x00FF0000 | (p>>8)&0x0000FF00;
}

enum {
	/* keyboard controller ports & cmds */
	Data=		0x60,		/* data port */
	Status=		0x64,		/* status port */
	 Inready=	0x01,		/*  input character ready */
	 Outbusy=	0x02,		/*  output busy */
	Cmd=		0x64,		/* command port (write only) */

	/* system control port a */
	Sysctla=	0x92,
	 Sysctlreset=	1<<0,
	 Sysctla20ena=	1<<1,
};

static int
isa20on(void)
{
	int r;
	ulong o;
	ulong *zp, *mb1p;

	zp = 0;
	mb1p = (ulong *)MB;
	o = *zp;

	*zp = 0x1234;
	*mb1p = 0x8765;
	mb586();
	wbinvd();
	r = *zp != *mb1p;

	*zp = o;
	return r;
}

static void
delay(uint ms)				/* approximate */
{
	int i;

	while(ms-- > 0)
		for(i = 1000*1000; i > 0; i--)
			;
}

static int
kbdinit(void)
{
	int c, try;

	/* wait for a quiescent controller */
	try = 500;			/* was 1000 */
	while(try-- > 0 && (c = inb(Status)) & (Outbusy | Inready)) {
		if(c & Inready)
			inb(Data);
		delay(1);
	}
	return try <= 0? -1: 0;
}

/*
 *  wait for output no longer busy (but not forever,
 *  there might not be a keyboard controller).
 */
static void
outready(void)
{
	int i;

	for (i = 1000; i > 0 && inb(Status) & Outbusy; i--)
		delay(1);
}

/*
 *  ask 8042 to enable the use of address bit 20
 */
int
i8042a20(void)
{
	if (kbdinit() < 0)
		return -1;
	outready();
	outb(Cmd, 0xD1);
	outready();
	outb(Data, 0xDF);
	outready();
	return 0;
}

void
a20init(void)
{
	int b;

	if (isa20on())
		return;
	if (i8042a20() < 0) {		/* original method, via kbd ctlr */
		/* newer method, last resort */
		b = inb(Sysctla);
		if (!(b & Sysctla20ena))
			outb(Sysctla, (b & ~Sysctlreset) | Sysctla20ena);
	}
	if (!isa20on()){
		print("a20 didn't come on!\n");
		for(;;)
			;
	}
}

void
_main(void)
{
	int ksize;
	Exec *exec;

	splhi();
	a20init();		/* don't wrap addresses at 1MB boundaries */
	ksize = Lowmemsz - (ulong)edata;	/* payload size */
	memmove(kernel, edata, ksize);
	memset(edata, 0, end - edata);

	cgainit();
	if(isgzip(kernel)) {
		print("gz...");
		memmove((uchar*)Unzipbuf, kernel, ksize);

		/* we have to uncompress the entire kernel to get OK status */
		if(gunzip(kernel, Bootkernmax, (uchar*)Unzipbuf, ksize) < 0){
			print("gzip failed.");
			exits(0);
		}
	}
	if(isexec(kernel))
		run(kernel);

	exec = (Exec *)kernel;
	print("unrecognized program; magic # 0x%x\n", swap(exec->magic));
	exits(0);
}

int
isexec(void *v)
{
	Exec *exec;

	exec = v;
	return swap(exec->magic) == I_MAGIC || swap(exec->magic) == S_MAGIC;
}

void
run(void *v)
{
	ulong entry, text, data;
	uchar *base;
	Exec *exec;

	base = v;
	exec = v;
	entry = swap(exec->entry) & ~KSEGM;
	text = swap(exec->text);
	data = swap(exec->data);
	/*
	 * align data segment on the expected page boundary.
	 * sizeof(Exec)+text is offset from base to data segment.
	 */
	memmove(base+PGROUND(sizeof(Exec)+text), base+sizeof(Exec)+text, data);

	print("starting protected-mode loader at 0x%x\n", entry);
	((void(*)(void))entry)();

	print("exec failed");
	exits(0);
}

int
isgzip(uchar *p)
{
	return p[0] == 0x1F && p[1] == 0x8B;
}

void*
malloc(ulong n)
{
	void *v;

	v = dbrk;
	dbrk += ROUND(n, BY2WD);
	return v;
}

void
free(void*)
{
}

void
puts(char *s)
{
	for(; *s; s++)
		cgaputc(*s);
}

int
print(char *fmt, ...)
{
	int sign;
	long d;
	ulong x;
	char *p, *s, buf[20];
	va_list arg;
	static char *hex = "0123456789abcdef";

	va_start(arg, fmt);
	for(p = fmt; *p; p++){
		if(*p != '%') {
			cgaputc(*p);
			continue;
		}
		SET(s);
		switch(*++p){
		case 'p':
		case 'x':
			x = va_arg(arg, ulong);
			if(x == 0){
				s = "0";
				break;
			}
			s = buf+sizeof buf;
			*--s = 0;
			while(x > 0){
				*--s = hex[x&15];
				x /= 16;
			}
			if(s == buf+sizeof buf)
				*--s = '0';
			break;
		case 'd':
			d = va_arg(arg, ulong);
			if(d == 0){
				s = "0";
				break;
			}
			if(d < 0){
				d = -d;
				sign = -1;
			}else
				sign = 1;
			s = buf+sizeof buf;
			*--s = 0;
			while(d > 0){
				*--s = (d%10)+'0';
				d /= 10;
			}
			if(sign < 0)
				*--s = '-';
			break;
		case 's':
			s = va_arg(arg, char*);
			break;
		case 0:
			return 0;
		}
		puts(s);
	}
	return 0;
}

void
exits(char*)
{
	for(;;)
		;
}
