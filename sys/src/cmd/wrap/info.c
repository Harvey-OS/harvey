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

char *xtypestr[] = { "???", "package", "update", "full update" };

void
main(int argc, char **argv)
{
	char *tm, *q;
	Wrap *w;
	Update *u;

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

	tm = asctime(localtime(w->tfull));
	if(q = strchr(tm, '\n'))
		*q = '\0';

	print("%s (complete as of %s)\n", w->name, tm);

	for(u=w->u+w->nu; --u >= w->u; ) {
		tm = asctime(localtime(u->time));
		if(q = strchr(tm, '\n'))
			*q = '\0';

		if(u->type < 0 || u->type >= nelem(xtypestr))
			print("%s", xtypestr[0]);
		else
			print("%s", xtypestr[u->type]);
		print(" %lld", u->time);
		if(u->type & UPD)
			print(" updating %lld", u->utime);
		if(u->desc)
			print(": %s", u->desc);
		print("\n");
	}
}
