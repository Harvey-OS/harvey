#include "headers.h"

#define INMEMORYTRUNCTHRESH (256 * 1024)

static int
dirfwstatlength(int fd, vlong offset)
{
	Dir d;
	memset(&d, 0xff, sizeof(d));
	d.name = d.uid = d.gid = d.muid = nil;
	d.length = offset;
	return dirfwstat(fd, &d);
}

SmbProcessResult
smbtruncatefile(SmbSession *s, SmbFile *f, vlong offset)
{
	Dir *d;
	ulong o;
	uchar *db = nil;
	vlong length;
	int rv;
	SmbProcessResult pr;

	d = dirfstat(f->fd);
	assert(d);
	length = d->length;
	free(d);

	if (length == offset)
		return SmbProcessResultReply;

	rv = dirfwstatlength(f->fd, offset);
	if (rv == 0) {
		pr = SmbProcessResultReply;
		goto done;
	}
//smblogprint(-1, "dirfwstatlength failed: %r\n");
	if (length > offset) {
		int nfd;
		char *fullpath;
		if (offset > INMEMORYTRUNCTHRESH) {
			smblogprint(-1, "smbcomwrite: truncation beyond %lud not supported\n", offset);
			pr = SmbProcessResultUnimp;
			goto done;
		}
		db = smbemalloc(offset);
		if (pread(f->fd, db, offset, 0) != offset) {
			pr = SmbProcessResultMisc;
			goto done;
		}
		fullpath = nil;
		smbstringprint(&fullpath, "%s%s", f->t->serv->path, f->name);
		nfd = open(fullpath, f->p9mode | OTRUNC);
		free(fullpath);
		if (nfd < 0) {
			smbseterror(s, ERRDOS, ERRnoaccess);
			pr = SmbProcessResultError;
			goto done;
		}
		close(nfd);
		if (pwrite(f->fd, db, offset, 0) != offset) {
			pr = SmbProcessResultMisc;
			goto done;
		}
		pr = SmbProcessResultReply;
	}
	else {
		db = smbemalloc(16384);
		memset(db, 0, 16384);
		o = length;
		while (o < offset) {
			long tt = 16384;
			if (tt > offset - o)
				tt = offset - o;
			if (pwrite(f->fd, db, tt, o) != tt) {
				smbseterror(s, ERRDOS, ERRnoaccess);
				pr = SmbProcessResultError;
				goto done;
			}
			o += tt;
		}
		pr = SmbProcessResultReply;
	}
done:
	free(db);
	return pr;
}

SmbProcessResult
smbcomwrite(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *b)
{
	SmbTree *t;
	SmbFile *f;
	ushort fid;
	ushort count;
	ulong offset;
	long nb;
	ushort yacount;
	uchar fmt;

	if (h->wordcount != 5)
		return SmbProcessResultFormat;

	fid = smbnhgets(pdata); pdata += 2;
	count = smbnhgets(pdata); pdata += 2;
	offset = smbnhgetl(pdata);

	smblogprint(SMB_COM_WRITE, "smbcomwrite: fid 0x%.4ux count 0x%.4ux offset 0x%.8lux\n",
		fid, count, offset);

	if (!smbbuffergetb(b, &fmt)
		|| fmt != 1
		|| !smbbuffergets(b, &yacount)
		|| yacount != count
		|| smbbufferreadspace(b) < count)
		return SmbProcessResultFormat;
	
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
	
	if (!f->ioallowed) {
		smbseterror(s, ERRDOS, ERRbadaccess);
		return SmbProcessResultError;
	}

	if (count == 0) {
		SmbProcessResult pr = smbtruncatefile(s, f, offset);
		if (pr != SmbProcessResultReply)
			return pr;
		nb = 0;
	}
	else {
		seek(f->fd, offset, 0);
		nb = write(f->fd, smbbufferreadpointer(b), count);
		if (nb < 0) {
			smbseterror(s, ERRDOS, ERRnoaccess);
			return SmbProcessResultError;
		}
	}
	h->wordcount = 1;
	if (!smbbufferputheader(s->response, h, &s->peerinfo)
		|| !smbbufferputs(s->response, nb)
		|| !smbbufferputs(s->response, 0))
		return SmbProcessResultMisc;
	return SmbProcessResultReply;
}
