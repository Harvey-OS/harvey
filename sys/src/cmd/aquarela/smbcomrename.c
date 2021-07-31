#include "headers.h"

SmbProcessResult
smbcomrename(SmbSession *s, SmbHeader *h, uchar *, SmbBuffer *b)
{
	int rv;
	char *oldpath, *newpath;
	char *olddir, *newdir;
	char *oldname, *newname;
	uchar oldfmt, newfmt;
	Dir d;
	SmbProcessResult pr;

	if (h->wordcount != 1)
		return SmbProcessResultFormat;
	if (!smbbuffergetb(b, &oldfmt) || oldfmt != 0x04 || !smbbuffergetstring(b, h, SMB_STRING_PATH, &oldpath)
		|| !smbbuffergetb(b, &newfmt) || newfmt != 0x04 || !smbbuffergetstring(b, h, SMB_STRING_PATH, &newpath))
		return SmbProcessResultFormat;
	smblogprint(h->command, "smbcomrename: %s to %s\n", oldpath, newpath);
	smbpathsplit(oldpath, &olddir, &oldname);
	smbpathsplit(newpath, &newdir, &newname);
	if (strcmp(olddir, newdir) != 0) {
		smblogprint(h->command, "smbcomrename: directories differ\n");
		goto noaccess;
	}
	memset(&d, 0xff, sizeof(d));
	d.uid = d.gid = d.muid = nil;
	d.name = newname;
	rv = dirwstat(oldpath, &d);
	if (rv < 0) {
		smblogprint(h->command, "smbcomrename failed: %r\n");
	noaccess:
		smbseterror(s, ERRDOS, ERRnoaccess);
		pr =  SmbProcessResultError;
	}
	else
		pr = smbbufferputack(s->response, h, &s->peerinfo);
	free(oldpath);
	free(olddir);
	free(oldname);
	free(newpath);
	free(newdir);
	free(newname);
	return pr;
}
