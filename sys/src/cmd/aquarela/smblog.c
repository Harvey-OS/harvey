#include "headers.h"

static QLock logreflock, logprintlock;
static int locked;

void
smbloglock(void)
{
	qlock(&logreflock);
	if (locked++ == 0)
		qlock(&logprintlock);
	qunlock(&logreflock);
}

void
smblogunlock(void)
{
	qlock(&logreflock);
	if (locked && --locked == 0)
		qunlock(&logprintlock);
	qunlock(&logreflock);
}

static int
smbloglockedvprint(char *fmt, va_list ap)
{
	if (smbglobals.log.fd >= 0)
		vfprint(smbglobals.log.fd, fmt, ap);
	if (smbglobals.log.print)
		vfprint(2, fmt, ap);
	return 0;
}

int
smblogvprint(int cmd, char *fmt, va_list ap)
{
	if (cmd < 0 || smboptable[cmd].debug) {
		smbloglock();
		smbloglockedvprint(fmt, ap);
		smblogunlock();
	}
	return 0;
}

int
smblogprint(int cmd, char *fmt, ...)
{
	if (cmd < 0 || smbtrans2optable[cmd].debug) {
		va_list ap;
		va_start(ap, fmt);
		smblogvprint(cmd, fmt, ap);
		va_end(ap);
	}
	return 0;
}

int
translogprint(int cmd, char *fmt, ...)
{
	if (cmd < 0 || smboptable[cmd].debug) {
		va_list ap;
		va_start(ap, fmt);
		smblogvprint(cmd, fmt, ap);
		va_end(ap);
	}
	return 0;
}

int
smblogprintif(int v, char *fmt, ...)
{
	if (v) {
		va_list ap;
		va_start(ap, fmt);
		smbloglock();
		smbloglockedvprint(fmt, ap);
		smblogunlock();
		va_end(ap);
	}
	return 0;
}

void
smblogdata(int cmd, int (*print)(int cmd, char *fmt, ...), void *ap, long n, long limit)
{
	uchar *p = ap;
	long i;
	long saven;
	i = 0;
	saven = n;
	if (saven > limit)
		n = limit;
	while (i < n) {
		int l = n - i < 16 ? n - i : 16;
		int b;
		(*print)(cmd, "0x%.4lux  ", i);
		for (b = 0; b < l; b += 2) {
			(*print)(cmd, " %.2ux", p[i + b]);
			if (b < l - 1)
				(*print)(cmd, "%.2ux", p[i + b + 1]);
			else
				(*print)(cmd, "  ");
		}
		while (b < 16) {
			(*print)(cmd, "     ");
			b += 2;
		}
		(*print)(cmd, "        ");
		for (b = 0; b < l; b++)
			if (p[i + b] >= ' ' && p[i + b] <= '~')
				(*print)(cmd, "%c", p[i + b]);
			else
				(*print)(cmd, ".");
		(*print)(cmd, "\n");
		i += l;
	}
	if (saven > limit)
		(*print)(cmd, "0x%.4ux   ...\n0x%.4ux\n", limit, saven);
}
