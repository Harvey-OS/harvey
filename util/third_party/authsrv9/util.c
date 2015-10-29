#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "util.h"

#include "des.h"

long
readn(int fd, void *buf, long n)
{
	long have;
	long nn;

	have = 0;
	while(have < n) {
		nn = read(fd, buf+have, n-have);
		if(nn < 0)
			return nn;
		if(nn == 0)
			break;
		have += nn;
	}
	return have;
}

int
vfprint(int fd, char *fmt, va_list ap)
{
	char *p;
	int r;

	r = vasprintf(&p, fmt, ap);
	if(r < 0)
		return r;
	if(write(fd, p, r) != r)
		r = -1;
	free(p);
	return r;
}

int
fprint(int fd, char *fmt, ...)
{
	va_list ap;
	int r;

	va_start(ap, fmt);
	r = vfprint(fd, fmt, ap);
	va_end(ap);
	return r;
}

int
print(char *fmt, ...)
{
	va_list ap;
	int r;

	va_start(ap, fmt);
	r = vfprint(1, fmt, ap);
	va_end(ap);
	return r;
}

char *
estrdup(char *s)
{
	char *p;
	int n;

	n = strlen(s);
	p = malloc(n+1);
	memmove(p, s, n+1);
	return p;
}

void
randominit(void)
{
	srandom(time(nil));
}

void
randombuf(uchar *p, int n)
{
	long r;
	uchar *e;

	e = p+n;
	while(p+3 < e) {
		r = random();
		*p++ = r; r >>= 8;
		*p++ = r; r >>= 8;
		*p++ = r; r >>= 8;
	}
	r = random();
	while(p < e) {
		*p++ = r; r >>= 8;
	}
}

int
min(int a, int b)
{
	if(a < b)
		return a;
	return b;
}

char *
hex(uchar *d, int n)
{
	static char buf[512+1];
	int i;

	n = min(n, sizeof buf/2);
	for(i = 0; i < n; i++)
		snprintf(buf+i*2, 3, "%02x", d[i]);
	return buf;
}

int
eq(char *a, char *b)
{
	return strcmp(a, b) == 0;
}

int
memeq(void *a, void *b, int n)
{
	return memcmp(a, b, n) == 0;
}

char *
remoteaddr(int fd)
{
	struct sockaddr_storage ss;
	socklen_t sslen;
	char host[32];
	char port[16];
	static char addr[64];

	sslen = sizeof ss;
	if(getpeername(fd, (struct sockaddr*)&ss, &sslen) != 0)
		return "unknown";

	if(getnameinfo((struct sockaddr*)&ss, sslen, host, sizeof host, port, sizeof port, NI_NUMERICSERV|NI_NUMERICHOST) != 0)
		return "unknown";

	snprintf(addr, sizeof addr, "%s", host);
	return addr;
}


void
passtokey(uchar *key, char *pw)
{
	uchar buf[28];
	int n, t, i;

	n = strlen(pw);
	if(n >= 28)
		n = 28-1;
	memset(buf, ' ', sizeof buf);
	memmove(buf, pw, n);
	buf[n] = 0;

	memset(key, 0, Deskeylen);
	t = 0;
	for(;;) {
		for(i = 0; i < Deskeylen; i++)
			key[i] = (buf[t+i]>>i) + ((uint)buf[t+i+1] << (8 - (i+1)));
		if(n <= 8)
			return;
		n -= 8;
		t += 8;
		if(n < 8) {
			t -= 8-n;
			n = 8;
		}
		authencrypt(key, buf+t, 8);
	}
}


void
authencrypt(uchar *k56, uchar *buf, int n)
{
	uchar *p;
	int i;
	int r;
	Desks ks;

	if(n < 8)
		exit(1); /* not much better to do */
	
	deskey56(&ks, k56);

	n -= 1;
	r = n%7;
	n /= 7;
	p = buf;
	for(i = 0; i < n; i++) {
		desenc(&ks, p);
		p += 7;
	}
	if(r)
		desenc(&ks, p-7+r);
}

void
authdecrypt(uchar *k56, uchar *buf, int n)
{
	uchar *p;
	int i;
	int r;
	Desks ks;

	if(n < 8)
		exit(1); /* not much better to do */
	
	deskey56(&ks, k56);

	p = buf;
	n -= 1;
	r = n%7;
	n /= 7;
	p += n*7;
	if(r)
		desdec(&ks, p-7+r);
	for(i = 0; i < n; i++) {
		p -= 7;
		desdec(&ks, p);
	}
}

