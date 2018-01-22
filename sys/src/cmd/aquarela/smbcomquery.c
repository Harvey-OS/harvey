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
smbcomqueryinformation(SmbSession *s, SmbHeader *h, uint8_t *l, SmbBuffer *b)
{
	SmbTree *t;
	uint8_t fmt;
	char *path;
	Dir *d;
	char *fullpath;

	if (!smbcheckwordcount("comqueryinformation", h, 0)
		|| !smbbuffergetb(b, &fmt)
		|| fmt != 4
		|| !smbbuffergetstring(b, h, SMB_STRING_PATH, &path))
		return SmbProcessResultFormat;
	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		free(path);
		smbseterror(s, ERRSRV, ERRinvtid);
		return SmbProcessResultError;
	}
	smblogprint(h->command, "smbcomqueryinformation: %s\n", path);
	fullpath = nil;
	smbstringprint(&fullpath, "%s%s", t->serv->path, path);
	d = dirstat(fullpath);
	free(fullpath);
	free(path);
	if (d == nil) {
		smbseterror(s, ERRDOS, ERRbadpath);
		return SmbProcessResultError;
	}
	h->wordcount = 10;
	if (!smbbufferputheader(s->response, h, &s->peerinfo)
		|| !smbbufferputs(s->response, smbplan9mode2dosattr(d->mode))
		|| !smbbufferputl(s->response, smbplan9time2utime(d->mtime, s->tzoff))
		|| !smbbufferputl(s->response, smbplan9length2size32(d->length))
		|| !smbbufferfill(s->response, 0, 10)
		|| !smbbufferputs(s->response, 0)) {
		free(d);
		return SmbProcessResultMisc;
	}
	free(d);
	return SmbProcessResultReply;
}

SmbProcessResult
smbcomqueryinformation2(SmbSession *s, SmbHeader *h, uint8_t *pdata,
			SmbBuffer *sb)
{
	SmbTree *t;
	Dir *d;
	uint16_t fid;
	uint16_t mtime, mdate;
	uint16_t atime, adate;
	SmbFile *f;

	if (!smbcheckwordcount("comqueryinformation2", h, 1))
		return SmbProcessResultFormat;
	fid = smbnhgets(pdata);
	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		return SmbProcessResultError;
	}
	f = smbidmapfind(s->fidmap, fid);
	if (f == nil) {
		smbseterror(s, ERRDOS, ERRbadfid);
		return SmbProcessResultError;
	}
	d = dirfstat(f->fd);
	if (d == nil) {
		smbseterror(s, ERRDOS, ERRbadpath);
		return SmbProcessResultError;
	}
	h->wordcount = 11;
	smbplan9time2datetime(d->atime, s->tzoff, &adate, &atime);
	smbplan9time2datetime(d->mtime, s->tzoff, &mdate, &mtime);
	if (!smbbufferputheader(s->response, h, &s->peerinfo)
		|| !smbbufferputs(s->response, mdate)
		|| !smbbufferputs(s->response, mtime)
		|| !smbbufferputs(s->response, adate)
		|| !smbbufferputs(s->response, atime)
		|| !smbbufferputs(s->response, mdate)
		|| !smbbufferputs(s->response, mtime)
		|| !smbbufferputl(s->response, smbplan9length2size32(d->length))
		|| !smbbufferputl(s->response,
			smbplan9length2size32(smbl2roundupint64_t(d->length, smbglobals.l2allocationsize)))
		|| !smbbufferputs(s->response, smbplan9mode2dosattr(d->mode))
		|| !smbbufferputs(s->response, 0)) {
		free(d);
		return SmbProcessResultMisc;
	}
	free(d);
	return SmbProcessResultReply;
}
