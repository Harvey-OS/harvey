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

Wrap *w;
char **prefix;
int nprefix;
int pass;

/*
 * We need to make sure that the files we list
 * in the md5sum file are the same files we
 * archive in the second pass through the proto
 * file; in order to do this without actually
 * keeping a list of files (which would defeat
 * the point of just processing the proto a 
 * second time), we run the list of file names
 * through MD5 and save the digests; the 
 * digest from pass 1 and the digest from pass 2
 * must match.
 */
int pass;
DigestState *md5s;

static void
mkarch(char *new, char *old, Dir *d, void *aux)
{
	Biobuf *b;
	uchar digest[MD5dlen], odigest[MD5dlen];
	vlong t;

	if(match(new, prefix, nprefix) == 0)
		return;

	if(md5file(old, digest) < 0)
		sysfatal("cannot open %s: %r", old);

	memset(odigest, 0xFF, MD5dlen);
	if(getfileinfo(w, new, &t, odigest) >= 0
	&& memcmp(digest, odigest, MD5dlen) == 0)
		return;

	md5((uchar*)new, strlen(new), nil, md5s);
	b = aux;
	switch(pass) {
	default:
		assert(0);
	case 1:
		Bprint(b, "%s %M\n", new, digest);
		break;
	case 2:
		Bputhdr(b, new, d);
		if((d->mode & CHDIR) == 0)
			if(Bcopyfile(b, old, d->length) < 0)
				sysfatal("error copying %s\n", old);
		break;
	}
}

void
usage(void)
{
	fprint(2, "usage: wrap/create [-d desc] [-p proto] [-r root] [-t time] [-v] name\n");
	fprint(2, "or  wrap/create -u [-d desc] [-p proto] [-r root] [-t time] [-v] package [prefix...]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *name, md5file[2*NAMELEN], md5sort[2*NAMELEN],
		rmfile[2*NAMELEN], *p, *q;
	uchar digest[MD5dlen], digest0[MD5dlen];
	Biobuf bout;
	vlong t;
	int fd;

	t = time(0);
	ARGBEGIN{
	case 'd':
		desc = ARGF();
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

	if((update && argc < 1) || (!update && argc != 1))
		usage();

	if(update) { 
		w = openwraphdr(argv[0], root, argv+1, argc-1);
		if(w == nil)
			sysfatal("no such package found");
		w->nu = 1;	/* ignore any updates */
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

	strcpy(md5file, "/tmp/wrap.md5.XXXXXX");
	fd = opentemp(md5file);

	md5s = md5(nil, 0, nil, nil);
	Binit(&bout, fd, OWRITE);
	pass = 1;
	if(rdproto(proto, root, mkarch, nil, &bout) < 0)
		sysfatal("rdproto: %r");
	Bflush(&bout);
	Bterm(&bout);
	md5(nil, 0, digest, md5s);
	md5s = nil;

	strcpy(md5sort, "/tmp/wrap.md5.XXXXXX");
	fd = opentemp(md5sort);
	fsort(fd, md5file);

	if(update) {
		strcpy(rmfile, "/tmp/wrap.rm.XXXXXX");
		fd = opentemp(rmfile);
		Bseek(w->u->bmd5, 0, 0);
		while(p = Bgetline(w->u->bmd5)) {
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
		Bputwrap(&bout, name, t, desc, w->u->time);
	else
		Bputwrap(&bout, name, t, desc, 0);
	Bputwrapfile(&bout, name, t, "proto", proto);
	Bputwrapfile(&bout, name, t, "md5sum", md5sort);
	if(update)
		Bputwrapfile(&bout, name, t, "remove", rmfile);

	pass = 2;
	md5s = md5(nil, 0, nil, nil);
	if(rdproto(proto, root, mkarch, nil, &bout) < 0)
		sysfatal("%r");
	md5(nil, 0, digest0, md5s);
	md5s = nil;

	if(memcmp(digest, digest0, MD5dlen) != 0)
		sysfatal("files changed underfoot between passes %M %M", digest, digest0);
	Bprint(&bout, "end of archive\n");
	Bterm(&bout);
	exits(0);
}
