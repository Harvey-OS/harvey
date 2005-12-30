#include "headers.h"

static void
smbfilefree(SmbFile **fp)
{
	SmbFile *f;
	f = *fp;
	if (f) {
		smbsharedfileput(f, f->sf, f->share);
		if (f->fd >= 0)
			close(f->fd);
		free(f->name);
		free(f);
		*fp = nil;
	}
}

void
smbfileclose(SmbSession *s, SmbFile *f)
{
	smblogprintif(smbglobals.log.fids, "smbfileclose: 0x%.4ux/0x%.4ux %s%s\n",
		f->t->id, f->id, f->t->serv->path, f->name);
	smbidmapremove(s->fidmap, f);
	smbfilefree(&f);
}
