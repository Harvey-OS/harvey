#include <u.h>
#include <libc.h>
#include "flashfs.h"

int
convJ2M(Jrec *j, uchar *buff)
{
	int m, n;

	switch(j->type) {
	case FT_create:
		if(j->mode & (1 << 8)) {
			if(j->mode & DMDIR)
				j->type = FT_DCREATE1;
			else
				j->type = FT_FCREATE1;
		}
		else {
			if(j->mode & DMDIR)
				j->type = FT_DCREATE0;
			else
				j->type = FT_FCREATE0;
		}
	case FT_FCREATE0:
	case FT_FCREATE1:
	case FT_DCREATE0:
	case FT_DCREATE1:
		n = putc3(&buff[0], j->fnum);
		goto create;
	case FT_chmod:
		if(j->mode & (1 << 8))
			j->type = FT_CHMOD1;
		else
			j->type = FT_CHMOD0;
	case FT_CHMOD0:
	case FT_CHMOD1:
		n = putc3(&buff[0], j->fnum);
		buff[n++] = j->mode;
		return n + putc3(&buff[n], j->mnum);
	case FT_REMOVE:
		return putc3(&buff[0], j->fnum);
	case FT_WRITE:
		n = putc3(&buff[0], j->fnum);
		n += putc3(&buff[n], j->mtime);
		n += putc3(&buff[n], j->offset);
		return n + putc3(&buff[n], j->size - 1);
	case FT_AWRITE:
		n = putc3(&buff[0], j->fnum);
		n += putc3(&buff[n], j->offset);
		return n + putc3(&buff[n], j->size - 1);
	case FT_trunc:
		if(j->mode & (1 << 8))
			j->type = FT_TRUNC1;
		else
			j->type = FT_TRUNC0;
	case FT_TRUNC0:
	case FT_TRUNC1:
		n = putc3(&buff[0], j->fnum);
		n += putc3(&buff[n], j->tnum);
		goto create;
	case FT_SUMMARY:
	case FT_SUMBEG:
		return putc3(&buff[0], j->seq);
	case FT_SUMEND:
		return 0;
	create:
		buff[n++] = j->mode;
		n += putc3(&buff[n], j->mtime);
		n += putc3(&buff[n], j->parent);
		m = strlen(j->name);
		memmove(&buff[n], j->name, m);
		n += m;
		buff[n++] = '\0';
		return n;
	}
	return -1;
}

int
convM2J(Jrec *j, uchar *buff)
{
	int m, n;

	j->type = buff[0];

	switch(j->type) {
	case FT_FCREATE0:
	case FT_FCREATE1:
	case FT_DCREATE0:
	case FT_DCREATE1:
		n = 1 + getc3(&buff[1], &j->fnum);
		j->mode = buff[n++];
		switch(j->type) {
		case FT_FCREATE0:
			break;
		case FT_FCREATE1:
			j->mode |= 1 << 8;
			break;
		case FT_DCREATE0:
			j->mode |= DMDIR;
			break;
		case FT_DCREATE1:
			j->mode |= DMDIR | (1 << 8);
			break;
		}
		j->type = FT_create;
		goto create;
	case FT_CHMOD0:
	case FT_CHMOD1:
		n = 1 + getc3(&buff[1], &j->fnum);
		j->mode = buff[n++];
		switch(j->type) {
		case FT_CHMOD0:
			break;
		case FT_CHMOD1:
			j->mode |= 1 << 8;
			break;
		}
		j->type = FT_chmod;
		return n + getc3(&buff[n], &j->mnum);
	case FT_REMOVE:
		return 1 + getc3(&buff[1], &j->fnum);
	case FT_WRITE:
		n = 1 + getc3(&buff[1], &j->fnum);
		n += getc3(&buff[n], &j->mtime);
		n += getc3(&buff[n], &j->offset);
		n += getc3(&buff[n], &j->size);
		j->size++;
		return n;
	case FT_AWRITE:
		n = 1 + getc3(&buff[1], &j->fnum);
		n += getc3(&buff[n], &j->offset);
		n += getc3(&buff[n], &j->size);
		j->size++;
		return n;
	case FT_TRUNC0:
	case FT_TRUNC1:
		n = 1 + getc3(&buff[1], &j->fnum);
		n += getc3(&buff[n], &j->tnum);
		j->mode = buff[n++];
		switch(j->type) {
		case FT_TRUNC0:
			break;
		case FT_TRUNC1:
			j->mode |= 1 << 8;
			break;
		}
		j->type = FT_trunc;
		goto create;
	case FT_SUMMARY:
	case FT_SUMBEG:
		return 1 + getc3(&buff[1], &j->seq);
	case FT_SUMEND:
		return 1;
	create:
		n += getc3(&buff[n], &j->mtime);
		n += getc3(&buff[n], &j->parent);
		memmove(j->name, &buff[n], MAXNSIZE+1);
		j->name[MAXNSIZE+1] = '\0';
		m = strlen(j->name);
		if(m > MAXNSIZE)
			return -1;
		return n + m + 1;
	}
	return -1;
}

int
Jconv(Fmt *fp)
{
	Jrec *j;

	j = va_arg(fp->args, Jrec *);
	switch(j->type) {
	case FT_create:
	case FT_FCREATE0:
	case FT_FCREATE1:
	case FT_DCREATE0:
	case FT_DCREATE1:
		return fmtprint(fp, "create f %ld p %ld t %lud m %ulo %s",
			j->fnum, j->parent, j->mtime, j->mode, j->name);
	case FT_chmod:
	case FT_CHMOD0:
	case FT_CHMOD1:
		return fmtprint(fp, "chmod f %ld m %ulo #%ld",
			j->fnum, j->mode, j->mnum);
	case FT_REMOVE:
		return fmtprint(fp, "remove f %ld", j->fnum);
	case FT_WRITE:
		return fmtprint(fp, "write f %ld z %ld o %ld t %uld",
			j->fnum, j->size, j->offset, j->mtime);
	case FT_AWRITE:
		return fmtprint(fp, "awrite f %ld z %ld o %ld",
			j->fnum, j->size, j->offset);
	case FT_trunc:
	case FT_TRUNC0:
	case FT_TRUNC1:
		return fmtprint(fp, "trunc f %ld o %ld p %ld t %ld m %ulo %s",
			j->fnum, j->tnum, j->parent, j->mtime, j->mode, j->name);
	case FT_SUMMARY:
		return fmtprint(fp, "summary %ld",
			j->seq);
	case FT_SUMBEG:
		return fmtprint(fp, "sumbeg %ld",
			j->seq);
	case FT_SUMEND:
		return fmtprint(fp, "end");
	default:
		return fmtprint(fp, "?type %d", j->type);
	}
}
