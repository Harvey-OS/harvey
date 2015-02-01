/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
