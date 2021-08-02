#include <u.h>
#include <libc.h>

#include "git.h"

#define TMPPATH(suff) (".git/objects/pack/repack."suff)

int
cleanup(Hash h)
{
	char newpfx[42], dpath[256], fpath[256];
	int i, j, nd;
	Dir *d;

	snprint(newpfx, sizeof(newpfx), "%H.", h);
	for(i = 0; i < 256; i++){
		snprint(dpath, sizeof(dpath), ".git/objects/%02x", i);
		if((nd = slurpdir(dpath, &d)) == -1)
			continue;
		for(j = 0; j < nd; j++){
			snprint(fpath, sizeof(fpath), ".git/objects/%02x/%s", i, d[j].name);
			remove(fpath);
		}
		remove(dpath);
		free(d);
	}
	snprint(dpath, sizeof(dpath), ".git/objects/pack");
	if((nd = slurpdir(dpath, &d)) == -1)
		return -1;
	for(i = 0; i < nd; i++){
		if(strncmp(d[i].name, newpfx, strlen(newpfx)) == 0)
			continue;
		snprint(fpath, sizeof(fpath), ".git/objects/pack/%s", d[i].name);
		remove(fpath);
	}
	return 0;
}

void
usage(void)
{
	fprint(2, "usage: %s [-d]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	char path[128], **names;
	int fd, nrefs;
	Hash *refs, h;
	Dir rn;

	ARGBEGIN{
	case 'd':
		chattygit++;
		break;
	default:
		usage();
	}ARGEND;

	gitinit();
	refs = nil;
	if((nrefs = listrefs(&refs, &names)) == -1)
		sysfatal("load refs: %r");
	if((fd = create(TMPPATH("pack.tmp"), OWRITE, 0644)) == -1)
		sysfatal("open %s: %r", TMPPATH("pack.tmp"));
	if(writepack(fd, refs, nrefs, nil, 0, &h) == -1)
		sysfatal("writepack: %r");
	if(indexpack(TMPPATH("pack.tmp"), TMPPATH("idx.tmp"), h) == -1)
		sysfatal("indexpack: %r");
	close(fd);

	nulldir(&rn);
	rn.name = path;
	snprint(path, sizeof(path), "%H.pack", h);
	if(dirwstat(TMPPATH("pack.tmp"), &rn) == -1)
		sysfatal("rename pack: %r");
	snprint(path, sizeof(path), "%H.idx", h);
	if(dirwstat(TMPPATH("idx.tmp"), &rn) == -1)
		sysfatal("rename pack: %r");
	if(cleanup(h) == -1)
		sysfatal("cleanup: %r");
	exits(nil);
}
