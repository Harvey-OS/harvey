/*
 * pANS stdio -- fgets
 */
#include "iolib.h"

char *
fgets(char *buf, int size, FILE *f)
{
	int n, c, avail;
	char *p, *ptr;

	if (size <= 0 ||
	    !(f->state == RD || f->state == RDWR || f->state == OPEN))
		return NULL;
	ptr = buf;
	for (size--; size > 0; size -= n) {
		if (f->rp >= f->wp) {		/* empty buffer? fill it */
			c = _IO_getc(f);
			if (c == EOF || ferror(f)) {
				if (buf == ptr)
					return NULL;
				break;
			}
			--f->rp;		/* back up to c */
		}

		avail = f->wp - f->rp;
		n = size < avail? size: avail;	/* maximum to copy */
		assert(n > 0);
		/* copy into ptr; return ptr. to byte after those copied */
		p = memccpy(ptr, f->rp, '\n', n);
		if (p != NULL)			/* stopped at newline? */
			n = p - ptr;
		/* else stopped at end of buffer */
		ptr += n;
		f->rp += n;
		if (p != NULL)
			break;
	}
	*ptr = '\0';
	return buf;
}
