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

SmbProcessResult
smbcomwriteandx(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *b)
{
	uchar andxcommand;
	ushort andxoffset;
	ulong andxoffsetfixup;
	SmbTree *t;
	SmbFile *f;
	ushort dataoff, fid, count;
	vlong offset;
	long nb;

	if (h->wordcount != 12 && h->wordcount != 14)
		return SmbProcessResultFormat;

	andxcommand = *pdata++;				// andx command
	pdata++;					// reserved 
	andxoffset = smbnhgets(pdata); pdata += 2;	// andx offset
	fid = smbnhgets(pdata); pdata += 2;		// fid
	offset = smbnhgetl(pdata); pdata += 4;		// offset in file
	pdata += 4;					// timeout
	pdata += 2;					// write mode
	pdata += 2;					// (Remaining) bytes waiting to be written
	pdata += 2;					// Reserved
	count = smbnhgets(pdata); pdata += 2;		// LSBs of length
	dataoff = smbnhgets(pdata); pdata += 2;		// offset to data in packet
	if (dataoff + count > smbbufferwriteoffset(b))
		return SmbProcessResultFormat;
	if(h->wordcount == 14)
		offset |= (vlong)smbnhgetl(pdata)<<32;

	smblogprint(SMB_COM_WRITE_ANDX, "smbcomwriteandx: fid 0x%.4ux count 0x%.4ux offset 0x%.llux\n",
		fid, count, offset);


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

	seek(f->fd, offset, 0);
	nb = write(f->fd, smbbufferpointer(b, dataoff), count);
	if (nb < 0) {
		smbseterror(s, ERRDOS, ERRnoaccess);
		return SmbProcessResultError;
	}

	h->wordcount = 6;
	if (!smbbufferputandxheader(s->response, h, &s->peerinfo, andxcommand, &andxoffsetfixup))
		return SmbProcessResultMisc;

	if (!smbbufferputs(s->response, nb)			// Count
		|| !smbbufferputs(s->response, 0)		// Available
		|| !smbbufferputl(s->response, 0)		// Reserved
		|| !smbbufferputs(s->response, 0))		// byte count in reply
		return SmbProcessResultMisc;

	if (andxcommand != SMB_COM_NO_ANDX_COMMAND)
		return smbchaincommand(s, h, andxoffsetfixup, andxcommand, andxoffset, b);

	return SmbProcessResultReply;
}
