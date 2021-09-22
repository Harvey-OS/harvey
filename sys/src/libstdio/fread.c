/*
 * pANS stdio -- fread
 */
#include "iolib.h"

#define BIGN (BUFSIZ/2)

long
fread(void *p, long recl, long nrec, FILE *f)
{
	char *s;
	int n, d, c;

	s = (char *)p;
	for (n = recl * nrec; n > 0; n -= d) {
		d = f->wp - f->rp;
		if (d > 0) {
			if (d > n)
				d = n;
			memmove(s, f->rp, d);
			f->rp += d;
		} else if (n >= BIGN && f->state == RD && // TODO: RDWR too?
		    !(f->flags & STRING) && f->buf != f->unbuf) {
			d = read(f->fd, s, n);
			if (d <= 0) {
				f->state = d == 0? END: ERR;
				break;
			}
		} else {
			c = _IO_getc(f);
			if (c == EOF)
				break;
			*s = c;
			d = 1;
		}
		s += d;
	}
	if (recl > 1)
		return (s - (char *)p) / recl;
	else
		return s - (char *)p;
}
