#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>

static void
enm(char *new, char *old, Dir *d, void*)
{
	print("%s %s%s%s%luo %s %s %s\n",
		new, (d->mode&DMDIR)?"d":"", (d->mode&DMAPPEND)?"a":"",
		(d->mode&DMEXCL)?"l":"", (d->mode&~(DMDIR|DMAPPEND|DMEXCL)), 
		d->uid, d->gid, old);
}

static void
usage(void)
{
	fprint(2, "usage: protodump [-r root] proto\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *root;

	root = "/";
	ARGBEGIN{
	case 'r':
		root = EARGF(usage());
		break;
	}ARGEND

	if(argc != 1)
		usage();

	if(revrdproto(argv[0], root, enm, nil, nil) < 0)
		sysfatal("rdproto: %r");
	exits(nil);
}
