/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <venti.h>
#include <mp.h>
#include <libsec.h>

#define MAGIC 0x54798314
#define NOTFREE(p)	assert((p)->magic == MAGIC)

struct Packet
{
	char *data;
	int len;
	void (*free)(void*);
	void *arg;
	int magic;
};

Packet*
packetalloc(void)
{
	Packet *p;

	p = vtmallocz(sizeof *p);
	p->free = vtfree;
	p->arg = nil;
	p->magic = MAGIC;
	return p;
}

void
packetappend(Packet *p, uint8_t *buf, int n)
{
	NOTFREE(p);
	if(n < 0)
		abort();
	if(p->free != vtfree)
		sysfatal("packetappend");
	p->data = vtrealloc(p->data, p->len+n);
	p->arg = p->data;
	memmove(p->data+p->len, buf, n);
	p->len += n;
}

uint
packetasize(Packet *p)
{
	NOTFREE(p);
	return p->len;
}

int
packetcmp(Packet *p, Packet *q)
{
	int i, len;

	NOTFREE(p);
	NOTFREE(q);
	len = p->len;
	if(len > q->len)
		len = q->len;
	if(len && (i=memcmp(p->data, q->data, len)) != 0)
		return i;
	if(p->len > len)
		return 1;
	if(q->len > len)
		return -1;
	return 0;
}

void
packetconcat(Packet *p, Packet *q)
{
	NOTFREE(p);
	NOTFREE(q);
	packetappend(p, q->data, q->len);
	if(q->free == vtfree)
		memset(q->data, 0xFE, q->len);
	q->free(q->arg);
	q->data = nil;
	q->len = 0;
}

int
packetconsume(Packet *p, uint8_t *buf, int n)
{
	NOTFREE(p);
	if(n < 0)
		abort();
	if(p->len < n)
		abort();
	memmove(buf, p->data, n);
	p->len -= n;
	memmove(p->data, p->data+n, p->len);
	return 0;
}

int
packetcopy(Packet *p, uint8_t *buf, int offset, int n)
{
	NOTFREE(p);
	if(offset < 0 || n < 0)
		abort();
	if(offset > p->len)
		abort();
	if(offset+n > p->len)
		n = p->len - offset;
	memmove(buf, p->data+offset, n);
	return 0;
}

Packet*
packetdup(Packet *p, int offset, int n)
{
	Packet *q;

	NOTFREE(p);
	if(offset < 0 || n < 0)
		abort();
	if(offset > p->len)
		abort();
	if(offset+n > p->len)
		n = p->len - offset;
	q = packetalloc();
	packetappend(q, p->data+offset, n);
	return q;
}

Packet*
packetforeign(uint8_t *buf, int n, void (*free)(void*), void *a)
{
	Packet *p;

	if(n < 0)
		abort();
	p = packetalloc();
	p->data = (char*)buf;
	p->len = n;
	p->free = free;
	p->arg = a;
	return p;
}

int
packetfragments(Packet *p, IOchunk *io, int nio, int offset)
{
	NOTFREE(p);
	if(offset < 0)
		abort();
	if(nio == 0)
		return 0;
	memset(io, 0, sizeof(io[0])*nio);
	if(offset >= p->len)
		return 0;
	io[0].addr = p->data + offset;
	io[0].len = p->len - offset;
	return p->len;
}

void
packetfree(Packet *p)
{
	NOTFREE(p);
	if(p->free == free)
		memset(p->data, 0xFE, p->len);
	p->free(p->arg);
	p->data = nil;
	p->len = 0;
	memset(p, 0xFB, sizeof *p);
	free(p);
}

uint8_t*
packetheader(Packet *p, int n)
{
	NOTFREE(p);
	if(n < 0)
		abort();
	if(n > p->len)
		abort();
	return p->data;
}

uint8_t*
packetpeek(Packet *p, uint8_t *buf, int offset, int n)
{
	NOTFREE(p);
	if(offset < 0 || n < 0)
		abort();
	if(offset+n > p->len)
		abort();
	return p->data+offset;
}

void
packetprefix(Packet *p, uint8_t *buf, int n)
{
	NOTFREE(p);
	if(n < 0)
		abort();
	if(p->free != free)
		sysfatal("packetappend");
	p->data = vtrealloc(p->data, p->len+n);
	p->arg = p->data;
	memmove(p->data+n, p->data, p->len);
	memmove(p->data, buf, n);
	p->len += n;
}

void
packetsha1(Packet *p, uint8_t d[20])
{
	NOTFREE(p);
	sha1((uint8_t*)p->data, p->len, d, nil);
}

uint
packetsize(Packet *p)
{
	NOTFREE(p);
	return p->len;
}

Packet*
packetsplit(Packet *p, int n)
{
	Packet *q;

	NOTFREE(p);
	q = packetalloc();
	q->data = vtmalloc(n);
	q->arg = q->data;
	q->free = vtfree;
	packetconsume(p, q->data, n);
	return q;
}

void
packetstats(void)
{
}

uint8_t*
packettrailer(Packet *p, int n)
{
	NOTFREE(p);
	if(n < 0)
		abort();
	if(n > p->len)
		abort();
	return p->data + p->len - n;
}

int
packettrim(Packet *p, int offset, int n)
{
	NOTFREE(p);
	if(offset < 0 || n < 0)
		abort();
	if(offset+n > p->len)
		abort();
	memmove(p->data+offset, p->data+offset+n, p->len-offset-n);
	p->len -= n;
	return 0;
}
