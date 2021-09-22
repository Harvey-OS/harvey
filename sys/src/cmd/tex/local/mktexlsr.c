/*
 * generate ls-R listings for /sys/lib/texmf indices
 * supposed to mimic unix's ls -LAR which looks approximately like this:

./file1
./file2
./file3

./dir1:
fileindir1
file2indir1

./dir2:
fileindir2

./dir2/dir3:
fileindir2dir3

 *
 * and they want some silly comment at the top:
 *

% ls-R -- filename database for kpathsea; do not change this line.

 */

#include <u.h>
#include <libc.h>
#include <bio.h>

void
lsr(char *prefix, int hdr)
{
	char *nprefix;
	int i, fd, n, len;
	Dir *dbuf;

	if(hdr)
		print("\n%s:\n", prefix);

	fd = open(".", OREAD);
	if(fd < 0) {
		fprint(2, "warning: could not read directory %s\n", prefix);
		return;
	}

	while((n = dirread(fd, &dbuf)) > 0) {
		for(i=0; i<n; i++) {
			if(dbuf[i].mode & DMDIR)
				continue;
			print("%s\n", dbuf[i].name);
		}
		free(dbuf);
	}

	len = 256;
	nprefix = malloc(len);
	if(nprefix == nil) {
		fprint(2, "warning: out of memory\n");
		close(fd);
		return;
	}

	close(fd);
	fd = open(".", OREAD);
	if(fd < 0) {
		fprint(2, "warning: could not read directory %s\n", prefix);
		return;
	}

	while((n = dirread(fd, &dbuf)) > 0) {
		for(i=0; i<n; i++) {
			if(!(dbuf[i].mode & DMDIR))
				continue;
			snprint(nprefix, len, "%s/%s", prefix, dbuf[i].name);
			if(chdir(dbuf[i].name) < 0) {
				fprint(2, "warning: couldn't chdir to %s (%s)\n", nprefix, dbuf[i].name);
				continue;
			}
			lsr(nprefix, 1);
			if(chdir("..") < 0) {
				fprint(2, "error: couldn't cd .. from %s\n", nprefix);
				exits("cd ..");
			}
		}
	}
	close(fd);
	free(nprefix);
}

void
usage(void)
{
	fprint(2, "usage: lsr [-p prefix] [dir]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *prefix;
	char *dir;

	prefix = nil;
	ARGBEGIN{
	case 'p':
		prefix=ARGF();
		break;
	default:
		usage();
	}ARGEND

	switch(argc) {
	case 0:
		dir = ".";
		break;
	case 1:
		dir = argv[0];
		break;
	default:
		SET(dir);
		usage();
	}

	if(prefix == nil)
		prefix = dir;

	if(chdir(dir) < 0) {
		fprint(2, "error: couldn't chdir to %s\n", dir);
		exits("chdir");
	}
	print("%% ls-R -- filename database for kpathsea; do not change this line.\n");
	lsr(prefix, 0);
	exits(0);
}
