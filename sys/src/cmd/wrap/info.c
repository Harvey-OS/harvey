/*
 * wrap/info - print information about packages
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include <libsec.h>
#include "wrap.h"

char *root;

void
usage(void)
{
	fprint(2, "wrap/info [-r root] package\n");
	exits("usage");
}

char *typestr[] = { "???", "pkg", "upd", "pkgupd" };

void
main(int argc, char **argv)
{
	char *tm, *q;
	Wrap *w;
	Update *u, *eu;

	ARGBEGIN{
	case 'r':
		root = ARGF();
		break;
	default:
		usage();
	}ARGEND

	rfork(RFNAMEG);
	fmtinstall('M', md5conv);

	if(argc != 1)
		usage();

	w = openwraphdr(argv[0], root, nil, -1);
	if(w == nil)
		sysfatal("no such package found: %r");

	tm = asctime(localtime(w->time));
	if(q = strchr(tm, '\n'))
		*q = '\0';

	print("%s (full package as of %s)\n", w->name, tm);

	for(u=w->u, eu=u+w->nu; u<eu; u++) {
		tm = asctime(localtime(u->time));
		if(q = strchr(tm, '\n'))
			*q = '\0';

		if(u->type < 0 || u->type >= nelem(typestr))
			print("%s", typestr[0]);
		else
			print("%s", typestr[u->type]);
		print(" %lld", u->time);
		if(u->type & UPD)
			print(" updating %lld", u->utime);
		if(u->desc)
			print(": %s", u->desc);
		print("\n");
	}
}
