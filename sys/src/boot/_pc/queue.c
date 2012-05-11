#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

int
qgetc(IOQ *q)
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

static int
qputc(IOQ *q, int c)
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

void
qinit(IOQ *q)
{
	q->in = q->out = q->buf;
	q->getc = qgetc;
	q->putc = qputc;
}
