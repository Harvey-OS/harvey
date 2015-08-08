/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

#define INMEMORYTRUNCTHRESH (256 * 1024)

static int
dirfwstatlength(int fd, int64_t offset)
{
	Dir d;
	memset(&d, 0xff, sizeof(d));
	d.name = d.uid = d.gid = d.muid = nil;
	d.length = offset;
	return dirfwstat(fd, &d);
}

SmbProcessResult
smbtruncatefile(SmbSession* s, SmbFile* f, int64_t offset)
{
	Dir* d;
	uint32_t o;
	uint8_t* db = nil;
	int64_t length;
	int rv;
	SmbProcessResult pr;

	d = dirfstat(f->fd);
	assert(d);
	length = d->length;
	free(d);

	if(length == offset)
		return SmbProcessResultReply;

	rv = dirfwstatlength(f->fd, offset);
	if(rv == 0) {
		pr = SmbProcessResultReply;
		goto done;
	}
	// smblogprint(-1, "dirfwstatlength failed: %r\n");
	if(length > offset) {
		int nfd;
		char* fullpath;
		if(offset > INMEMORYTRUNCTHRESH) {
			smblogprint(-1, "smbcomwrite: truncation beyond %lud "
			                "not supported\n",
			            offset);
			pr = SmbProcessResultUnimp;
			goto done;
		}
		db = smbemalloc(offset);
		if(pread(f->fd, db, offset, 0) != offset) {
			pr = SmbProcessResultMisc;
			goto done;
		}
		fullpath = nil;
		smbstringprint(&fullpath, "%s%s", f->t->serv->path, f->name);
		nfd = open(fullpath, f->p9mode | OTRUNC);
		free(fullpath);
		if(nfd < 0) {
			smbseterror(s, ERRDOS, ERRnoaccess);
			pr = SmbProcessResultError;
			goto done;
		}
		close(nfd);
		if(pwrite(f->fd, db, offset, 0) != offset) {
			pr = SmbProcessResultMisc;
			goto done;
		}
		pr = SmbProcessResultReply;
	} else {
		db = smbemalloc(16384);
		memset(db, 0, 16384);
		o = length;
		while(o < offset) {
			int32_t tt = 16384;
			if(tt > offset - o)
				tt = offset - o;
			if(pwrite(f->fd, db, tt, o) != tt) {
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
smbcomwrite(SmbSession* s, SmbHeader* h, uint8_t* pdata, SmbBuffer* b)
{
	SmbTree* t;
	SmbFile* f;
	uint16_t fid;
	uint16_t count;
	uint32_t offset;
	int32_t nb;
	uint16_t yacount;
	uint8_t fmt;

	if(h->wordcount != 5)
		return SmbProcessResultFormat;

	fid = smbnhgets(pdata);
	pdata += 2;
	count = smbnhgets(pdata);
	pdata += 2;
	offset = smbnhgetl(pdata);

	smblogprint(SMB_COM_WRITE,
	            "smbcomwrite: fid 0x%.4ux count 0x%.4ux offset 0x%.8lux\n",
	            fid, count, offset);

	if(!smbbuffergetb(b, &fmt) || fmt != 1 || !smbbuffergets(b, &yacount) ||
	   yacount != count || smbbufferreadspace(b) < count)
		return SmbProcessResultFormat;

	t = smbidmapfind(s->tidmap, h->tid);
	if(t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		return SmbProcessResultError;
	}
	f = smbidmapfind(s->fidmap, fid);
	if(f == nil) {
		smbseterror(s, ERRDOS, ERRbadfid);
		return SmbProcessResultError;
	}

	if(!f->ioallowed) {
		smbseterror(s, ERRDOS, ERRbadaccess);
		return SmbProcessResultError;
	}

	if(count == 0) {
		SmbProcessResult pr = smbtruncatefile(s, f, offset);
		if(pr != SmbProcessResultReply)
			return pr;
		nb = 0;
	} else {
		seek(f->fd, offset, 0);
		nb = write(f->fd, smbbufferreadpointer(b), count);
		if(nb < 0) {
			smbseterror(s, ERRDOS, ERRnoaccess);
			return SmbProcessResultError;
		}
	}
	h->wordcount = 1;
	if(!smbbufferputheader(s->response, h, &s->peerinfo) ||
	   !smbbufferputs(s->response, nb) || !smbbufferputs(s->response, 0))
		return SmbProcessResultMisc;
	return SmbProcessResultReply;
}

SmbProcessResult
smbcomwriteandx(SmbSession* s, SmbHeader* h, uint8_t* pdata, SmbBuffer* b)
{
	uint8_t andxcommand;
	uint16_t andxoffset;
	uint32_t andxoffsetfixup;
	SmbTree* t;
	SmbFile* f;
	uint16_t dataoff, fid, count;
	int64_t offset;
	int32_t nb;

	if(h->wordcount != 12 && h->wordcount != 14)
		return SmbProcessResultFormat;

	andxcommand = *pdata++; // andx command
	pdata++;                // reserved
	andxoffset = smbnhgets(pdata);
	pdata += 2; // andx offset
	fid = smbnhgets(pdata);
	pdata += 2; // fid
	offset = smbnhgetl(pdata);
	pdata += 4; // offset in file
	pdata += 4; // timeout
	pdata += 2; // write mode
	pdata += 2; // (Remaining) bytes waiting to be written
	pdata += 2; // Reserved
	count = smbnhgets(pdata);
	pdata += 2; // LSBs of length
	dataoff = smbnhgets(pdata);
	pdata += 2; // offset to data in packet
	if(dataoff + count > smbbufferwriteoffset(b))
		return SmbProcessResultFormat;
	if(h->wordcount == 14)
		offset |= (int64_t)smbnhgetl(pdata) << 32;

	smblogprint(
	    SMB_COM_WRITE_ANDX,
	    "smbcomwriteandx: fid 0x%.4ux count 0x%.4ux offset 0x%.llux\n", fid,
	    count, offset);

	t = smbidmapfind(s->tidmap, h->tid);
	if(t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		return SmbProcessResultError;
	}
	f = smbidmapfind(s->fidmap, fid);
	if(f == nil) {
		smbseterror(s, ERRDOS, ERRbadfid);
		return SmbProcessResultError;
	}

	if(!f->ioallowed) {
		smbseterror(s, ERRDOS, ERRbadaccess);
		return SmbProcessResultError;
	}

	seek(f->fd, offset, 0);
	nb = write(f->fd, smbbufferpointer(b, dataoff), count);
	if(nb < 0) {
		smbseterror(s, ERRDOS, ERRnoaccess);
		return SmbProcessResultError;
	}

	h->wordcount = 6;
	if(!smbbufferputandxheader(s->response, h, &s->peerinfo, andxcommand,
	                           &andxoffsetfixup))
		return SmbProcessResultMisc;

	if(!smbbufferputs(s->response, nb)    // Count
	   || !smbbufferputs(s->response, 0)  // Available
	   || !smbbufferputl(s->response, 0)  // Reserved
	   || !smbbufferputs(s->response, 0)) // byte count in reply
		return SmbProcessResultMisc;

	if(andxcommand != SMB_COM_NO_ANDX_COMMAND)
		return smbchaincommand(s, h, andxoffsetfixup, andxcommand,
		                       andxoffset, b);

	return SmbProcessResultReply;
}
