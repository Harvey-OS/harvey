#include "headers.h"

SmbProcessResult
smbtrans2setfileinformation(SmbSession *s, SmbHeader *h)
{
	SmbTree *t;
	ushort infolevel;
	SmbBuffer *b;
	SmbProcessResult pr;
	ushort fid;
	SmbFile *f;
	vlong newsize;
	uvlong atime, mtime;
	ulong attr;
	ulong mode;

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
		smblogprint(-1, "smbtrans2setfileinformation: infolevel 0x%.4ux not implemented\n", infolevel);
		smbseterror(s, ERRDOS, ERRunknownlevel);
		pr = SmbProcessResultError;
		break;
	}
done:
	smbbufferfree(&b);
	return pr;
}
