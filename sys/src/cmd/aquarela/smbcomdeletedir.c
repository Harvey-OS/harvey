#include "headers.h"

SmbProcessResult
smbcomdeletedirectory(SmbSession *s, SmbHeader *h, uchar *, SmbBuffer *b)
{
	int rv;
	char *path;
	uchar fmt;

	if (h->wordcount != 0)
		return SmbProcessResultFormat;
	if (!smbbuffergetb(b, &fmt) || fmt != 0x04 || !smbbuffergetstring(b, h, SMB_STRING_PATH, &path))
		return SmbProcessResultFormat;
	smblogprint(h->command, "smbcomdeletedirectory: %s\n", path);
	rv = remove(path);
	if (rv < 0) {
		smblogprint(h->command, "smbcomdeletedirectory failed: %r\n");
		smbseterror(s, ERRDOS, ERRnoaccess);
		free(path);
		return SmbProcessResultError;
	}
	free(path);
	return smbbufferputack(s->response, h, &s->peerinfo);
}
