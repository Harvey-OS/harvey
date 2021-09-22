/*
 * pANS stdio -- fwrite
 */
#include "iolib.h"

#define BIGN (BUFSIZ/2)

static long
_write(void *s, long n, FILE *f)
{
	long d;

	d = write(f->fd, s, n);
	if (d != n) {
		f->state = ERR;
		return EOF;
	}
	return d;
}

long
fwrite(const void *p, long recl, long nrec, FILE *f)
{
	char *s;
	int n, d;

	s = (char *)p;
	for (n = recl * nrec; n > 0; n -= d) {
		d = f->rp - f->wp;
		if (d > 0) {
			if (d > n)
				d = n;
			memmove(f->wp, s, d);	/* buffer what fits */
			f->wp += d;
		} else if (n >= BIGN && f->state == WR &&
		    !(f->flags & (STRING | LINEBUF)) && f->buf != f->unbuf) {
			if (f->flags & APPEND)
				seek(f->fd, 0, SEEK_END);
			d = f->wp - f->buf;
			if (d > 0) {		/* buffered output? flush it */
				if (_write(f->buf, d, f) == EOF)
					break;
				f->wp = f->rp = f->buf;
			}
			d = _write(s, n, f);
			if (d == EOF)
				break;
		} else {
			if (_IO_putc(*s, f) == EOF)
				break;
			d = 1;
		}
		s += d;
	}
	if (recl > 1)
		return (s - (char *)p) / recl;
	else
		return s - (char *)p;
}
