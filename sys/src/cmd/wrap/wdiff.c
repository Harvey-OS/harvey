/*
 * wrap/wdiff - diff files against package contents
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include <libsec.h>
#include "wrap.h"

int bflag;
int list;
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
	fprint(2, "wrap/wdiff [-bl] [-r root] package [prefix...]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	Wrap *w;
	char *p, *q, *f[2];
	uchar digest[MD5dlen];
	char str[4*MD5dlen];
	Biobuf *b;
	Dir d;

	ARGBEGIN{
	case 'b':
		bflag = 1;
		break;
	case 'l':
		list = 1;
		break;
	case 'r':
		root = ARGF();
		break;
	default:
		usage();
	}ARGEND

	rfork(RFNAMEG);
	fmtinstall('M', md5conv);

	if(argc < 1)
		usage();

	if(access(argv[0], 0) < 0)
		sysfatal("no such file '%s'", argv[0]);

	if(list)
		w = openwraphdr(argv[0], root, nil, 0);
	else
		w = openwraphdr(argv[0], root, argv+1, argc-1);

	if(w == nil)
		sysfatal("no such package found");

	b = Bopen(mkpath(w->u->dir, "md5sum"), OREAD);
	if(b == nil)
		sysfatal("md5sum file not found");
	while(p = Brdline(b, '\n')) {
		p[Blinelen(b)-1] = '\0';
		if(tokenize(p, f, 2) != 2)
			sysfatal("error in md5sum file");
		if(argc != 1 && match(p, argv+1, argc-1) == 0)
			continue;

		q = mkpath(root, p);
		if(dirstat(q, &d) >= 0 && (d.mode & CHDIR)) {
			free(q);
			continue;
		}

		if(md5file(q, digest) < 0) {
			print("%s removed\n", p);
			free(q);
			continue;
		}
		free(q);

		snprint(str, sizeof str, "%M", digest);
		if(strcmp(str, f[1]) == 0)
			continue;

		if(list)
			print("%s modified\n", p);
		else
			diff(w, p);
	}
}
