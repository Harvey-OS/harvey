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
smbcomcheckdirectory(SmbSession *s, SmbHeader *h, uchar *, SmbBuffer *b)
{
	char *path;
	Dir *d;
	uchar fmt;
	SmbProcessResult pr;
	SmbTree *t;
	char *fullpath = nil;

	if (!smbcheckwordcount("comcheckdirectory", h, 0))
		return SmbProcessResultFormat;

	if (!smbbuffergetb(b, &fmt)
		|| fmt != 4
		|| !smbbuffergetstring(b, h, SMB_STRING_PATH, &path))
		return SmbProcessResultFormat;

	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		return SmbProcessResultError;
	}
	
	smbstringprint(&fullpath, "%s%s", t->serv->path, path);
smblogprintif(1, "smbcomcheckdirectory: statting %s\n", fullpath);
	d = dirstat(fullpath);

	if (d == nil || (d->mode & DMDIR) == 0) {
		smbseterror(s, ERRDOS, ERRbadpath);
		pr = SmbProcessResultError;
		goto done;
	}

	if (access(fullpath, AREAD) < 0) {
		smbseterror(s, ERRDOS, ERRbadpath);
		pr = SmbProcessResultError;
		goto done;
	}

	pr = smbbufferputack(s->response, h, &s->peerinfo) ? SmbProcessResultReply : SmbProcessResultMisc;
done:
	free(fullpath);
	free(path);
	free(d);
	return pr;
}
