#include "headers.h"

SmbProcessResult
smbcomdeletedirectory(SmbSession *s, SmbHeader *h, uchar *, SmbBuffer *b)
{
	int rv;
	char *path;
	char *fullpath = nil;
	SmbTree *t;
	uchar fmt;

	if (h->wordcount != 0)
		return SmbProcessResultFormat;
	if (!smbbuffergetb(b, &fmt) || fmt != 0x04 || !smbbuffergetstring(b, h, SMB_STRING_PATH, &path))
		return SmbProcessResultFormat;
	smblogprint(h->command, "smbcomdeletedirectory: %s\n", path);
	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		return SmbProcessResultError;
	}
	smbstringprint(&fullpath, "%s%s", t->serv->path, path);
	rv = remove(fullpath);
	if (rv < 0) {
		smblogprint(h->command, "smbcomdeletedirectory failed: %r\n");
		smbseterror(s, ERRDOS, ERRnoaccess);
		free(path);
		free(fullpath);
		return SmbProcessResultError;
	}
	free(path);
	free(fullpath);
	return smbbufferputack(s->response, h, &s->peerinfo);
}
