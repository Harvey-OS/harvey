#include	"u.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"lib.h"

enum {
	/* prom operations */
	Promop_getc   = 1,
	Promop_puts   = 2,
	Promop_open   = 0x10,
	Promop_close  = 0x11,
	Promop_read   = 0x13,
	Promop_write  = 0x14,
	Promop_getenv = 0x22,

	/* environment variable indices for getenv */
	/* auto_action might be 1; it looks that way. */
	Promenv_booted_dev	= 4,
	Promenv_booted_file	= 6,
	Promenv_booted_osflags	= 8,
	Promenv_tty_dev		= 0xf,
};

Hwrpb	*hwrpb;

static uvlong	dispatchf;
static ulong	clk2ms;

void
consinit(void)
{
	Procdesc *p;
	Hwcrb *crb;
	Hwdsr *dsr;
	char *s;

	hwrpb = (Hwrpb*)0x10000000;

	crb = (Hwcrb*)((ulong)hwrpb + hwrpb->crboff);
	p = (Procdesc*)(crb->dispatchva);
	dispatchf = p->addr;
	clk2ms = hwrpb->cfreq/1000;

	print("\nAlpha Plan 9 secondary boot\n");
	if (hwrpb->rev >= 6) {
		dsr = (Hwdsr*)((ulong)hwrpb + hwrpb->dsroff);
		s = (char*)dsr + dsr->sysnameoff + 8;
		print("%s\n", s);
	}
}

uvlong
dispatch(uvlong r16, uvlong r17, uvlong r18, uvlong r19, uvlong r20)
{
	return gendispatch(dispatchf, r16, r17, r18, r19, r20);
};

int
devopen(char *s)
{
	vlong ret;
	int n;

	n = strlen(s);
	ret = dispatch(0x10, (uvlong)s, n, 0, 0);
	if (ret < 0)
		return -1;
	return (int) ret;
}

int
devclose(int fd)
{
	vlong ret;

	ret = dispatch(0x11, fd, 0, 0, 0);
	if (ret < 0)
		return -1;
	return 0;
}

int
devread(int fd, uchar *buf, int len, int blkno)
{
	vlong ret;

	ret = dispatch(0x13, fd, len, (uvlong)buf, blkno);
	if (ret < 0)
		return -1;
	return (int) ret;
}

int
devwrite(int fd, uchar *buf, int len, int blkno)
{
	vlong ret;

	ret = dispatch(0x14, fd, len, (uvlong)buf, blkno);
	if (ret < 0)
		return -1;
	return (int) ret;
}

void
dumpenv(void)
{
	int id, n;
	static char buf[256];

	/* old upper bound was 0x100, which blows up on my 164LX. 50 works. */
	for (id = 1; id < 50; id++) {
		n = dispatch(Promop_getenv, id, (uvlong)buf, sizeof(buf)-1, 0);
		if (n == 0)
			continue;
		if (n < 0) {
			print("dispatch failed at id %d\n", id);
			break;
		}
		buf[n] = 0;
		print("env[0x%x]: %s\n", id, buf);
	}
}

char *
getenv(char *name)
{
	int id, n;
	static char buf[256];

	if (strcmp(name, "booted_dev") == 0)
		id = Promenv_booted_dev;
	else
		return 0;
	n = dispatch(Promop_getenv, id, (uvlong)buf, sizeof(buf), 0);
	if (n < 0)
		return 0;
	buf[n] = 0;
	return buf;
}

void
putstrn0(char *s, int n)
{
	uvlong ret;
	int cnt;

	for (;;) {
		ret = dispatch(2, 0, (uvlong)s, n, 0);
		cnt = (int) ret;
		s += cnt;
		n -= cnt;
		if (n <= 0)
			break;
	}
}

void
putstrn(char *s, int n)
{
	char *p;

	for (;;) {
		if (n == 0)
			return;
		p = memchr(s, '\n', n);
		if (p == 0) {
			putstrn0(s, n);
			return;
		}
		putstrn0(s, p-s);
		putstrn0("\r\n", 2);
		n -= p-s+1;
		s = p+1;
	}
}

int
snprint(char *s, int n, char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	n = doprint(s, s+n, fmt, arg) - s;
	va_end(arg);
	return n;
}

int
sprint(char *s, char *fmt, ...)
{
	int n;
	va_list arg;

	va_start(arg, fmt);
	n = doprint(s, s+PRINTSIZE, fmt, arg) - s;
	va_end(arg);
	return n;
}

int
print(char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	va_start(arg, fmt);
	n = doprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	putstrn(buf, n);

	return n;
}

void
panic(char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	strcpy(buf, "panic: ");
	va_start(arg, fmt);
	n = doprint(buf+strlen(buf), buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	buf[n] = '\n';
	putstrn(buf, n+1);
	firmware();
}

ulong
msec(void)
{
	static ulong last, wrap;
	ulong cnt;

	cnt = pcc_cnt();
	if (cnt < last)
		wrap++;
	last = cnt;
	return (((uvlong)wrap << 32) + cnt)/clk2ms;
}
