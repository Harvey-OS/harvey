#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include "wrap.h"

Arch*
openarch(char *file)
{
	Arch *a;
	Biobuf *b;
	Dir d;

	if((b = Bopengz(file)) == nil)
		return nil;

	a = emalloc(sizeof(*a));
	a->b = b;

	/* all we care is that it is not a pipe from gzip */
	if(dirfstat(b->fid, &d) >= 0 && d.type == 'M')
		a->canseek = 1;

	return a;
}

Ahdr*
gethdr(Arch *arch)
{
	long m;
	char *p, *f[10];
	Ahdr *a;

	a = &arch->hdr;
	if(a->name) {
		free(a->name);
		a->name = nil;
	}

	m = Boffset(arch->b);

	if(arch->canseek)
		Bseek(arch->b, m, 0);
	else {
		assert(m <= arch->nexthdr);
		if(Bdrain(arch->b, arch->nexthdr - m) < 0) {
			return nil;
		}
	}

	if((p = Bgetline(arch->b)) == nil)
		return nil;

	if(strcmp(p, "end of archive") == 0) {
		free(p);
		werrstr("");
		return nil;
	}

	if(tokenize(p, f, nelem(f)) != 6 || f[0] != p) {
		free(p);
		werrstr("bad format");
		return nil;
	}

	a->name = f[0];
	a->mode = strtoul(f[1], 0, 8);
	strcpy(a->uid, f[2]);
	strcpy(a->gid, f[3]);
	a->mtime = strtoll(f[4], 0, 10);
	a->length = strtoll(f[5], 0, 10);
	arch->nexthdr = Boffset(arch->b) + a->length;
	return a;
}

void
closearch(Arch *a)
{
	Bterm(a->b);
	free(a);
}

void
Bputhdr(Biobuf *b, char *name, Dir *d)
{
	Bprint(b, "%s %luo %s %s %lud %lld\n", name,
		d->mode, d->uid, d->gid, d->mtime, d->length);
}

Arch*
openarchgz(char *file, char **newfilep, char **prefix, int nprefix)
{
	int fd, hdrs;
	Biobuf *btmp;
	char *newfile;
	Ahdr *a;
	Arch *arch;

	hdrs = nprefix == -1;
	if((arch = openarch(file)) == nil)
		return nil;

	if(arch->canseek) {
		*newfilep = file;
		return arch;
	}

	newfile = estrdup("/tmp/wrap.gz.XXXXXX");
	fd = opentemp(newfile);

	btmp = emalloc(sizeof(*btmp)); 	/* so we can return it */
	Binit(btmp, fd, OWRITE);
	btmp->flag = Bmagic;	/* so Bterm will free and close(fd) */

	while(a = gethdr(arch)) {
		if(hdrs && strcmp(a->name, "/wrap") != 0 && strprefix(a->name, "/wrap/") != 0) {
			werrstr("");
			break;
		}
		if(match(a->name, prefix, nprefix)) {
			Bputhdr(btmp, a->name, a);
			if(Bcopy(btmp, arch->b, a->length) < 0) {
				Bterm(btmp);
				closearch(arch);
				return nil;
			}
		}
	}
	if(waserrstr()) {
		Bterm(btmp);
		closearch(arch);
		return nil;
	}
	closearch(arch);

	Bprint(btmp, "end of archive\n");
	Bflush(btmp);

	Binit(btmp, fd, OREAD);
	arch = emalloc(sizeof(*arch));
	arch->b = btmp;
	arch->canseek = 1;
	if(newfilep)
		*newfilep = newfile;
	return arch;
}
