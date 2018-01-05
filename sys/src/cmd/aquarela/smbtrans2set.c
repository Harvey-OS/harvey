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
smbtrans2setfileinformation(SmbSession *s, SmbHeader *h)
{
	SmbTree *t;
	uint16_t infolevel;
	SmbBuffer *b;
	SmbProcessResult pr;
	uint16_t fid;
	SmbFile *f;
	int64_t newsize;
	uint64_t atime, mtime;
	uint32_t attr;
	uint32_t mode;

	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		pr = SmbProcessResultError;
		goto done;
	}
	b = smbbufferinit(s->transaction.in.parameters, s->transaction.in.parameters, s->transaction.in.tpcount);
	if (!smbbuffergets(b, &fid) || !smbbuffergets(b, &infolevel)) {
	misc:
		pr = SmbProcessResultMisc;
		goto done;
	}

	f = smbidmapfind(s->fidmap, fid);
	if (f == nil) {
		smbseterror(s, ERRDOS, ERRbadfid);
		pr = SmbProcessResultError;
		goto done;
	}

	switch (infolevel) {
	case SMB_SET_FILE_ALLOCATION_INFO:
	case SMB_SET_FILE_END_OF_FILE_INFO:
		if (s->transaction.in.tdcount < 8)
			goto misc;
		newsize = smbnhgetv(s->transaction.in.data);
		pr = smbtruncatefile(s, f, newsize);
		if (pr == SmbProcessResultReply && !smbbufferputs(s->transaction.out.parameters, 0))
			goto misc;
		break;

	case SMB_SET_FILE_BASIC_INFO:
		if (s->transaction.in.tdcount < 4 * 8 + 4)
			goto misc;
		atime = smbnhgetv(s->transaction.in.data + 8);
		mtime = smbnhgetv(s->transaction.in.data + 24);
		attr = smbnhgetv(s->transaction.in.data + 32);
		if (attr) {
			Dir *od = dirfstat(f->fd);
			if (od == nil)
				goto noaccess;
			mode = smbdosattr2plan9wstatmode(od->mode, attr);
			free(od);
		}
		else
			mode = 0xffffffff;
		if (atime || mtime || mode != 0xffffffff) {
			Dir d;
			memset(&d, 0xff, sizeof(d));
			d.name = d.uid = d.gid = d.muid = nil;
			if (atime)
				d.atime = smbtime2plan9time(atime);
			if (mtime)
				d.mtime = smbtime2plan9time(mtime);
			d.mode = mode;
			if (dirfwstat(f->fd, &d) < 0) {
			noaccess:
				smbseterror(s, ERRDOS, ERRnoaccess);
				pr = SmbProcessResultError;
				goto done;
			}
		}
		if (!smbbufferputs(s->transaction.out.parameters, 0))
			goto misc;
		pr = SmbProcessResultReply;
		break;

	case SMB_SET_FILE_DISPOSITION_INFO:
		if (s->transaction.in.tdcount < 1)
			goto misc;
		f->sf->deleteonclose = *s->transaction.in.data;
		if (!smbbufferputs(s->transaction.out.parameters, 0))
			goto misc;
		pr = SmbProcessResultReply;
		break;

	default:
		smblogprint(-1, "smbtrans2setfileinformation: infolevel 0x%.4x not implemented\n", infolevel);
		smbseterror(s, ERRDOS, ERRunknownlevel);
		pr = SmbProcessResultError;
		break;
	}
done:
	smbbufferfree(&b);
	return pr;
}

SmbProcessResult
smbtrans2setpathinformation(SmbSession *s, SmbHeader *h)
{
	char *fullpath, *path;
	SmbTree *t;
	uint16_t infolevel;
	SmbBuffer *b;
	SmbProcessResult pr;
	uint16_t atime, adate, mtime, mdate;
	uint32_t attr;
	uint32_t mode;
	uint32_t size;
//	uvlong length;

	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		pr = SmbProcessResultError;
		goto done;
	}
	b = smbbufferinit(s->transaction.in.parameters, s->transaction.in.parameters, s->transaction.in.tpcount);
	path = nil;
	if (!smbbuffergets(b, &infolevel) || !smbbuffergetbytes(b, nil, 4)
		|| !smbbuffergetstring(b, h, SMB_STRING_PATH, &path)) {
	misc:
		pr = SmbProcessResultMisc;
		goto done;
	}

	fullpath = nil;
	smbstringprint(&fullpath, "%s%s", t->serv->path, path);

	translogprint(s->transaction.in.setup[0], "path %s\n", path);
	translogprint(s->transaction.in.setup[0], "infolevel 0x%.4x\n", infolevel);
	translogprint(s->transaction.in.setup[0], "fullpath %s\n", fullpath);

	switch (infolevel) {
	case SMB_INFO_STANDARD:
		if (s->transaction.in.tdcount < 6 * 4 + 2 * 2)
			goto misc;
		adate = smbnhgets(s->transaction.in.data + 6);
		atime = smbnhgets(s->transaction.in.data + 4);
		mdate = smbnhgets(s->transaction.in.data + 10);
		mtime = smbnhgets(s->transaction.in.data + 8);
		size = smbnhgetl(s->transaction.in.data + 12);
		attr = smbnhgets(s->transaction.in.data + 20);
		if (attr) {
			Dir *od = dirstat(fullpath);
			if (od == nil)
				goto noaccess;
			mode = smbdosattr2plan9wstatmode(od->mode, attr);
			free(od);
		}
		else
			mode = 0xffffffff;
		translogprint(s->transaction.in.setup[0], "mode 0%od\n", mode);

//		if (size)
//			length = size;
//		else
//			length = ~0LL;
	
		translogprint(s->transaction.in.setup[0], "size %lld\n", size);
		translogprint(s->transaction.in.setup[0], "adate %d atime %d", adate, atime);
		translogprint(s->transaction.in.setup[0], "mdate %d mtime %d\n", mdate, mtime);

		if (size || adate || atime || mdate || mtime || mode != 0xffffffff) {
			Dir d;
			memset(&d, 0xff, sizeof(d));
			d.name = d.uid = d.gid = d.muid = nil;
			if (adate || atime)
				d.atime = smbdatetime2plan9time(adate, atime, s->tzoff);
			if (mdate || mtime)
				d.mtime = smbdatetime2plan9time(mdate, mtime, s->tzoff);
			d.mode = mode;
			d.length = size;
			if (dirwstat(fullpath, &d) < 0) {
			noaccess:
				smbseterror(s, ERRDOS, ERRnoaccess);
				pr = SmbProcessResultError;
				goto done;
			}
		}
		if (!smbbufferputs(s->transaction.out.parameters, 0))
			goto misc;
		pr = SmbProcessResultReply;
		break;

	default:
		smblogprint(-1, "smbtrans2setpathinformation: infolevel 0x%.4x not implemented\n", infolevel);
		smbseterror(s, ERRDOS, ERRunknownlevel);
		pr = SmbProcessResultError;
		break;
	}
done:
	smbbufferfree(&b);
	return pr;
}
