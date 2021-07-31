#include "headers.h"

SmbProcessResult
smbcomdeletedirectory(SmbSession *s, SmbHeader *h, uchar *, SmbBuffer *b)
{
	int rv;
	char *path;
	char *fullpath = nil;
	uchar fmt;

	if (h->wordcount != 0)
		return SmbProcessResultFormat;
	if (!smbbuffergetb(b, &fmt) || fmt != 0x04 || !smbbuffergetstring(b, h, SMB_STRING_PATH, &path))
		return SmbProcessResultFormat;
	smblogprint(h->command, "smbcomdeletedirectory: %s\n", path);
        smbstringprint(&fullpath, "%s%s", s->serv->path, path);
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
