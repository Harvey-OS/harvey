#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

void
initq(IOQ *q)
{
	lock(q);
	unlock(q);
	q->in = q->out = q->buf;
	q->puts = puts;
	q->putc = putc;
}

int
putc(IOQ *q, int c)
{
	uchar *nextin;
	if(q->in >= &q->buf[sizeof(q->buf)-1])
		nextin = q->buf;
	else
		nextin = q->in+1;
	if(nextin == q->out)
		return -1;
	*q->in = c;
	q->in = nextin;
	return 0;
}

int
getc(IOQ *q)
{
	int c;

	if(q->in == q->out)
		return -1;
	c = *q->out;
	if(q->out == q->buf+sizeof(q->buf)-1)
		q->out = q->buf;
	else
		q->out++;
	return c;
}

void
puts(IOQ *q, void *buf, int n)
{
	int m; uchar *nextin;
	uchar *s = buf;

	while(n){
		m = q->out - q->in - 1;
		if(m < 0)
			m = &q->buf[NQ] - q->in;
		if(m == 0)
			break;
		if(m > n)
			m = n;
		memmove(q->in, s, m);
		n -= m;
		s += m;
		nextin = q->in + m;
		if(nextin >= &q->buf[NQ])
			q->in = q->buf;
		else
			q->in = nextin;
	}
}

int
gets(IOQ *q, void *buf, int n)
{
	int m, k = 0; uchar *nextout;
	uchar *s = buf;

	while(n){
		m = q->in - q->out;
		if(m < 0)
			m = &q->buf[NQ] - q->out;
		if(m == 0)
			return k;
		if(m > n)
			m = n;
		memmove(s, q->out, m);
		s += m;
		k += m;
		n -= m;
		nextout = q->out + m;
		if(nextout >= &q->buf[NQ])
			q->out = q->buf;
		else
			q->out = nextout;
	}
	return k;
}

int
cangetc(void *arg)
{
	IOQ *q = (IOQ *)arg;
	int n = q->in - q->out;
	if (n < 0)
		n += sizeof(q->buf);
	return n;
}

int
canputc(void *arg)
{
	IOQ *q = (IOQ *)arg;
	return sizeof(q->buf)-cangetc(q)-1;
}
