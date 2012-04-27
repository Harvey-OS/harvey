#include "headers.h"
#include <String.h>

int
smbremovefile(SmbTree *t, char *dir, char *name)
{
	String *s;
	int rv;
	s = s_new();
	s_append(s, t->serv->path);
	s_append(s, "/");
	if (dir) {
		s_append(s, dir);
		s_append(s, "/");
	}
	s_append(s, name);
	rv = remove(s_to_c(s));
	s_free(s);
	return rv;
}

SmbProcessResult
smbcomdelete(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *b)
{
	SmbProcessResult pr;
	ushort sattr;
	uchar fmt;
	char *pattern = nil;
	char *dir = nil;
	char *name = nil;
	Reprog *r = nil;
	SmbTree *t;
	int x, count;
	SmbDirCache *dc = nil;

	if (h->wordcount != 1)
		return SmbProcessResultFormat;
	sattr = smbnhgets(pdata);
	if (!smbbuffergetb(b, &fmt) || fmt != 0x04
		|| !smbbuffergetstring(b, h, SMB_STRING_PATH, &pattern))
		return SmbProcessResultFormat;
	smblogprint(SMB_COM_DELETE, "searchattributes: 0x%.4ux\npattern:%s\n", sattr, pattern);
	smbpathsplit(pattern, &dir, &name);
	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		pr = SmbProcessResultError;
		goto done;
	}
	dc = smbmkdircache(t, dir);
	if (dc == nil) {
		pr = SmbProcessResultMisc;
		goto done;
	}
	r = smbmkrep(name);
	count = 0;
	for (x = 0; x < dc->n; x++) {
		if (!smbmatch(dc->buf[x].name, r))
			continue;
		if (smbremovefile(t, dir, dc->buf[x].name) == 0)
			count++;
	}
	if (count == 0) {
		smbseterror(s, ERRDOS, ERRnoaccess);
		pr = SmbProcessResultError;
	}
	else
		pr = smbbufferputack(s->response,h, &s->peerinfo);
done:
	free(pattern);
	free(dir);
	free(name);
	smbdircachefree(&dc);
	free(r);
	return pr;
}
