#include "headers.h"

SmbProcessResult
smbcomcreatedirectory(SmbSession *s, SmbHeader *h, uchar *, SmbBuffer *b)
{
	int fd;
	char *path;
	char *fullpath = nil;
	SmbTree *t;
	uchar fmt;

	if (h->wordcount != 0)
		return SmbProcessResultFormat;
	if (!smbbuffergetb(b, &fmt) || fmt != 0x04 || !smbbuffergetstring(b, h, SMB_STRING_PATH, &path))
		return SmbProcessResultFormat;
	smblogprint(h->command, "smbcomcreatedirectory: %s\n", path);
	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		return SmbProcessResultError;
	}
	smbstringprint(&fullpath, "%s%s", t->serv->path, path);
	fd = create(fullpath, OREAD, DMDIR | 0775);
	if (fd < 0) {
		smblogprint(h->command, "smbcomcreatedirectory failed: %r\n");
		smbseterror(s, ERRDOS, ERRnoaccess);
		free(path);
		return SmbProcessResultError;
	}
	close(fd);
	free(fullpath);
	free(path);
	return smbbufferputack(s->response, h, &s->peerinfo);
}
