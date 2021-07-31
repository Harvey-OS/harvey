#include	"all.h"

void
initq(IOQ *q)
{
	q->in = q->out = q->buf;
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

int
cangetc(IOQ *q)
{
	int n = q->in - q->out;

	if (n < 0)
		n += sizeof(q->buf);
	return n;
}

int
canputc(IOQ *q)
{
	return sizeof(q->buf)-cangetc(q)-1;
}
