#include "headers.h"

SmbProcessResult
smbcomsetinformation2(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *)
{
	ushort fid, adate, atime, mdate, mtime;
	SmbTree *t;
	SmbFile *f;
	Dir d;

	if (h->wordcount != 7)
		return SmbProcessResultFormat;
	fid = smbnhgets(pdata);
	adate = smbnhgets(pdata + 6);
	atime = smbnhgets(pdata + 8);
	mdate = smbnhgets(pdata + 10);
	mtime = smbnhgets(pdata + 12);
	smblogprint(h->command,
		"smbcomsetinformation2: fid 0x%.4ux adate 0x%.4ux atime 0x%.4ux mdate 0x%.4ux mtime 0x%.4ux\n",
		fid, adate, atime, mdate, mtime);
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
	memset(&d, 0xff, sizeof(d));
	d.name = d.uid = d.gid = d.muid = nil;
	if (adate || atime || mdate || mtime) {
//smblogprint(-1, "smbcomsetinformation2: changing times not implemented\n");
//		return SmbProcessResultUnimp;
		/* something to change */
		if (!(adate && atime && mdate && mtime)) {
			/* some null entries */
			ushort odate, otime;
			Dir *od = dirfstat(f->fd);
			if (od == nil) {
				smbseterror(s, ERRDOS, ERRnoaccess);
				return SmbProcessResultError;
			}
			if (adate || atime) {
				/* something changed in access time */
				if (!(adate && atime)) {
					/* some nulls in access time */
					smbplan9time2datetime(d.atime, s->tzoff, &odate, &otime);
					if (adate == 0)
						adate = odate;
					if (atime == 0)
						atime = otime;
				}
				d.atime = smbdatetime2plan9time(adate, atime, s->tzoff);
			}
			if (mdate || mtime) {
				/* something changed in modify time */
				if (!(mdate && mtime)) {
					/* some nulls in modify time */
					smbplan9time2datetime(d.mtime, s->tzoff, &odate, &otime);
					if (mdate == 0)
						mdate = odate;
					if (mtime == 0)
						mtime = otime;
				}
				d.mtime = smbdatetime2plan9time(mdate, mtime, s->tzoff);
			}
			free(od);
		}
		if (dirfwstat(f->fd, &d) < 0) {
			smbseterror(s, ERRDOS, ERRnoaccess);
			return SmbProcessResultError;
		}
	}
	return smbbufferputack(s->response, h, &s->peerinfo);
}

SmbProcessResult
smbcomsetinformation(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *b)
{
	ushort attr;
	ulong utime;
	char *name;
	if (h->wordcount != 8)
		return SmbProcessResultFormat;
	attr = smbnhgets(pdata); pdata += 2;
	utime = smbnhgetl(pdata);
	if (!smbbuffergetstring(b, h, SMB_STRING_PATH, &name))
		return SmbProcessResultFormat;
	smblogprint(h->command,
		"smbcomsetinformation: attr 0x%.4ux utime %lud path %s\n",
		attr, utime, name);
	if (utime) {
		Dir d;
		memset(&d, 0xff, sizeof(d));
		d.name = d.uid = d.gid = d.muid = nil;
		d.mtime = smbutime2plan9time(utime, s->tzoff);
		if (dirwstat(name, &d) < 0) {
			smbseterror(s, ERRDOS, ERRnoaccess);
			free(name);
			return SmbProcessResultError;
		}
	}
	free(name);		
	return smbbufferputack(s->response, h, &s->peerinfo);
}
