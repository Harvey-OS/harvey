/*
 * wrap/update - create software package and updates
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include <libsec.h>
#include "wrap.h"

char *root = "/";
char *desc;
char *proto;
int verbose;
int update;
int debug;
int listonly;

Wrap *w;
char **prefix;
int nprefix;

Biobuf bmd5;
Biobuf barch;
Biobuf bstdout;

static void
mkarch(char *new, char *old, Dir *d, void*)
{
	uchar digest[MD5dlen];
	vlong t;

	if(match(new, prefix, nprefix) == 0)
		return;

	if(md5file(old, digest) < 0)
		sysfatal("cannot open %s: %r", old);
	if(getfileinfo(w, new, &t, digest, nil) >= 0)
		return;

	if(listonly) {
		if((d->mode & CHDIR) == 0) 
			Bprint(&bstdout, "%s\n", new);
		return;
	}

	Bputhdr(&barch, new, d);
	if((d->mode & CHDIR) == 0) {
		Bprint(&bmd5, "%s %M\n", new, digest);
		if(Bcopyfile(&barch, old, d->length) < 0)
			sysfatal("error copying %lld %s: %r\n", d->length, old);
	}
}

void
usage(void)
{
	fprint(2, "usage: wrap/create [-d desc] [-p proto] [-r root] [-t time] [-lv] name\n");
	fprint(2, "or  wrap/create -u [-d desc] [-p proto] [-r root] [-t time] [-lv] old-package [prefix...]\n");
	exits("usage");
}

/*
 * creating an archive goes through the following steps:
 *
 *	process proto, writing md5 sums to /tmp/wrap.md5.* and
 *		the archive to /tmp/wrap.arch.*.
 *	sort /tmp/wrap.md5.* into /tmp/wrap.md5sort.*
 *	write an archive to stdout that begins with the md5 list
 *		and other wrap files, and then continues with /tmp/wrap.arch.*.
 *
 * the pain of making sure the wrap metadata is at the beginning
 * of the archive means that we can access it in gzipped archives without
 * needing to gunzip the whole thing.
 *
 * we used to use an algorithm that processed the proto twice, once
 * to create the md5 sum file and once to create the archive.
 * it's much better to copy the archive to a temporary file since you
 * can put it in ramfs, you avoid walking the entire tree again,
 * and prefetching can help you if you're not using ramfs.
 */
void
main(int argc, char **argv)
{
	char *name, md5file[2*NAMELEN], md5sort[2*NAMELEN], tmparch[2*NAMELEN],
		rmfile[2*NAMELEN], *p, *q;
	Biobuf bout;
	vlong t, archlen;
	int archfd, fd;

	t = time(0);
	ARGBEGIN{
	case 'D':
		debug = 1;
		break;
	case 'd':
		desc = ARGF();
		break;
	case 'l':
		listonly = 1;
		break;
	case 'p':
		proto = ARGF();
		break;
	case 'r':
		root = ARGF();
		break;
	case 't':
		t = strtoll(ARGF(), 0, 10);
		break;
	case 'u':
		update = 1;
		break;
	case 'v':
		verbose = 1;
		break;
	default:
		usage();
	}ARGEND

	rfork(RFNAMEG);
	fmtinstall('M', md5conv);

	if(!update && argc != 1)
		usage();

	if(update) {
		prefix = argv+1;
		nprefix = argc-1;
	}

	if(update) { 
		w = openwraphdr(argv[0], root, nil, 0);
		if(w == nil)
			sysfatal("no such package found");

		/* ignore any updates */
		while(w->nu > 0 && w->u[w->nu-1].type == UPD)
			w->nu--;

		if(proto == nil)
			proto = mkpath(w->u->dir, "proto");
		name = w->name;
	} else {
		name = argv[0];
		if(strlen(name) >= NAMELEN)
			sysfatal("name '%s' too long", name);
		if(proto == nil)
			proto = "/sys/lib/sysconfig/proto/allproto";
	}

	if(access(proto, AREAD) < 0)
		sysfatal("cannot read proto: %r");

	if(listonly) {
		Binit(&bstdout, 1, OWRITE);
		if(rdproto(proto, root, mkarch, nil, nil) < 0)
			sysfatal("rdproto: %r");
		Bterm(&bstdout);
		exits(nil);
	}
	
	strcpy(md5file, "/tmp/wrap.md5.XXXXXX");
	fd = opentemp(md5file);
	Binit(&bmd5, fd, OWRITE);

	strcpy(tmparch, "/tmp/wrap.arch.XXXXXX");
	archfd = opentemp(tmparch);
	Binit(&barch, archfd, OWRITE);

	if(rdproto(proto, root, mkarch, nil, nil) < 0)
		sysfatal("rdproto: %r");
	Bflush(&bmd5);
	Bterm(&bmd5);
	Bflush(&barch);
	archlen = Boffset(&barch);
	Bterm(&barch);

	strcpy(md5sort, "/tmp/wrap.md5sort.XXXXXX");
	fd = opentemp(md5sort);
	fsort(fd, md5file);

	if(update) {
		strcpy(rmfile, "/tmp/wrap.rm.XXXXXX");
		fd = opentemp(rmfile);
		Bseek(w->u->bmd5, 0, 0);
		while(p = Bgetline(w->u->bmd5)) {
			if(match(p, prefix, nprefix) == 0)
				continue;
			if(q = strchr(p, ' '))
				*q = 0;
			q = mkpath(root, p);
			if(access(q, 0) < 0)
				fprint(fd, "%s\n", p);
			free(q);
			free(p);
		}
	}

	Binit(&bout, 1, OWRITE);
	if(update)
		Bputwrap(&bout, name, t, desc, w->tfull, nprefix==0);
	else
		Bputwrap(&bout, name, t, desc, 0, 1);
	Bputwrapfile(&bout, name, t, "proto", proto);
	Bputwrapfile(&bout, name, t, "md5sum", md5sort);
	if(update)
		Bputwrapfile(&bout, name, t, "remove", rmfile);

	seek(archfd, 0, 0);	/* Binit assumes this */
	Binit(&barch, archfd, OREAD);
	if(Bcopy(&bout, &barch, archlen) < 0)
		sysfatal("copy of temp file did not succeed: %lld %lld", Boffset(&barch), archlen);

	Bprint(&bout, "end of archive\n");
	Bterm(&bout);
	exits(0);
}
