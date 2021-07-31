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
int package;
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
	Biobuf *b;
	Dir d;
	Wrap *w;
	char *p, *q, *f[2];
	int i;
	uchar digest[MD5dlen], digest0[MD5dlen];
	vlong t;

	ARGBEGIN{
	case 'b':
		bflag = 1;
		break;
	case 'l':
		list = 1;
		break;
	case 'p':
		package = 1;
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

	if(list)
		w = openwraphdr(argv[0], root, nil, -1);
	else {
		if(access(argv[0], 0) < 0)
			sysfatal("no such file '%s'", argv[0]);

		w = openwraphdr(argv[0], root, argv+1, argc-1);
	}

	if(w == nil)
		sysfatal("no such package found");

	if(package) {
		while(w->nu > 0 && w->u[w->nu-1].type == UPD)
			w->nu--;
	}

	/*
	 * Loop through each md5sum file of each package,
	 * in increasing time order.
	 */
	for(i=0; i<w->nu; i++) {
		b = Bopen(mkpath(w->u[i].dir, "md5sum"), OREAD);
		if(b == nil)
			sysfatal("md5sum file not found");

		while(p = Brdline(b, '\n')) {
			p[Blinelen(b)-1] = '\0';
			if(tokenize(p, f, 2) != 2)
				sysfatal("error in md5sum file");
			if(argc != 1 && match(f[0], argv+1, argc-1) == 0)
				continue;

			/* Ignore directories. */	
			q = mkpath(root, f[0]);
			if(dirstat(q, &d) >= 0 && (d.mode & CHDIR)) {
				free(q);
				continue;
			}

			if(getfileinfo(w, f[0], &t, nil, digest0) < 0) {
				free(q);
				print("can't happen\n");
				continue;
			}

			/* If the file is covered by a later update, handle it later. */
			if(t != w->u[i].time) {
				free(q);
				continue;
			}

			/* If the file does not exist, complain. */	
			if(md5file(q, digest) < 0) {
				if(list)
					print("# %s\n", f[0]);
				else
					print("%s removed\n", f[0]);
				free(q);
				continue;
			}
			free(q);

			/* If the digest doesn't match, complain. */
			if(memcmp(digest, digest0, MD5dlen) != 0) {
				if(list)
					print("%s\n", f[0]);
				else
					diff(w, f[0]);
			}
		}
	}
}
