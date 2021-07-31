#include "headers.h"

SmbProcessResult
smbcomcreatedirectory(SmbSession *s, SmbHeader *h, uchar *, SmbBuffer *b)
{
	int fd;
	char *path;
	uchar fmt;

	if (h->wordcount != 0)
		return SmbProcessResultFormat;
	if (!smbbuffergetb(b, &fmt) || fmt != 0x04 || !smbbuffergetstring(b, h, SMB_STRING_PATH, &path))
		return SmbProcessResultFormat;
	smblogprint(h->command, "smbcomcreatedirectory: %s\n", path);
	fd = create(path, OREAD, DMDIR | 0775);
	if (fd < 0) {
		smblogprint(h->command, "smbcomcreatedirectory failed: %r\n");
		smbseterror(s, ERRDOS, ERRnoaccess);
		free(path);
		return SmbProcessResultError;
	}
	close(fd);
	free(path);
	return smbbufferputack(s->response, h, &s->peerinfo);
}
