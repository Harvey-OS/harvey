#include "headers.h"

static SmbProcessResult
query(SmbSession *s, char *cmdname, char *filename, ushort infolevel, vlong cbo, Dir *d)
{
	vlong ntatime, ntmtime;
	ushort dosmode;
	ulong fnlfixupoffset;
	vlong allocsize;

	if (d == nil) {
		smbseterror(s, ERRDOS, ERRbadfile);
		return SmbProcessResultError;
	}

	switch (infolevel) {
	case SMB_QUERY_FILE_BASIC_INFO:
		ntatime = smbplan9time2time(d->atime);
		ntmtime = smbplan9time2time(d->mtime);
		dosmode = smbplan9mode2dosattr(d->mode);
		if (!smbbufferputv(s->transaction.out.data, ntmtime)
			|| !smbbufferputv(s->transaction.out.data, ntatime)
			|| !smbbufferputv(s->transaction.out.data, ntmtime)
			|| !smbbufferputv(s->transaction.out.data, ntmtime)
			|| !smbbufferputl(s->transaction.out.data, dosmode))
//			|| !smbbufferputl(s->transaction.out.data, 0))
			return SmbProcessResultMisc;
		break;
	case SMB_QUERY_FILE_ALL_INFO:
		ntatime = smbplan9time2time(d->atime);
		ntmtime = smbplan9time2time(d->mtime);
		dosmode = smbplan9mode2dosattr(d->mode);
		allocsize = (d->length + (1 << smbglobals.l2allocationsize) - 1) & ~((1 << smbglobals.l2allocationsize) - 1);
		if (!smbbufferputv(s->transaction.out.data, ntmtime)
			|| !smbbufferputv(s->transaction.out.data, ntatime)
			|| !smbbufferputv(s->transaction.out.data, ntmtime)
			|| !smbbufferputv(s->transaction.out.data, ntmtime)
			|| !smbbufferputs(s->transaction.out.data, dosmode)
			|| !smbbufferputbytes(s->transaction.out.data, nil, 6)
			|| !smbbufferputv(s->transaction.out.data, allocsize)
			|| !smbbufferputv(s->transaction.out.data, d->length)
			|| !smbbufferputl(s->transaction.out.data, 0)		// hard links - ha
			|| !smbbufferputb(s->transaction.out.data, 0)		// TODO delete pending
			|| !smbbufferputb(s->transaction.out.data, (d->mode & DMDIR) != 0)
			|| !smbbufferputv(s->transaction.out.data, d->qid.path)
			|| !smbbufferputl(s->transaction.out.data, 0)		// EA size
			|| !smbbufferputl(s->transaction.out.data, (dosmode & SMB_ATTR_READ_ONLY) ? 0xa1 : 0x1a7)
			|| !smbbufferputv(s->transaction.out.data, cbo)
			|| !smbbufferputs(s->transaction.out.data, dosmode)
			|| !smbbufferputl(s->transaction.out.data, 0))	// alignment
			return SmbProcessResultMisc;
		fnlfixupoffset = smbbufferwriteoffset(s->transaction.out.data);
		if (!smbbufferputl(s->transaction.out.data, 0)
		|| !smbbufferputstring(s->transaction.out.data, &s->peerinfo, SMB_STRING_REVPATH, filename)
		|| !smbbufferfixuprelativel(s->transaction.out.data, fnlfixupoffset))
			return SmbProcessResultMisc;
		break;
	case SMB_QUERY_FILE_STANDARD_INFO:
		if (!smbbufferputv(s->transaction.out.data, smbl2roundupvlong(d->length, smbglobals.l2allocationsize))
			|| !smbbufferputv(s->transaction.out.data, d->length)
			|| !smbbufferputl(s->transaction.out.data, 1)
			|| !smbbufferputb(s->transaction.out.data, 0)
			|| !smbbufferputb(s->transaction.out.data, (d->qid.type & QTDIR) != 0))
			return SmbProcessResultMisc;
		break;
	case SMB_QUERY_FILE_EA_INFO:
		if (!smbbufferputl(s->transaction.out.data, 0))
			return SmbProcessResultMisc;
		break;
	case SMB_QUERY_FILE_STREAM_INFO:
		/* don't do it, never will */
		goto unknownlevel;
	default:
		smblogprint(-1, "smbtrans2query%sinformation: infolevel 0x%.4ux not implemented\n", cmdname, infolevel);
	unknownlevel:
		smbseterror(s, ERRDOS, ERRunknownlevel);
		return SmbProcessResultError;
	}
	return SmbProcessResultReply;
}

SmbProcessResult
smbtrans2querypathinformation(SmbSession *s, SmbHeader *h)
{
	SmbTree *t;
	SmbBuffer *b = nil;
	SmbProcessResult pr;
	ushort infolevel;
	Dir *d;
	char *path = nil;
	char *fullpath;

	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		pr = SmbProcessResultError;
		goto done;
	}
	b = smbbufferinit(s->transaction.in.parameters, s->transaction.in.parameters, s->transaction.in.tpcount);
	if (!smbbuffergets(b, &infolevel) || !smbbuffergetbytes(b, nil, 4)
		|| !smbbuffergetstring(b, h, SMB_STRING_PATH, &path)) {
		pr = SmbProcessResultMisc;
		goto done;
	}
	smblogprintif(smbglobals.log.query, "infolevel 0x%.4ux\n", infolevel);
	smblogprintif(smbglobals.log.query, "path %s\n", path);
	fullpath = nil;
	smbstringprint(&fullpath, "%s%s", t->serv->path, path);
	smblogprintif(smbglobals.log.query, "fullpath %s\n", fullpath);
	d = dirstat(fullpath);
	pr = query(s, "path", path, infolevel, 0, d);
	free(d);
	free(fullpath);
done:
	free(path);
	smbbufferfree(&b);
	return pr;
}

SmbProcessResult
smbtrans2queryfileinformation(SmbSession *s, SmbHeader *h)
{
	SmbTree *t;
	SmbFile *f;
	SmbBuffer *b = nil;
	SmbProcessResult pr;
	ushort fid;
	ushort infolevel;
	Dir *d;

	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		pr = SmbProcessResultError;
		goto done;
	}
	b = smbbufferinit(s->transaction.in.parameters, s->transaction.in.parameters, s->transaction.in.tpcount);
	if (!smbbuffergets(b, &fid) || !smbbuffergets(b, &infolevel)) {
		pr = SmbProcessResultMisc;
		goto done;
	}
	smblogprintif(smbglobals.log.query, "fid 0x%.4ux\n", fid);
	smblogprintif(smbglobals.log.query, "infolevel 0x%.4ux\n", infolevel);
	f = smbidmapfind(s->fidmap, fid);
	if (f == nil) {
		smbseterror(s, ERRDOS, ERRbadfid);
		pr = SmbProcessResultError;
		goto done;
	}
	d = dirfstat(f->fd);
	pr = query(s, "file", f->name, infolevel, seek(f->fd, 0, 1), d);
	free(d);
done:
	smbbufferfree(&b);
	return pr;
}

SmbProcessResult
smbtrans2queryfsinformation(SmbSession *s, SmbHeader *h)
{
	SmbTree *t;
	ushort infolevel;
	SmbBuffer *b;
	SmbProcessResult pr;
	ulong fixup;
	ulong vnbase;

	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		pr = SmbProcessResultError;
		goto done;
	}
	b = smbbufferinit(s->transaction.in.parameters, s->transaction.in.parameters, s->transaction.in.tpcount);
	if (!smbbuffergets(b, &infolevel)) {
	misc:
		pr = SmbProcessResultMisc;
		goto done;
	}
	pr = SmbProcessResultReply;
	switch (infolevel) {
	case SMB_INFO_ALLOCATION:
		if (!smbbufferputl(s->transaction.out.data, 0)
			|| !smbbufferputl(s->transaction.out.data, 1 << (smbglobals.l2allocationsize - smbglobals.l2sectorsize))
			|| !smbbufferputl(s->transaction.out.data, 0xffffffff)
			|| !smbbufferputl(s->transaction.out.data, 0xffffffff)
			|| !smbbufferputs(s->transaction.out.data, 1 << smbglobals.l2sectorsize))
			goto misc;
		break;
	case SMB_INFO_VOLUME:
		if (!smbbufferputl(s->transaction.out.data, 0xdeadbeef)
			|| !smbbufferputstring(s->transaction.out.data, &s->peerinfo, 0, t->serv->name))
			goto misc;
		break;
	case SMB_QUERY_FS_VOLUME_INFO:
		if (!smbbufferputv(s->transaction.out.data, 0)
			|| !smbbufferputl(s->transaction.out.data, 0xdeadbeef))
			goto misc;
		fixup = smbbufferwriteoffset(s->transaction.out.data);
		if (!smbbufferputl(s->transaction.out.data, 0)
			|| !smbbufferputs(s->transaction.out.data, 0))
			goto misc;
		vnbase = smbbufferwriteoffset(s->transaction.out.data);
		if (!smbbufferputstring(s->transaction.out.data, &s->peerinfo, 0, t->serv->name)
			|| !smbbufferfixupl(s->transaction.out.data, fixup,
				smbbufferwriteoffset(s->transaction.out.data) - vnbase))
			goto misc;
		break;
	case SMB_QUERY_FS_SIZE_INFO:
		if (!smbbufferputv(s->transaction.out.data, 0xffffffffffffffffLL)
			|| !smbbufferputv(s->transaction.out.data, 0xffffffffffffffffLL)
			|| !smbbufferputl(s->transaction.out.data, 1 << (smbglobals.l2allocationsize - smbglobals.l2sectorsize))
			|| !smbbufferputl(s->transaction.out.data, 1 << smbglobals.l2sectorsize))
			goto misc;
		break;
	case SMB_QUERY_FS_ATTRIBUTE_INFO:
//print("doing attribute info\n");
		if (!smbbufferputl(s->transaction.out.data, 3)
			|| !smbbufferputl(s->transaction.out.data, 255))
			goto misc;
		fixup = smbbufferwriteoffset(s->transaction.out.data);
		if (!smbbufferputl(s->transaction.out.data, 0)
			|| !smbbufferputstring(s->transaction.out.data, &s->peerinfo, SMB_STRING_UNTERMINATED, smbglobals.serverinfo.nativelanman)
			|| !smbbufferfixuprelativel(s->transaction.out.data, fixup))
			goto misc;
//print("done attribute info\n");
		break;
	default:
		smblogprint(-1, "smbtrans2queryfsinformation: infolevel 0x%.4ux not implemented\n", infolevel);
		smbseterror(s, ERRDOS, ERRunknownlevel);
		pr = SmbProcessResultError;
	}
done:
	smbbufferfree(&b);
	return pr;
}
