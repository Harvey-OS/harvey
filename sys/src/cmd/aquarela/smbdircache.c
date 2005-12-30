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

