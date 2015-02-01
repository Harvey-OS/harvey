/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

SmbDirCache *
smbmkdircache(SmbTree *t, char *path)
{
	long n;
	SmbDirCache *c;
	Dir *buf;
	int fd;
	char *fullpath = nil;

	smbstringprint(&fullpath, "%s%s", t->serv->path, path);
//smblogprintif(1, "smbmkdircache: path %s\n", fullpath);
	fd = open(fullpath, OREAD);
	free(fullpath);

	if (fd < 0)
		return nil;
	n = dirreadall(fd, &buf);
	close(fd);
	if (n < 0) {
		free(buf);
		return nil;
	}
	c = smbemalloc(sizeof(SmbDirCache));
	c->buf = buf;
	c->n = n;
	c->i = 0;
	return c;
}

void
smbdircachefree(SmbDirCache **cp)
{
	SmbDirCache *c;
	c = *cp;
	if (c) {
		free(c->buf);
		free(c);
		*cp = nil;
	}
}

