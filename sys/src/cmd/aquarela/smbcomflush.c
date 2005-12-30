#include "headers.h"
#include <String.h>

SmbProcessResult
smbcomflush(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *)
{
	SmbTree *t;
	SmbFile *f;
	ushort fid;
	Dir nulldir;
	if (h->wordcount != 1)
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
	memset(&nulldir, 0xff, sizeof(nulldir));
	nulldir.name = nulldir.uid = nulldir.gid = nulldir.muid = nil;
	dirfwstat(f->fd, &nulldir);
	return smbbufferputack(s->response, h, &s->peerinfo);
}
