#include <u.h>
#include <libc.h>
#include "fmtdef.h"

/*
 * public routine for final flush of a formatting buffer
 * to a file descriptor; returns total char count.
 */
int
fmtfdflush(Fmt *f)
{
	if(_fmtFdFlush(f) <= 0)
		return -1;
	return f->nfmt;
}

/*
 * initialize an output buffer for buffered printing
 */
int
fmtfdinit(Fmt *f, int fd, char *buf, int size)
{
	f->runes = 0;
	f->start = buf;
	f->to = buf;
	f->stop = buf + size;
	f->flush = _fmtFdFlush;
	f->farg = (void*)fd;
	f->nfmt = 0;
	return 0;
}
