/*
 * wrap/wcmp - cmp against package
 *
 * can be done without blowing up the
 * archive thanks to the md5 checksums.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include "wrap.h"

int bflag;
char *root = "/";

void
diff(Wrap *w, char *name)
{
	Waitmsg wmsg;
	char *flag;
	char *wrapped, *local;

	if(bflag)
		flag = "-mbn";
	else
		flag = "-mn";

	wrapped = mkpath(w->root, name);
	local = mkpath(root, name);

	switch(fork()) {
	case 0:
		execl("/bin/diff", "diff", flag, wrapped, local, nil);
		fprint(2, "exec failed\n");
		_exits(0);
	case -1:
		sysfatal("fork failed");
	default:
		wait(&wmsg);
		free(wrapped);
		free(local);
	}
}

void
usage(void)
{
	fprint(2, "wrap/wdiff [-b] [-r root] package [prefix...]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i;
	Wrap *w;

	ARGBEGIN{
	case 'b':
		bflag = 1;
		break;
	case 'r':
		root = ARGF();
		break;
	default:
		usage();
	}ARGEND

	if(argc < 1)
		usage();

	if(access(argv[0], 0) < 0)
		sysfatal("no such file '%s'", argv[0]);

	w = openwraphdr(argv[0], root, argv-1, argc+1);
	if(w == nil)
		sysfatal("no such package found");

	for(i=1; i<argc; i++)
		diff(w, argv[i]);
}
