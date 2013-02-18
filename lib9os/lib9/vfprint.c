/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */
#include "lib9.h"
#include "fmtdef.h"

/*
 * generic routine for flushing a formatting buffer
 * to a file descriptor
 */
int
_fmtFdFlush(Fmt *f)
{
	int n;

	n = (char*)f->to - (char*)f->start;
	if(n && write((int)f->farg, f->start, n) != n)
		return 0;
	f->to = f->start;
	return 1;
}

int
vfprint(int fd, char *fmt, va_list args)
{
	Fmt f;
	char buf[256];
	int n;

	fmtfdinit(&f, fd, buf, sizeof(buf));
	va_copy(f.args, args);
	n = dofmt(&f, fmt);
	va_end(f.args);
	if(n > 0 && _fmtFdFlush(&f) == 0)
		return -1;
	return n;
}
