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

void
main(int argc, char **argv)
{
	int i;
	char *tm, *q;
	Wrap *w;

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

	tm = asctime(localtime(w->u->time));
	if(q = strchr(tm, '\n'))
		*q = '\0';

	print("package %s %s\n", w->name, tm);
	if(w->u->desc)
		print("desc %s\n", w->u->desc);

	for(i=1; i<w->nu; i++) {
		tm = asctime(localtime(w->u->time));
		if(q = strchr(tm, '\n'))
			*q = '\0';
		print("update %s", tm);
		tm = asctime(localtime(w->u->utime));
		if(q = strchr(tm, '\n'))
			*q = '\0';
		print(" to %s", tm);
		if(w->u[i].desc)
			print(" %s", w->u[i].desc);
		print("\n");
	}
}
