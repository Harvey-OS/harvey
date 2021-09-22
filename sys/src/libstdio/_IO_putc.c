/*
 * pANS stdio -- _IO_putc, _IO_cleanup
 */
#include "iolib.h"

void
_IO_cleanup(void)
{
	fflush(NULL);
}

static long
_write(void *s, long n, FILE *f)
{
	long d;

	if (n <= 0)
		return 0;
	if (f->flags & APPEND)
		seek(f->fd, 0, SEEK_END);
	d = write(f->fd, s, n);
	if (d != n) {
		f->state = ERR;
		return EOF;
	}
	return d;
}

/*
 * Look this over for simplification
 */
int
_IO_putc(int c, FILE *f)
{
	static int first = 1;

	switch (f->state) {
	case RD:
		f->state = ERR;
		/* fall through */
	case ERR:
	case CLOSED:
		return EOF;
	case OPEN:
		_IO_setvbuf(f);
		/* fall through */
	case RDWR:
	case END:
		f->rp = f->buf + f->bufl;
		if (f->flags & LINEBUF) {
			f->wp = f->rp;
			f->lp = f->buf;
		} else
			f->wp = f->buf;
		break;
	}
	if (first) {
		first = 0;
		atexit(_IO_cleanup);
	}
	if (f->flags & STRING) {
		f->rp = f->buf + f->bufl;
		if (f->wp == f->rp) {
			if (f->flags & BALLOC)
				f->buf = realloc(f->buf, f->bufl + BUFSIZ);
			else {
				f->state = ERR;
				return EOF;
			}
			if (f->buf == NULL) {
				f->state = ERR;
				return EOF;
			}
			f->rp = f->buf + f->bufl;
			f->bufl += BUFSIZ;
		}
		*f->wp++ = c;
	} else if (f->flags & LINEBUF) {
		if (f->lp == f->rp) {
			if (_write(f->buf, f->lp - f->buf, f) == EOF)
				return EOF;
			f->lp = f->buf;
		}
		*f->lp++ = c;
		if (c == '\n') {
			if (_write(f->buf, f->lp - f->buf, f) == EOF)
				return EOF;
			f->lp = f->buf;
		}
	} else if (f->buf == f->unbuf) {
		f->unbuf[0] = c;
		if (_write(f->buf, 1, f) == EOF)
			return EOF;
	} else {
		if (f->wp == f->rp) {			/* full buffer? */
			if (_write(f->buf, f->wp - f->buf, f) == EOF)
				return EOF;
			f->wp = f->buf;
			f->rp = f->buf + f->bufl;
		}
		*f->wp++ = c;
	}
	f->state = WR;
	/*
	 * Make sure EOF looks different from putc(-1)
	 * Should be able to cast to unsigned char, but
	 * there's a vc bug preventing that from working
	 */
	return c & 0xff;
}
